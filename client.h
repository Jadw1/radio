#ifndef RADIO_CLIENT_H
#define RADIO_CLIENT_H

#include <string>
#include <netinet/in.h>
#include <vector>
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
    struct sockaddr_in discoverAddr;
    bool doWork;
    std::vector<RadioProxy> proxies;

    void setSocketOptions();
    void discoverProxies();
};

#endif //RADIO_CLIENT_H
