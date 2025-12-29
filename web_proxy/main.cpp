#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fstream>
#include <algorithm>

using namespace std;

const int PROXY_PORT = 8888;
const int BUFFER_SIZE = 8192;
const string CACHE_DIR = "cache/";

struct ProxyStats
{
    int totalRequests = 0;
    int cacheHits = 0;
    int cacheMisses = 0;
    int errorResponses = 0;
};

ProxyStats globalStats;

string GetCacheFilename(string url)
{
    string filename = url;
    replace(filename.begin(), filename.end(), '/', '_');
    replace(filename.begin(), filename.end(), ':', '_');
    replace(filename.begin(), filename.end(), '?', '_');
    return CACHE_DIR + filename;
}

bool FileExists(string name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool IsSuccessResponse(const char* buffer, int length)
{
    string responseHeader(buffer, length);
    size_t firstLineEnd = responseHeader.find("\r\n");
    if (firstLineEnd != string::npos)
    {
        string firstLine = responseHeader.substr(0, firstLineEnd);
        if (firstLine.find("200") != string::npos)
        {
            return true;
        }
    }
    return false;
}

void ParseRequest(string request, string& host, string& path)
{
    size_t firstSpace = request.find(" ");
    size_t secondSpace = request.find(" ", firstSpace + 1);
    if (firstSpace == string::npos || secondSpace == string::npos) return;

    string url = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);

    if (url.find("http://") == 0) url = url.substr(7);

    size_t slashPos = url.find("/");
    if (slashPos != string::npos)
    {
        host = url.substr(0, slashPos);
        path = url.substr(slashPos);
    }
    else
    {
        host = url;
        path = "/";
    }
}

void SendErrorResponse(int clientSocket, string code, string msg)
{
    string response = "HTTP/1.0 " + code + " " + msg + "\r\n";
    response += "Content-Type: text/html\r\n\r\n";
    response += "<html><body><h1>" + code + " " + msg + "</h1></body></html>";
    send(clientSocket, response.c_str(), response.length(), 0);
}

void PrintStats()
{
    cout << "[INFO] Stats -> Hits: " << globalStats.cacheHits 
         << ", Misses: " << globalStats.cacheMisses 
         << ", Errors: " << globalStats.errorResponses << endl;
}

void HandleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesReceived <= 0)
    {
        close(clientSocket);
        return;
    }

    string request(buffer, bytesReceived);
    if (request.find("GET") != 0)
    {
        SendErrorResponse(clientSocket, "501", "Not Implemented");
        close(clientSocket);
        return;
    }

    globalStats.totalRequests++;
    string host = "", path = "";
    ParseRequest(request, host, path);

    if (host.empty())
    {
        close(clientSocket);
        return;
    }

    string cacheFile = GetCacheFilename(host + path);

    if (FileExists(cacheFile))
    {
        cout << "[CACHE HIT] " << host << path << endl;
        globalStats.cacheHits++;
        
        ifstream file(cacheFile, ios::binary);
        while (file.read(buffer, BUFFER_SIZE) || file.gcount() > 0)
        {
            send(clientSocket, buffer, file.gcount(), 0);
        }
        file.close();
    }
    else
    {
        cout << "[CACHE MISS] Fetching " << host << path << endl;
        globalStats.cacheMisses++;

        struct hostent* server = gethostbyname(host.c_str());
        if (!server)
        {
            globalStats.errorResponses++;
            SendErrorResponse(clientSocket, "404", "Not Found");
            PrintStats();
            close(clientSocket);
            return;
        }

        int targetSocket = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(80);
        memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);

        if (connect(targetSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
        {
            globalStats.errorResponses++;
            SendErrorResponse(clientSocket, "502", "Bad Gateway");
            PrintStats();
            close(targetSocket);
            close(clientSocket);
            return;
        }

        string remoteRequest = "GET " + path + " HTTP/1.0\r\n";
        remoteRequest += "Host: " + host + "\r\n";
        remoteRequest += "Connection: close\r\n\r\n";
        send(targetSocket, remoteRequest.c_str(), remoteRequest.length(), 0);

        int n = recv(targetSocket, buffer, BUFFER_SIZE, 0);
        if (n > 0)
        {
            bool shouldCache = IsSuccessResponse(buffer, n);
            ofstream cacheOut;
            
            if (shouldCache)
            {
                cacheOut.open(cacheFile, ios::binary);
            }
            else
            {
                cout << "[INFO] Response is not 200 OK. Not caching." << endl;
                globalStats.errorResponses++;
            }

            send(clientSocket, buffer, n, 0);
            if (shouldCache && cacheOut.is_open()) cacheOut.write(buffer, n);

            while ((n = recv(targetSocket, buffer, BUFFER_SIZE, 0)) > 0)
            {
                send(clientSocket, buffer, n, 0);
                if (shouldCache && cacheOut.is_open()) cacheOut.write(buffer, n);
            }
            
            if (cacheOut.is_open()) cacheOut.close();
        }
        close(targetSocket);
    }

    PrintStats();
    close(clientSocket);
}

int main()
{
    mkdir(CACHE_DIR.c_str(), 0777);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in proxyAddr;
    proxyAddr.sin_family = AF_INET;
    proxyAddr.sin_addr.s_addr = INADDR_ANY;
    proxyAddr.sin_port = htons(PROXY_PORT);

    if (bind(serverSocket, (struct sockaddr*)&proxyAddr, sizeof(proxyAddr)) < 0)
    {
        return 1;
    }

    listen(serverSocket, 10);
    cout << "HTTP Proxy Server started on port " << PROXY_PORT << endl;

    while (true)
    {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);
        
        if (clientSocket >= 0)
        {
            HandleClient(clientSocket);
        }
    }

    close(serverSocket);
    return 0;
}