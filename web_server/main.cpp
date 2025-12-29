#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;

const int SERVER_PORT = 8080;
const int BUFFER_SIZE = 2048;

void LogDebug(string message)
{
    cout << "[DEBUG] " << message << endl;
}

string GetContentType(string path)
{
    if (path.find(".html") != string::npos) return "text/html";
    if (path.find(".jpg") != string::npos || path.find(".jpeg") != string::npos) return "image/jpeg";
    if (path.find(".png") != string::npos) return "image/png";
    if (path.find(".css") != string::npos) return "text/css";
    return "text/plain";
}

void HandleRequest(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    int bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
    if (bytesRead <= 0)
    {
        close(clientSocket);
        return;
    }

    string request(buffer);
    
    size_t endOfFirstLine = request.find("\r\n");
    if (endOfFirstLine != string::npos)
    {
        LogDebug("Received Request: " + request.substr(0, endOfFirstLine));
    }

    stringstream ss(request);
    string method, path, protocol;
    ss >> method >> path >> protocol;

    if (method != "GET")
    {
        string response = "HTTP/1.1 501 Not Implemented\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        write(clientSocket, response.c_str(), response.length());
        close(clientSocket);
        return;
    }

    if (path == "/") path = "/index.html";
    
    string fileName = path.substr(1);
    ifstream file(fileName, ios::binary);

    if (file.is_open())
    {
        file.seekg(0, ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, ios::beg);

        string contentType = GetContentType(fileName);
        stringstream header;
        header << "HTTP/1.1 200 OK\r\n"
               << "Content-Type: " << contentType << "\r\n"
               << "Content-Length: " << fileSize << "\r\n"
               << "Connection: close\r\n\r\n";

        string headerStr = header.str();
        write(clientSocket, headerStr.c_str(), headerStr.length());

        vector<char> fileBuffer(fileSize);
        file.read(fileBuffer.data(), fileSize);
        write(clientSocket, fileBuffer.data(), fileSize);

        cout << "[200 OK] Served: " << fileName << endl;
        file.close();
    }
    else
    {
        string body = "File Not Found";
        stringstream header;
        header << "HTTP/1.1 404 Not Found\r\n"
               << "Content-Type: text/plain\r\n"
               << "Content-Length: " << body.length() << "\r\n"
               << "Connection: close\r\n\r\n"
               << body;

        string response = header.str();
        write(clientSocket, response.c_str(), response.length());

        cout << "[404 Not Found] Tried: " << fileName << endl;
    }

    close(clientSocket);
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        cerr << "Socket creation failed" << endl;
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "Bind failed" << endl;
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 10) < 0)
    {
        cerr << "Listen failed" << endl;
        close(serverSocket);
        return 1;
    }

    cout << "HTTP Server is running on port " << SERVER_PORT << "..." << endl;

    while (true)
    {
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrLen);

        if (clientSocket >= 0)
        {
            HandleRequest(clientSocket);
        }
    }

    close(serverSocket);
    return 0;
}