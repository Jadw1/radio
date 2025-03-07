#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <unistd.h>
#include "speaker.h"
#include "err.h"
#include "protocol.h"

Speaker::Speaker() {
    asProxy = false;
}

void Speaker::play(char* buff, size_t start, size_t len, bool isMeta) {
    buffer = std::string(buff + start, len);
    isMetadata = isMeta;
}

void Speaker::print() {
    std::ostream& output = (isMetadata) ? std::cerr : std::cout;
    output << buffer;
}

void Speaker::connect(int port, int timeout, std::string address) {
    this->timeout = timeout;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        syserr("socket");
    }

    if(!address.empty()) {
        listenMulticast(address);
    }

    struct sockaddr_in local_address;
    local_address.sin_family = AF_INET;
    local_address.sin_addr.s_addr = htonl(INADDR_ANY);
    local_address.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&local_address, sizeof local_address) < 0)
        syserr("bind");

    fd.fd = sock;
    fd.events = POLLIN;
    asProxy = true;
}

void Speaker::listenMulticast(const std::string &address) {
    ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if(inet_aton(address.c_str(), &ip_mreq.imr_multiaddr) == 0) {
        fatal("inet_aton - invalid multicast address\n");
    }

    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ip_mreq, sizeof(ip_mreq)) < 0) {
        syserr("setsockopt");
    }

    multicast = true;
}

void Speaker::work() {
    if(!asProxy) {
        print();
        return;
    }

    fd.revents = 0;
    int ret = poll(&fd, 1, 500);
    if(ret < -1) { //ignoring interupt
        syserr("poll");
    }
    else if(ret > 0) {
        handleSocketInput();
    }

    time_t currentTime;
    time(&currentTime);
    for(auto it = timeMap.begin(); it != timeMap.end(); it++) {
        if(currentTime - it->second > timeout) {
            timeMap.erase(it);
            continue;
        }

        std::string msg;
        char header[4];
        if(isMetadata) {
            setHeader((uint16_t*)&header, 2, METADATA, (uint16_t)buffer.size());
        }
        else {
            setHeader((uint16_t*)&header, 2, AUDIO, (uint16_t)buffer.size());
        }
        msg.append(header, 4);
        msg.append(buffer);

        ssize_t len = sendto(sock, msg.c_str(), msg.length(), 0, &it->first, sizeof(it->first));
        if(len < 0)
            syserr("sendto");
        else if((size_t)len != msg.length())
            fatal("partial send");
    }
}

void Speaker::handleSocketInput() {
    struct sockaddr income;
    socklen_t income_len = sizeof(income);
    const size_t buffLen = UINT16_MAX + 4;
    char buffer[buffLen];

    ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, &income, &income_len);
    if(len < 0) {
        syserr("recvfrom");
    }

    protocol_type proto = getProtocolType((uint16_t*)buffer, len/2);
    switch (proto) {
        case DISCOVER:
            addClient(income);
            sendIam(income);
            break;
        case KEEPALIVE:
            keepAlive(income);
            break;
        default:
            break;
    }
}

void Speaker::keepAlive(const struct sockaddr& addr) {
    auto it = timeMap.find(addr);
    if(it == timeMap.end()) {
        return;
    }

    time_t currentTime;
    time(&currentTime);
    it->second = currentTime;
}

void Speaker::addClient(const sockaddr& addr) {
    auto it = timeMap.find(addr);
    time_t currentTime;
    time(&currentTime);

    if(it == timeMap.end()) {
        timeMap.insert(std::pair<struct sockaddr, time_t>(addr, currentTime));
    }
    else {
        it->second = currentTime;
    }
}

void Speaker::sendIam(const sockaddr &addr) {
    char msg[200];
    setHeader((uint16_t*)msg, 200 / 2, IAM, name.length());

    size_t len = 4 + name.length();
    strncpy(msg+4, name.c_str(), name.length());

    ssize_t ret = sendto(sock, msg, len, 0, &addr, sizeof(addr));
    if(ret < 0) {
        syserr("sendto");
    }
    else if((size_t)ret != len) {
        fatal("partial sendto");
    }
}

void Speaker::setName(const std::string &name) {
    this->name = name;
}

void Speaker::disconnect() {
    if(asProxy) {
        if(multicast) {
            if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (void*)&ip_mreq, sizeof ip_mreq) < 0) {
                syserr("setsockopt");
            }
        }
        close(sock);
    }
}
