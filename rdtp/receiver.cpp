#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

const int PAYLOAD_SIZE = 1024;
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

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <port> <output_file>" << endl;
        return 1;
    }

    int port = stoi(argv[1]);
    string outFileName = argv[2];

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = INADDR_ANY;
    servAddr.sin_port = htons(port);

    bind(sock, (struct sockaddr*)&servAddr, sizeof(servAddr));

    ofstream outFile(outFileName, ios::binary);
    uint32_t expectedSeq = 0;

    cout << "Receiver ready on port " << port << endl;

    while (true)
    {
        Packet pkt;
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int bytes = recvfrom(sock, &pkt, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (bytes <= 0) break;

        if (pkt.checksum == CalculateChecksum(pkt) && pkt.type == TYPE_DATA)
        {
            if (pkt.seqNum == expectedSeq)
            {
                outFile.write(pkt.payload, pkt.payloadSize);
                outFile.flush();
                
                Packet ack;
                ack.seqNum = expectedSeq;
                ack.type = TYPE_ACK;
                ack.payloadSize = 0;
                ack.checksum = CalculateChecksum(ack);
                sendto(sock, &ack, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, addrLen);
                
                expectedSeq++;
            }
            else
            {
                Packet lastAck;
                lastAck.seqNum = (expectedSeq > 0) ? expectedSeq - 1 : 0;
                lastAck.type = TYPE_ACK;
                lastAck.payloadSize = 0;
                lastAck.checksum = CalculateChecksum(lastAck);
                sendto(sock, &lastAck, sizeof(Packet), 0, (struct sockaddr*)&clientAddr, addrLen);
            }
        }
    }

    outFile.close();
    close(sock);
    return 0;
}