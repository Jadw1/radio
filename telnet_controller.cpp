#include <netinet/in.h>
#include <unistd.h>
#include "telnet_controller.h"
#include "err.h"

TelnetController::TelnetController(int port) {
    this->port = port;
    pos = 0;
    maxPos = 2;
    selected = -1;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        syserr("socket");

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&server_address, sizeof server_address) < 0)
        syserr("bind");

    if(listen(sock, 1) < 0) {
        syserr("listen");
    }

    fd.fd = sock;
    fd.events = POLLIN;
    fd.revents = 0;

    mode = LISTEN;
}

client_action TelnetController::handleInput(int* p) {
    client_action action = NONE;

    switch (mode) {
        case LISTEN:
            action = listenToConnect();
            break;
        case WORK:
            action = handle();
            break;
    }

    *p = pos - 1;
    return action;
}

client_action TelnetController::listenToConnect() {
    fd.revents = 0;
    int ret = poll(&fd, 1, timeout);
    if(ret < 0) {
        syserr("poll");
    }
    else if(ret == 0) {
        return NONE;
    }

    struct sockaddr_in client_address;
    socklen_t client_len = sizeof(client_address);

    msg_sock = accept(fd.fd, (struct sockaddr *) &client_address, &client_len);
    if(msg_sock < 0) {
        syserr("accept");
    }

    //set telnet options
    static const char options[] = "\377\375\042\377\373\001";
    int write_len = write(msg_sock, options, sizeof(options));
    if (write_len < 0) {
        syserr("write");
    }
    else if(write_len != sizeof(options)) {
        fatal("partial write");
    }

    //receive telnet response and ignore it
    const int buffSize = 1000;
    char buff[buffSize];
    int read_len = read(msg_sock, buff, buffSize);
    if(read_len < 0) {
        syserr("read");
    }

    msg_fd.fd = msg_sock;
    msg_fd.events = POLLIN;
    msg_fd.revents = 0;

    pos = 0;
    mode = WORK;

    return REFRESH;
}

client_action TelnetController::handle() {
    msg_fd.revents = 0;
    int ret = poll(&msg_fd, 1, timeout);
    if(ret < 0) {
        syserr("poll");
    }
    else if(ret == 0) {
        return NONE;
    }

    const int buffLen = 5;
    char buff[buffLen];
    int read_len = read(msg_fd.fd, buff, buffLen);
    if(read_len < 0) {
        syserr("read");
    }
    else if(read_len == 0) {
        connectionLost();
        return NONE;
    }

    client_action action = NONE;
    switch (parseInput(buff, read_len)) {
        case UP:
            if(pos > 0) {
                action = REFRESH;
                pos--;
            }
            break;
        case DOWN:
            if(pos < maxPos - 1) {
                action = REFRESH;
                pos++;
            }
            break;
        case ENTER:
            if(pos == 0)
                action = DO_DISCOVER;
            else if(pos > 0 && pos < maxPos - 1)
                action = CONNECT;
            break;
        case OTHER:
            break;
    }
    return action;
}

void TelnetController::connectionLost() {
    msg_sock = -1;
    msg_fd.fd = -1;
    mode = LISTEN;
}

key_pressed TelnetController::parseInput(char *input, size_t len) {
    if(len == 2 && input[0] == 13 && input[1] == 0)
        return ENTER;
    else if(len == 3) {
        if(input[0] != 27 && input[1] != 91)
            return OTHER;

        else if(input[2] == 65)
            return UP;
        else if(input[2] == 66)
            return DOWN;

        return OTHER;
    }
    return OTHER;
}

std::string TelnetController::constructMenu(const std::vector<RadioProxy>& proxies) {
    static const std::string clear = "\u001Bc";
    static const std::string newLine = "\r\n";
    static const std::string selector = "> ";
    static const std::string search = "Szukaj pośrednika";
    static const std::string end = "Koniec";

    maxPos = proxies.size() + 2;

    std::string menu;
    menu.append(clear);
    for(int i=0; i < maxPos; i++) {
        if(i == pos) {
            menu.append(selector);
        }

        if(i == 0) {
            menu.append(search);
        }
        else if(i == maxPos - 1) {
            menu.append(end);
        }
        else {
            menu.append(proxies[i-1].name);
            if(selected == i-1) {
                menu.append(" *");
            }
        }

        menu.append(newLine);
    }

    return menu;
}

void TelnetController::printMenu(const std::vector<RadioProxy>& proxies) {
    if(mode == LISTEN) {
        return;
    }

    std::string response = constructMenu(proxies);
    int write_len = write(msg_fd.fd, response.c_str(), response.length());
    if(write_len < 0) {
        syserr("write");
    }
    else if(write_len != response.length()) {
        fatal("partial write");
    }
}

void TelnetController::setSelected(int i) {
    selected = i;
}
