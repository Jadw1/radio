#include <iostream>
#include <signal.h>
#include <getopt.h>
#include "client.h"
#include "err.h"

RadioClient* client = nullptr;

void printUsage() {
    std::cerr << "niechcemisie\n";
    exit(0);
}

void stopClient(int sig) {
    if(client) {
        client->stop();
    }
}

int main(int argc, char* argv[]) {
    std::string host;
    int port = -1, telnet = -1;
    int timeout = 5, opt;
    sigset_t blockMask;
    struct sigaction action;

    sigemptyset(&blockMask);
    action.sa_handler = stopClient;
    action.sa_mask = blockMask;
    action.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &action, 0) == -1) {
        syserr("sigaction");
    }

    while((opt = getopt(argc, argv, "H:P:p:T:")) !=  -1) {
        switch (opt) {
            case 'H':
                host = optarg;
                break;
            case 'P':
                port = std::stoi(optarg);
                break;
            case 'p':
                telnet = std::stoi(optarg);
                break;
            case 'T':
                timeout = std::stoi(optarg);
                break;
            default:
                printUsage();
        }
    }

    if(host.empty() || port < 0 || telnet < 0 || timeout <= 0) {
        printUsage();
    }

    RadioClient tmp(host, port, telnet, timeout);
    client = &tmp;
    client->work();
}
