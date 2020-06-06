#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctime>
#include <unistd.h>
#include "speaker.h"
#include "err.h"
#include "protocol.h"

Speaker::Speaker(const std::string& name, int timeout): name(name) {
    this->timeout = timeout;
    asProxy = false;
}

void Speaker::play(char* buff, size_t start, size_t len, bool isMeta) {
    //print(buff, start, len, isMeta);
    buffer = std::string(buff + start, len);
    isMetadata = isMeta;
}

void Speaker::print() {
    std::ostream& output = (isMetadata) ? std::cerr : std::cout;
    output << buffer;
}

void Speaker::connect(int port, std::string address) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        syserr("socket");
    }

    if(address.empty()) {
        listenBroadcast();
    }
    else {
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

void Speaker::listenBroadcast() {
    int optval = 1;

    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof(optval)) < 0) {
        syserr("setsockopt");
    }
}

void Speaker::listenMulticast(const std::string &address) {
    struct ip_mreq ip_mreq;
    ip_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if(inet_aton(address.c_str(), &ip_mreq.imr_multiaddr) == 0) {
        fatal("inet_aton - invalid multicast address\n");
    }

    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&ip_mreq, sizeof(ip_mreq)) < 0) {
        syserr("setsockopt");
    }
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

        //send
    }
}

void Speaker::handleSocketInput() {
    struct sockaddr income;
    socklen_t income_len = sizeof(income);
    uint16_t buffer[2];

    ssize_t len = recvfrom(sock, buffer, sizeof(buffer), 0, &income, &income_len);
    if(len < 0) {
        syserr("recvfrom");
    }

    protocol_type proto = getProtocolType(buffer, 2);
    switch (proto) {
        case DISCOVER:
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
    else if(ret != len) {
        fatal("partial sendto");
    }
}

void Speaker::setName(const std::string &name) {
    this->name = name;
}
