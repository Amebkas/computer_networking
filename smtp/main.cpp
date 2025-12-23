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

void ReceiveResponse(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1);
    if (bytesRead > 0)
    {
        cout << "S: " << buffer;
    }
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
    if (he == NULL)
    {
        return "";
    }
    addrList = (struct in_addr**)he->h_addr_list;
    return inet_ntoa(*addrList[0]);
}

int main()
{ 
    string smtpServer = "127.0.0.1";
    string sender = "test@example.com";
    string recipient = "your_email@mail.ru";
    string subject = "Test Subject";
    string body = "Hello! This is a test message from my C++ SMTP client.";

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    string serverIp = GetIpByHostname(smtpServer);
    if (serverIp == "")
    {
        cerr << "Could not resolve hostname" << endl;
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SMTP_PORT);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "Connection failed" << endl;
        return 1;
    }

    ReceiveResponse(clientSocket);

    SendCommand(clientSocket, "HELO myclient.com\r\n");
    ReceiveResponse(clientSocket);

    SendCommand(clientSocket, "MAIL FROM: <" + sender + ">\r\n");
    ReceiveResponse(clientSocket);

    SendCommand(clientSocket, "RCPT TO: <" + recipient + ">\r\n");
    ReceiveResponse(clientSocket);

    SendCommand(clientSocket, "DATA\r\n");
    ReceiveResponse(clientSocket);

    string message = "From: " + sender + "\r\n" +
                     "To: " + recipient + "\r\n" +
                     "Subject: " + subject + "\r\n" +
                     "\r\n" +
                     body + "\r\n" +
                     ".\r\n";
    
    SendCommand(clientSocket, message);
    ReceiveResponse(clientSocket);

    SendCommand(clientSocket, "QUIT\r\n");
    ReceiveResponse(clientSocket);

    close(clientSocket);
    cout << "Connection closed." << endl;

    return 0;
}