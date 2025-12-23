#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "common.h"

using namespace std;

const string SERVER_NAME = "T.A.H.I.R.";
const int SERVER_FIXED_NUMBER = 50;

int CreateAndBindServerSocket(int port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        cerr << "Server: Ошибка создания сокета" << endl;
        return -1;
    }
    cout << "Server: Сокет сервера создан." << endl;

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        cerr << "Server: Ошибка setsockopt" << endl;
        close(server_fd);
        return -1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        cerr << "Server: Ошибка привязки сокета" << endl;
        close(server_fd);
        return -1;
    }
    cout << "Server: Сокет привязан к порту " << port << endl;

    return server_fd;
}

bool StartListening(int server_fd, int backlog)
{
    if (listen(server_fd, backlog) < 0)
    {
        cerr << "Server: Ошибка прослушивания" << endl;
        return false;
    }
    cout << "Server: Начато прослушивание входящих подключений на порту " << PORT << endl;
    return true;
}

void DisplayClientServerData(const Message& clientMessage, const Message& serverMessage)
{
    cout << "Server: --- Данные клиента ---" << endl;
    cout << "Server: Имя клиента: " << clientMessage.m_Name << endl;
    cout << "Server: Мое имя: " << serverMessage.m_Name << endl;
    cout << "Server: Число от клиента: " << clientMessage.m_Number << endl;
    cout << "Server: Мое число: " << serverMessage.m_Number << endl;
    cout << "Server: Сумма чисел = " << (clientMessage.m_Number + serverMessage.m_Number) << endl;
    cout << endl;
}

// Изменена на void, так как завершение сервера происходит через exit(0)
void HandleSingleClient(int clientSocket)
{
    Message clientMessage = ReceiveData(clientSocket);
    cout << "Server: Получено сообщение от клиента: '" << SerializeMessage(clientMessage) << "'" << endl;

    if (clientMessage.m_Name.empty())
    {
        cerr << "Server: [ERROR] Ошибка парсинга сообщения клиента." << endl;
        close(clientSocket);
        return;
    }

    Message serverResponse(SERVER_NAME, SERVER_FIXED_NUMBER);
    DisplayClientServerData(clientMessage, serverResponse);

    SendData(clientSocket, serverResponse);
    cout << "Server: Отправлен ответ клиенту: '" << SerializeMessage(serverResponse) << "'" << endl;

    if (!IsValidNumber(clientMessage.m_Number))
    {
        cout << "Server: Число вне диапазона (1-100) от клиента. Завершение работы сервера." << endl;
        close(clientSocket);
        exit(0);
    }

    close(clientSocket);
    cout << "Server: Сокет клиента закрыт." << endl;
}

void RunServerLoop(int server_fd)
{
    sockaddr_in address;
    int addrlen = sizeof(address);

    while (true)
    {
        cout << "Server: Ожидание нового подключения..." << endl;
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0)
        {
            cerr << "Server: Ошибка при принятии подключения" << endl;
            continue;
        }
        cout << "Server: Новое подключение от " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << endl;

        HandleSingleClient(new_socket);
    }
}

int main()
{
    int server_fd = CreateAndBindServerSocket(PORT);
    if (server_fd == -1)
    {
        return 1;
    }

    if (!StartListening(server_fd, 3))
    {
        close(server_fd);
        return 1;
    }

    RunServerLoop(server_fd);

    close(server_fd);
    cout << "Server: Сокет сервера закрыт. Завершение работы." << endl;

    return 0;
}