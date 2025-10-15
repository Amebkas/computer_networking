#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"

using namespace std;

const string CLIENT_NAME = "Some client";
const char* SERVER_IP = "127.0.0.1";

int GetClientNumber()
{
    int clientNumber;
    while (true)
    {
        cout << "Client: Введите целое число от 1 до 100: ";
        cin >> clientNumber;
        if (IsValidNumber(clientNumber))
        {
            return clientNumber;
        } else
        {
            cerr << "Client: Число вне допустимого диапазона (1-100). Пожалуйста, попробуйте еще раз." << endl;
        }
    }
}

int CreateClientSocket()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0)
    {
        cerr << "Client: Ошибка создания сокета" << endl;
        return -1;
    }
    cout << "Client: Сокет создан." << endl;
    return clientSocket;
}

bool ConnectToServer(int clientSocket, const char* serverIp, int port)
{
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIp, &serv_addr.sin_addr) <= 0)
    {
        cerr << "Client: Недопустимый адрес / Адрес не поддерживается" << endl;
        return false;
    }

    if (connect(clientSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        cerr << "Client: Ошибка подключения к серверу" << endl;
        return false;
    }
    cout << "Client: Подключено к серверу " << serverIp << ":" << port << endl;
    return true;
}

void ProcessServerResponse(const Message& clientMessage, const Message& serverResponse)
{
    if (!serverResponse.m_Name.empty())
    {
        cout << "Client: --- Итоги ---" << endl;
        cout << "Client: Мое имя: " << clientMessage.m_Name << endl;
        cout << "Client: Имя сервера: " << serverResponse.m_Name << endl;
        cout << "Client: Мое число: " << clientMessage.m_Number << endl;
        cout << "Client: Число сервера: " << serverResponse.m_Number << endl;
        cout << "Client: Сумма чисел: " << (clientMessage.m_Number + serverResponse.m_Number) << endl << endl;
    } else
    {
        cerr << "Client: Ошибка при получении или парсинге ответа сервера." << endl;
    }
}

int main()
{
    int clientNumber = GetClientNumber();

    int clientSocket = CreateClientSocket();
    if (clientSocket == -1)
    {
        return 1;
    }

    if (!ConnectToServer(clientSocket, SERVER_IP, PORT))
    {
        close(clientSocket);
        return 1;
    }

    Message clientMessage(CLIENT_NAME, clientNumber);
    SendData(clientSocket, clientMessage);
    cout << "Client: Отправлено сообщение серверу: '" << SerializeMessage(clientMessage) << "'" << endl;

    Message serverResponse = ReceiveData(clientSocket);
    cout << "Client: Получен ответ от сервера: '" << SerializeMessage(serverResponse) << "'" << endl;

    ProcessServerResponse(clientMessage, serverResponse);

    close(clientSocket);
    cout << "Client: Сокет закрыт. Завершение работы." << endl;

    return 0;
}