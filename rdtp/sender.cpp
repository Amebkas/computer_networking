#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <chrono>

using namespace std;

const int PAYLOAD_SIZE = 1024;
const int WINDOW_SIZE = 10;
const int TIMEOUT_MS = 200;
const int TYPE_DATA = 0;
const int TYPE_ACK = 1;

struct Packet
{
    uint32_t seqNum;
    uint32_t type;
    uint32_t payloadSize;
    uint32_t checksum;
    char payload[PAYLOAD_SIZE];
};

uint32_t CalculateChecksum(const Packet& pkt)
{
    uint32_t sum = pkt.seqNum + pkt.type + pkt.payloadSize;
    for (int i = 0; i < (int)pkt.payloadSize; i++)
    {
        sum += (unsigned char)pkt.payload[i];
    }
    return sum;
}

bool IsDebug = false;

void LogDebug(string message)
{
    if (IsDebug)
    {
        cout << "[DEBUG] " << message << endl;
    }
}

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        cout << "Usage: " << argv[0] << " <ip> <port> <file> [-d]" << endl;
        return 1;
    }

    string receiverIp = argv[1];
    int receiverPort = stoi(argv[2]);
    string fileName = argv[3];

    if (argc > 4 && string(argv[4]) == "-d") IsDebug = true;

    ifstream file(fileName, ios::binary);
    if (!file.is_open()) return 1;

    vector<Packet> allPackets;
    uint32_t currentSeq = 0;
    while (!file.eof())
    {
        Packet pkt;
        memset(&pkt, 0, sizeof(pkt));
        file.read(pkt.payload, PAYLOAD_SIZE);
        pkt.payloadSize = file.gcount();
        if (pkt.payloadSize == 0) break;
        pkt.seqNum = currentSeq++;
        pkt.type = TYPE_DATA;
        pkt.checksum = CalculateChecksum(pkt);
        allPackets.push_back(pkt);
    }
    file.close();

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(receiverPort);
    inet_pton(AF_INET, receiverIp.c_str(), &destAddr.sin_addr);

    uint32_t base = 0;
    uint32_t nextSeq = 0;
    int totalPackets = allPackets.size();

    auto startTime = chrono::steady_clock::now();

    while (base < (uint32_t)totalPackets)
    {
        while (nextSeq < base + WINDOW_SIZE && nextSeq < (uint32_t)totalPackets)
        {
            LogDebug("Sending packet " + to_string(nextSeq));
            sendto(sock, &allPackets[nextSeq], sizeof(Packet), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
            nextSeq++;
        }

        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(sock, &readFds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = TIMEOUT_MS * 1000;

        int res = select(sock + 1, &readFds, NULL, NULL, &tv);
        if (res > 0)
        {
            Packet ackPkt;
            recvfrom(sock, &ackPkt, sizeof(Packet), 0, NULL, NULL);
            if (ackPkt.type == TYPE_ACK && ackPkt.checksum == CalculateChecksum(ackPkt))
            {
                LogDebug("Received ACK " + to_string(ackPkt.seqNum));
                if (ackPkt.seqNum >= base)
                {
                    base = ackPkt.seqNum + 1;
                }
            }
        }
        else
        {
            LogDebug("Timeout! Resending window from " + to_string(base));
            nextSeq = base;
        }

        if (!IsDebug)
        {
            cout << "\rProgress: " << (base * 100 / totalPackets) << "%" << flush;
        }
    }

    auto endTime = chrono::steady_clock::now();
    chrono::duration<double> diff = endTime - startTime;
    cout << "\nTransfer complete. Time: " << diff.count() << "s, Speed: " << (totalPackets * PAYLOAD_SIZE / 1024.0 / diff.count()) << " KB/s" << endl;

    close(sock);
    return 0;
}