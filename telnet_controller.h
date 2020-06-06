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
    NONE
};

struct RadioProxy {
    std::string name;
    struct sockaddr addr;
};

class TelnetController {
public:
    TelnetController(int port);

    client_action handleInput();
    void printMenu(const std::vector<RadioProxy>& proxies);

private:
    int port;
    int sock, msg_sock;
    work_mode mode;
    struct pollfd fd, msg_fd;
    int timeout = 500;
    int pos, maxPos;

    void listenToConnect();
    client_action handle();
    void connectionLost();

    key_pressed parseInput(char* input, size_t len);
    std::string constructMenu(const std::vector<RadioProxy>& proxies);
};

#endif //RADIO_TELNET_CONTROLLER_H
