#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>

using namespace std;

const int BUFFER_SIZE = 1024;
const int PORT = 12000;

int main()
{
    int serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    srand(time(NULL));
    cout << "UDP Pinger Server started on port " << PORT << endl;

    while (true)
    {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        int n = recvfrom(serverSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clientAddr, &addrLen);
        
        if (rand() % 10 < 3)
        {
            cout << "Packet dropped (simulation)" << endl;
            continue;
        }

        sendto(serverSocket, buffer, n, 0, (struct sockaddr*)&clientAddr, addrLen);
    }

    close(serverSocket);
    return 0;
}