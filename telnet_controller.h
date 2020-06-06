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
    CLOSE,
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
    void printMenu(const std::vector<RadioProxy>& proxies, int selected);
    void setMetadata(const std::string& meta);
    void stop();

private:
    int sock, msg_sock;
    work_mode mode;
    struct pollfd fd, msg_fd;
    const int timeout = 10;
    int pos, maxPos;
    std::string metadata;

    client_action listenToConnect();
    client_action handle();
    void connectionLost();

    key_pressed parseInput(char* input, size_t len);
    std::string constructMenu(const std::vector<RadioProxy>& proxies, int selected);
};

#endif //RADIO_TELNET_CONTROLLER_H
