#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#include <iomanip>
#include <vector>
#include <algorithm>

using namespace std;

const int MAX_PINGS = 10;
const int TIMEOUT_SEC = 1;
const int BUFFER_SIZE = 1024;
const int PORT = 12000;

double GetTimestampMs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

int main(int argc, char** argv)
{
    string serverIp = "127.0.0.1";
    if (argc > 1) serverIp = argv[1];

    int clientSocket = socket(AF_INET, SOCK_DGRAM, 0);

    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, serverIp.c_str(), &serverAddr.sin_addr);

    vector<double> rttList;
    int packetsLost = 0;

    for (int i = 1; i <= MAX_PINGS; i++)
    {
        double startTime = GetTimestampMs();
        string message = "Ping " + to_string(i) + " " + to_string((long)startTime);
        
        sendto(clientSocket, message.c_str(), message.length(), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        
        int n = recvfrom(clientSocket, buffer, BUFFER_SIZE, 0, NULL, NULL);
        double endTime = GetTimestampMs();

        if (n >= 0)
        {
            double rtt = (endTime - startTime) / 1000.0;
            rttList.push_back(rtt);
            cout << "Ответ от сервера: " << buffer << ", RTT = " << fixed << setprecision(6) << rtt << " сек" << endl;
        }
        else
        {
            cout << "Request timed out" << endl;
            packetsLost++;
        }
    }

    if (!rttList.empty())
    {
        double minRtt = *min_element(rttList.begin(), rttList.end());
        double maxRtt = *max_element(rttList.begin(), rttList.end());
        double sumRtt = 0;
        for (double r : rttList) sumRtt += r;
        
        cout << "\n--- Статистика Ping ---" << endl;
        cout << "Пакетов: отправлено = " << MAX_PINGS << ", получено = " << rttList.size() 
             << ", потеряно = " << packetsLost << " (" << (packetsLost * 100 / MAX_PINGS) << "% потерь)" << endl;
        cout << "RTT min/avg/max = " << minRtt << "/" << (sumRtt / rttList.size()) << "/" << maxRtt << " сек" << endl;
    }

    close(clientSocket);
    return 0;
}