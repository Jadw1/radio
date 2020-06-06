#ifndef RADIO_TELNET_CONTROLLER_H
#define RADIO_TELNET_CONTROLLER_H

#include <poll.h>
#include <string>
#include <vector>
#include <netinet/in.h>

enum work_mode {
    LISTEN,
    WORK
};

enum key_pressed {
    UP,
    DOWN,
    ENTER,
    OTHER
};

enum client_action {
    DO_DISCOVER,
    CONNECT,
    REFRESH,
    NONE
};

struct RadioProxy {
    std::string name;
    struct sockaddr addr;
};

class TelnetController {
public:
    TelnetController(int port);

    client_action handleInput(int* p);
    void printMenu(const std::vector<RadioProxy>& proxies);
    void setSelected(int i);

private:
    int port;
    int sock, msg_sock;
    work_mode mode;
    struct pollfd fd, msg_fd;
    int timeout = 10;
    int pos, maxPos;
    int selected;

    client_action listenToConnect();
    client_action handle();
    void connectionLost();

    key_pressed parseInput(char* input, size_t len);
    std::string constructMenu(const std::vector<RadioProxy>& proxies);
};

#endif //RADIO_TELNET_CONTROLLER_H
