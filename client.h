#ifndef RADIO_CLIENT_H
#define RADIO_CLIENT_H

#include <string>
#include <netinet/in.h>
#include <vector>
#include <thread>
#include <mutex>
#include "telnet_controller.h"

class TelnetController;

class RadioClient {
public:
    RadioClient(std::string address, int port, int telnetPort, int timeout);
    void work();
    void stop();



private:
    TelnetController telnet;
    int sock;
    struct pollfd fd;
    int timeout;
    struct sockaddr_in discoverAddr;
    bool doWork;
    std::vector<RadioProxy> proxies;
    int selected;
    int timeoutStoper;

    std::thread keeper;
    bool keeperWork;
    std::mutex mutex;
    struct sockaddr listenTo;

    void sendDiscover(struct sockaddr* sendAddress, socklen_t socklen);
    void setSocketOptions();
    void discoverProxies();
    void connectToProxy(int i);
    void disconnectProxy();
    void handleSockInput();
};

#endif //RADIO_CLIENT_H
