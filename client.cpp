#include "client.h"
#include "err.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <poll.h>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <iostream>
#include "protocol.h"

RadioClient::RadioClient(std::string address, int port, int telnetPort, int timeout) : telnet(telnetPort) {
    this->timeout = timeout * 1000;
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

    fd.fd = sock;
    fd.events = POLLIN;

    doWork = true;
    selected = -1;
    connected = false;
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
    sendDiscover((struct sockaddr*)&discoverAddr, sizeof(discoverAddr));

    proxies.clear();
    requireRefresh = true;
    doDiscovery = true;
    selected = -1;
}

void RadioClient::work() {
    int p;
    while(doWork) {
        requireRefresh = false;

        handleSockInput();
        client_action action = telnet.handleInput(&p);
        if(action == DO_DISCOVER) {
            discoverProxies();
        }
        else if(action == CONNECT) {
            connectToProxy(p);
        }
        else if(action == CLOSE) {
            stop();
            continue;
        }

        if(requireRefresh || action == REFRESH)
            telnet.printMenu(proxies, selected);
    }
}

void RadioClient::stop() {
    doWork = false;
    discoverProxies();
    telnet.stop();
    close(sock);
}

constexpr int keepAliveInterval = 3500;
void sendKeepAlive(bool& work, int fd, std::mutex& mutex, struct sockaddr addr) {
    uint16_t keepAlive[2];
    setHeader(keepAlive, 2, KEEPALIVE, 0);
    size_t len = sizeof(keepAlive);
    socklen_t addrLen = sizeof(addr);

    while(work) {
        mutex.lock();
        ssize_t sendLen = sendto(fd, keepAlive, len, 0, &addr, addrLen);
        mutex.unlock();

        if(sendLen < 0)
            syserr("sendto");
        else if((size_t)sendLen != len)
            fatal("partial send");

        usleep(keepAliveInterval);
    }
}

void RadioClient::connectToProxy(size_t i) {
    if(i >= proxies.size())
        return;
    disconnectProxy(false);

    selected = i;
    listenTo = proxies[selected].addr;
    connected = true;

    sendDiscover(&listenTo, sizeof(listenTo));

    keeperWork = true;
    keeper = std::thread(sendKeepAlive, std::ref(keeperWork), sock, std::ref(mutex), listenTo);
    timeoutStoper = 0;
    requireRefresh = true;
}

void RadioClient::disconnectProxy(bool remove) {
    if(!connected && !keeperWork)
        return;

    if(remove) {
        proxies.erase(proxies.begin() + selected);
    }

    telnet.setMetadata("");
    connected = false;
    keeperWork = false;
    keeper.join();
    selected = -1;
    requireRefresh = true;
}

void RadioClient::sendDiscover(struct sockaddr* sendAddress, socklen_t socklen) {
    uint32_t discover;
    setHeader((uint16_t *)&discover, 2, DISCOVER, 0);

    mutex.lock();
    int send_len = sendto(sock, &discover, sizeof(discover), 0, sendAddress, socklen);
    mutex.unlock();
    if(send_len < 0) {
        syserr("sendto");
    }
    else if(send_len != sizeof(discover)) {
        fatal("partial send");
    }
}

void RadioClient::handleSockInput() {
    fd.revents = 0;
    int waitFor = 100;
    int ret = poll(&fd, 1, waitFor);
    if(ret < 0) {
        syserr("poll");
    }
    else if(ret == 0) {
        if(connected) {
            timeoutStoper += waitFor;
            if(timeoutStoper > timeout) {
                disconnectProxy(true);
            }
        }

        return;
    }

    struct sockaddr income;
    const size_t buffLen = UINT16_MAX + 4;
    char buffor[buffLen];
    socklen_t addrLen = sizeof(income);

    mutex.lock();
    int recvLen = recvfrom(fd.fd, buffor, buffLen, 0, &income, &addrLen);
    mutex.unlock();
    if(recvLen < 0)
        syserr("recvfrom");

    protocol_type type = getProtocolType((uint16_t*)buffor, recvLen/2);
    if(doDiscovery && type == IAM) {
        RadioProxy proxy;
        proxy.addr = income;
        proxy.name = getData(buffor, recvLen);

        addProxy(proxy);
    }
    else if(connected && strncmp(income.sa_data, listenTo.sa_data,14) == 0) {
        switch (type) {
            case AUDIO: {
                timeoutStoper = 0;
                std::string audio = getData(buffor, recvLen);
                std::cout << audio;
            }
                break;
            case METADATA:
                timeoutStoper = 0;
                telnet.setMetadata(getData(buffor, recvLen));
                requireRefresh = true;
                break;
            default:
                break;
        }
    }
}

void RadioClient::addProxy(RadioProxy &proxy) {
    for(RadioProxy& p: proxies) {
        if(strncmp(p.addr.sa_data, proxy.addr.sa_data, 14) == 0)
            return;
    }

    if(connected && strncmp(listenTo.sa_data, proxy.addr.sa_data, 14) == 0) {
        selected = proxies.size();
    }
    proxies.push_back(proxy);
    requireRefresh = true;
}


