#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

using namespace std;

const int BUFFER_SIZE = 4096;
const int SMTP_PORT = 1025;

string ReadLine(int clientSocket)
{
    string line = "";
    char c;
    while (read(clientSocket, &c, 1) > 0)
    {
        line += c;
        if (line.length() >= 2 && line.substr(line.length() - 2) == "\r\n")
        {
            break;
        }
    }
    return line;
}

bool ExpectResponse(int clientSocket, int expectedCode)
{
    string fullResponse = "";
    string lastLine = "";
    
    while (true)
    {
        lastLine = ReadLine(clientSocket);
        fullResponse += lastLine;
        if (lastLine.length() < 4) break;
        if (lastLine[3] == ' ') break;
    }

    cout << "S: " << fullResponse;
    
    if (fullResponse.empty()) return false;
    int actualCode = stoi(fullResponse.substr(0, 3));
    return actualCode == expectedCode;
}

void SendCommand(int clientSocket, string command)
{
    cout << "C: " << command;
    write(clientSocket, command.c_str(), command.length());
}

string GetIpByHostname(string hostname)
{
    struct hostent* he;
    struct in_addr** addrList;
    he = gethostbyname(hostname.c_str());
    if (he == NULL) return "";
    addrList = (struct in_addr**)he->h_addr_list;
    return inet_ntoa(*addrList[0]);
}

int main()
{
    string smtpServer = "127.0.0.1";
    string sender = "test@example.com";
    string recipient = "your_email@mail.ru";

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    string serverIp = GetIpByHostname(smtpServer);
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SMTP_PORT);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) return 1;

    if (!ExpectResponse(clientSocket, 220)) return 1;

    SendCommand(clientSocket, "EHLO myclient.com\r\n");
    if (!ExpectResponse(clientSocket, 250)) return 1;

    SendCommand(clientSocket, "MAIL FROM: <" + sender + ">\r\n");
    if (!ExpectResponse(clientSocket, 250)) return 1;

    SendCommand(clientSocket, "RCPT TO: <" + recipient + ">\r\n");
    if (!ExpectResponse(clientSocket, 250)) return 1;

    SendCommand(clientSocket, "DATA\r\n");
    if (!ExpectResponse(clientSocket, 354)) return 1;

    string message = "Subject: Test\r\n\r\nHello!\r\n.\r\n";
    SendCommand(clientSocket, message);
    if (!ExpectResponse(clientSocket, 250)) return 1;

    SendCommand(clientSocket, "QUIT\r\n");
    ExpectResponse(clientSocket, 221);

    close(clientSocket);
    return 0;
}