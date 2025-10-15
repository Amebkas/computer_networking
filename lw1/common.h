#pragma once
#include <sys/socket.h>
#include <string>
#include <utility>

using namespace std;

const int PORT = 5001;
const int BUFFER_SIZE = 1024;
const char DELIMETER = ':';
const int MIN_NUMBER = 1;
const int MAX_NUBMER = 100;

struct Message
{
    string m_Name;
    int m_Number;

    Message(const string& name = "", int number = 0)
        : m_Name(name), m_Number(number)
    {}
};

string SerializeMessage(const Message& msg)
{
    return msg.m_Name + DELIMETER + to_string(msg.m_Number);
}

Message DeserializeMessage(const string& messageString)
{
    size_t delimiterPos = messageString.find(DELIMETER);
    if (delimiterPos != string::npos)
    {
        string name = messageString.substr(0, delimiterPos);
        int number = stoi(messageString.substr(delimiterPos + 1));
        return Message(name, number);
    }
    return Message();
}

void SendData(int socket_fd, const Message& msg)
{
    string messageString = SerializeMessage(msg);
    send(socket_fd, messageString.c_str(), messageString.length(), 0);
}

Message ReceiveData(int socket_fd)
{
    char buffer[BUFFER_SIZE] = {0};
    recv(socket_fd, buffer, sizeof(buffer), 0);
    return DeserializeMessage(string(buffer));
}

bool IsValidNumber(int number)
{
    return number >= MIN_NUMBER && number <= MAX_NUBMER;
}