#include <iostream>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <vector>

const int PORT = 5001;
const std::string SERVER_NAME = "T.A.H.I.R.";
const int SERVER_FIXED_NUMBER = 50;

void HandleClient(int clientSocket)
{
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::string clientMessage(buffer);
    std::cout << "Сообщение клиента: '" << clientMessage << "'" << std::endl;

    size_t delimiterPos = clientMessage.find(':');
    if (delimiterPos == std::string::npos)
    {
        std::cerr << "[ERROR] Ошибка парсинга сообщения клиента." << std::endl;
        close(clientSocket);
        return;
    }

    std::string clientName = clientMessage.substr(0, delimiterPos);
    int clientNumber = std::stoi(clientMessage.substr(delimiterPos + 1));

    std::cout << "--- Данные клиента ---" << std::endl;
    std::cout << "Имя клиента: " << clientName << std::endl;
    std::cout << "Имя сервера: " << SERVER_NAME << std::endl;
    std::cout << "Число клиента: " << clientNumber << std::endl;
    std::cout << "Число сервера: " << SERVER_FIXED_NUMBER << std::endl;
    std::cout << "Сумма чисел = " << (clientNumber + SERVER_FIXED_NUMBER) << std::endl;
	std::cout << std::endl;

    std::string response = SERVER_NAME + ":" + std::to_string(SERVER_FIXED_NUMBER);
    send(clientSocket, response.c_str(), response.length(), 0);
    std::cout << "Отправлен ответ клиенту: '" << response << "'" << std::endl;

    if (clientNumber < 1 || clientNumber > 100)
    {
        std::cout << "Число вне диапазона (1-100). Завершение работы сервера." << std::endl;
        close(clientSocket);
        exit(0);
    }

    close(clientSocket);
    std::cout << "Сокет клиента закрыт." << std::endl;
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return -1;
    }
    std::cout << "Сокет сервера создан." << std::endl;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "Ошибка конфигурации сокета" << std::endl;
        close(server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "Сокет привязан к порту " << PORT << std::endl;

    if (listen(server_fd, 3) < 0)
    {
        std::cerr << "Ошибка прослушивания" << std::endl;
        close(server_fd);
        return -1;
    }
    std::cout << "Начато прослушивание входящих подключений на порту " << PORT << std::endl;

    while (true)
    {
        std::cout << "Ожидание нового подключения..." << std::endl;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
        {
            std::cerr << "Ошибка при принятии подключения" << std::endl;
            continue;
        }
        std::cout << "Новое подключение " << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << std::endl;

        HandleClient(new_socket);
    }

    close(server_fd);
    std::cout << "Сокет сервера закрыт. Завершение работы." << std::endl;

    return 0;
}