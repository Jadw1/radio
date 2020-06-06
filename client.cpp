#include "client.h"
#include "err.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <cstring>
#include "protocol.h"

RadioClient::RadioClient(std::string address, int port, int telnetPort, int timeout) : telnet(telnetPort) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        syserr("socket");
    }

    setSocketOptions();

    struct addrinfo hints;
    struct addrinfo *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    if(getaddrinfo(address.c_str(), NULL, &hints, &result) != 0) {
        syserr("getaddrinfo");
    }

    discoverAddr.sin_family = AF_INET;
    discoverAddr.sin_addr.s_addr = ((struct sockaddr_in*) (result->ai_addr))->sin_addr.s_addr;
    discoverAddr.sin_port = htons(port);
    freeaddrinfo(result);

    doWork = true;
}

void RadioClient::setSocketOptions() {
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof optval) < 0)
        syserr("setsockopt broadcast");

    optval = 4;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&optval, sizeof optval) < 0)
        syserr("setsockopt multicast ttl");

    optval = 0;
    if(setsockopt(sock, SOL_IP, IP_MULTICAST_LOOP, (void*)&optval, sizeof(optval)) < 0)
        syserr("setsockopt multicast loop");
}

void RadioClient::discoverProxies() {
    uint32_t discover;
    setHeader((uint16_t *)&discover, 2, DISCOVER, 0);

    int send_len = sendto(sock, &discover, sizeof(discover), 0, (struct sockaddr *)&discoverAddr, sizeof(discoverAddr));
    if(send_len < 0) {
        syserr("sendto");
    }
    else if(send_len != sizeof(discover)) {
        fatal("partial send");
    }

    proxies.clear();
    struct pollfd fd;
    fd.fd = sock;
    fd.events = POLLIN;
    bool search = true;

    while(search) {
        int ret = poll(&fd, 1, 1000);
        if(ret < 0) {
            syserr("poll");
        }
        else if(ret == 0) {
            search = false;
            continue;
        }

        RadioProxy proxy;
        const size_t buffLen = UINT16_MAX + 4;
        char buffor[buffLen];
        socklen_t addrLen = sizeof(proxy.addr);

        int recvLen = recvfrom(fd.fd, buffor, buffLen, 0, &proxy.addr, &addrLen);
        if(recvLen < 0)
            syserr("recvfrom");

        if(getProtocolType((uint16_t*)buffor, recvLen/2) != IAM)
            continue;

        proxy.name = getData(buffor, recvLen);
        proxies.push_back(proxy);
    }


}

void RadioClient::work() {
    while(doWork) {
        client_action action = telnet.handleInput();
        if(action == DO_DISCOVER) {
            discoverProxies();
        }
        telnet.printMenu(proxies);
    }
}

void RadioClient::stop() {
    doWork = false;
}


