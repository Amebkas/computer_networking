#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

using namespace std;

const int DNS_PORT = 53;
const int BUFFER_SIZE = 65536;
const int TYPE_A = 1;
const int TYPE_AAAA = 28;
const int TYPE_NS = 2;
const char* ROOT_SERVER = "198.41.0.4";

struct DnsHeader
{
    uint16_t id;
    uint16_t flags;
    uint16_t qCount;
    uint16_t ansCount;
    uint16_t authCount;
    uint16_t addCount;
};

struct DnsResourceRecord
{
    string name;
    uint16_t type;
    uint16_t dnsClass;
    uint32_t ttl;
    uint16_t dataLen;
    string rdataStr;
};

bool IsDebug = false;

void LogDebug(string message)
{
    if (IsDebug)
    {
        cout << "[DEBUG] " << message << endl;
    }
}

void FormatDnsName(unsigned char* dns, string host)
{
    int lock = 0;
    string dotHost = host + ".";
    for (int i = 0; i < (int)dotHost.length(); i++)
    {
        if (dotHost[i] == '.')
        {
            *dns++ = i - lock;
            for (; lock < i; lock++)
            {
                *dns++ = dotHost[lock];
            }
            lock++;
        }
    }
    *dns++ = '\0';
}

string ParseName(unsigned char* buffer, unsigned char** reader)
{
    unsigned char* ptr = *reader;
    string name = "";
    bool jumped = false;
    int offset;
    int step = 0;

    while (*ptr != 0)
    {
        if (*ptr >= 192)
        {
            offset = ((*ptr) & 0x3F) * 256 + *(ptr + 1);
            if (!jumped) step = (ptr - *reader) + 2;
            jumped = true;
            ptr = buffer + offset;
        }
        else
        {
            int len = *ptr++;
            for (int i = 0; i < len; i++)
            {
                name += (char)*ptr++;
            }
            name += '.';
        }
    }
    
    if (!jumped) step = (ptr - *reader) + 1;
    *reader += step;
    
    if (name.length() > 0) name.pop_back();
    return name;
}

int BuildQuery(unsigned char* buffer, string host, int queryType)
{
    DnsHeader* dns = (DnsHeader*)buffer;
    dns->id = htons(getpid());
    dns->flags = htons(0x0000);
    dns->qCount = htons(1);
    dns->ansCount = 0;
    dns->authCount = 0;
    dns->addCount = 0;

    unsigned char* qname = buffer + sizeof(DnsHeader);
    FormatDnsName(qname, host);
    
    int nameLen = strlen((char*)qname) + 1;
    uint16_t* qtype = (uint16_t*)(qname + nameLen);
    uint16_t* qclass = (uint16_t*)(qname + nameLen + 2);

    *qtype = htons(queryType);
    *qclass = htons(1);

    return sizeof(DnsHeader) + nameLen + 4;
}

vector<DnsResourceRecord> SendQuery(string server, string host, int type, bool useTcp)
{
    unsigned char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int queryLen = BuildQuery(buffer, host, type);
    int sock;
    int receivedLen = 0;

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(DNS_PORT);
    inet_pton(AF_INET, server.c_str(), &servAddr.sin_addr);

    if (useTcp)
    {
        LogDebug("Switching to TCP for " + server);
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
        {
            close(sock);
            return {};
        }

        uint16_t tcpLen = htons(queryLen);
        send(sock, &tcpLen, 2, 0);
        send(sock, buffer, queryLen, 0);
        
        uint16_t respLenHeader;
        if (recv(sock, &respLenHeader, 2, 0) <= 0) { close(sock); return {}; }
        int expectedLen = ntohs(respLenHeader);
        
        int totalRead = 0;
        while (totalRead < expectedLen)
        {
            int r = recv(sock, buffer + totalRead, expectedLen - totalRead, 0);
            if (r <= 0) break;
            totalRead += r;
        }
        receivedLen = totalRead;
        close(sock);
    }
    else
    {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        struct timeval tv;
        tv.tv_sec = 2; tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sendto(sock, buffer, queryLen, 0, (struct sockaddr*)&servAddr, sizeof(servAddr));
        socklen_t addrLen = sizeof(servAddr);
        receivedLen = recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&servAddr, &addrLen);
        close(sock);

        if (receivedLen > 0)
        {
            DnsHeader* head = (DnsHeader*)buffer;
            if (ntohs(head->flags) & 0x0200)
            {
                return SendQuery(server, host, type, true);
            }
        }
    }

    if (receivedLen <= 0) return {};

    DnsHeader* head = (DnsHeader*)buffer;
    unsigned char* reader = buffer + sizeof(DnsHeader);

    for (int i = 0; i < ntohs(head->qCount); i++)
    {
        ParseName(buffer, &reader);
        reader += 4; 
    }

    vector<DnsResourceRecord> records;
    auto parseRecords = [&](int count)
    {
        for (int i = 0; i < count; i++)
        {
            DnsResourceRecord rr;
            rr.name = ParseName(buffer, &reader);
            rr.type = ntohs(*(uint16_t*)reader); reader += 2;
            rr.dnsClass = ntohs(*(uint16_t*)reader); reader += 2;
            rr.ttl = ntohl(*(uint32_t*)reader); reader += 4;
            rr.dataLen = ntohs(*(uint16_t*)reader); reader += 2;

            if (rr.type == TYPE_A)
            {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, reader, ip, INET_ADDRSTRLEN);
                rr.rdataStr = string(ip);
                reader += rr.dataLen;
            }
            else if (rr.type == TYPE_AAAA)
            {
                char ip[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, reader, ip, INET6_ADDRSTRLEN);
                rr.rdataStr = string(ip);
                reader += rr.dataLen;
            }
            else if (rr.type == TYPE_NS)
            {
                unsigned char* nsPtr = reader;
                rr.rdataStr = ParseName(buffer, &nsPtr);
                reader += rr.dataLen;
            }
            else
            {
                reader += rr.dataLen;
            }
            records.push_back(rr);
        }
    };

    parseRecords(ntohs(head->ansCount));
    parseRecords(ntohs(head->authCount));
    parseRecords(ntohs(head->addCount));

    return records;
}

string Resolve(string host, int type)
{
    string currentServer = ROOT_SERVER;
    LogDebug("Starting resolution for " + host + " from Root: " + currentServer);

    while (true)
    {
        vector<DnsResourceRecord> records = SendQuery(currentServer, host, type, false);
        if (records.empty()) return "Error: No response";

        for (auto& rr : records)
        {
            if (rr.type == type && rr.name == host)
            {
                return rr.rdataStr;
            }
        }

        string nextIp = "";
        for (auto& rr : records)
        {
            if (rr.type == TYPE_A && !rr.rdataStr.empty())
            {
                nextIp = rr.rdataStr;
                break;
            }
        }

        if (nextIp == "")
        {
            for (auto& rr : records)
            {
                if (rr.type == TYPE_NS)
                {
                    LogDebug("Resolving NS: " + rr.rdataStr);
                    nextIp = Resolve(rr.rdataStr, TYPE_A);
                    if (nextIp.find("Error") == string::npos) break;
                }
            }
        }

        if (nextIp == "" || nextIp.find("Error") != string::npos) return "Error: Host not found";
        
        LogDebug("Moving to: " + nextIp);
        currentServer = nextIp;
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        cout << "Usage: " << argv[0] << " <domain> <type> [-d]" << endl;
        return 1;
    }

    string host = argv[1];
    string typeStr = argv[2];
    int type = TYPE_A;
    if (typeStr == "AAAA") type = TYPE_AAAA;

    if (argc > 3 && string(argv[3]) == "-d") IsDebug = true;

    string result = Resolve(host, type);
    cout << result << endl;

    return 0;
}