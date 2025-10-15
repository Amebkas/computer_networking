#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>

const int PORT = 5001;
const char* SERVER_IP = "127.0.0.1";
const std::string CLIENT_NAME = "Some client";

void SendMessage(int socket_fd, const std::string& clientName, int clientNumber)
{
    std::string message = clientName + ":" + std::to_string(clientNumber);
    send(socket_fd, message.c_str(), message.length(), 0);
    std::cout << "Отправлено сообщение серверу: '" << message << "'" << std::endl;
}

std::pair<std::string, int> ReceiveResponse(int socket_fd)
{
    char buffer[1024] = {0};
    recv(socket_fd, buffer, sizeof(buffer), 0);
    std::string response(buffer);
    std::cout << "Получен ответ от сервера: '" << response << "'" << std::endl;

    size_t delimiterPos = response.find(':');
    if (delimiterPos != std::string::npos)
    {
        std::string serverName = response.substr(0, delimiterPos);
        int serverNumber = std::stoi(response.substr(delimiterPos + 1));
        return {serverName, serverNumber};
    }
    return {"", 0};
}

int main()
{
    int clientNumber;
    std::cout << "Введите целое число от 1 до 100: ";
    std::cin >> clientNumber;

    int clientSocket = 0;
    struct sockaddr_in serv_addr;

    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return -1;
    }
    std::cout << "Сокет создан." << std::endl;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0)
    {
        std::cerr << "Недопустимый адрес / Адрес не поддерживается" << std::endl;
        return -1;
    }

    if (connect(clientSocket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        return -1;
    }
    std::cout << "Подключено к серверу " << SERVER_IP << ":" << PORT << std::endl;

    SendMessage(clientSocket, CLIENT_NAME, clientNumber);

    std::pair<std::string, int> serverResponse = ReceiveResponse(clientSocket);
    std::string serverName = serverResponse.first;
    int serverNumber = serverResponse.second;

    if (!serverName.empty())
    {
        std::cout << "--- Итоги ---" << std::endl;
        std::cout << "Мое имя: " << CLIENT_NAME << std::endl;
        std::cout << "Имя сервера: " << serverName << std::endl;
        std::cout << "Мое число: " << clientNumber << std::endl;
        std::cout << "Число сервера: " << serverNumber << std::endl;
        std::cout << "Сумма чисел: " << (clientNumber + serverNumber) << std::endl << std::endl;
    } else
    {
        std::cerr << "Ошибка при получении или парсинге ответа сервера." << std::endl;
    }

    close(clientSocket);
    std::cout << "Сокет закрыт. Завершение работы." << std::endl;

    return 0;
}