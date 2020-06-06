#include <iostream>
#include "radio.h"
#include <getopt.h>
#include <cstring>
#include <signal.h>
#include "err.h"
#include <climits>

Radio* radio = nullptr;

void printUsage() {
    std::cerr << "niechcemisie\n";
    exit(0);
}

void stopRadio(int sig) {
    if(radio) {
        radio->stopPlaying();
    }
}

int main(int argc, char* argv[]) {
    std::string host, resource, port;
    int timeout = 5, opt;
    bool metadata = false;
    sigset_t blockMask;
    struct sigaction action;

    sigemptyset(&blockMask);
    action.sa_handler = stopRadio;
    action.sa_mask = blockMask;
    action.sa_flags = SA_RESTART;

    if(sigaction(SIGINT, &action, 0) == -1) {
        syserr("sigaction");
    }

    while((opt = getopt(argc, argv, "h:r:p:m:t:")) !=  -1) {
        switch (opt) {
            case 'h':
                host = optarg;
                break;
            case 'r':
                resource = optarg;
                break;
            case 'p':
                port = optarg;
                break;
            case 'm':
                if(strcmp(optarg, "yes") == 0) {
                    metadata = true;
                }
                else if(strcmp(optarg, "no") == 0) {
                    metadata = false;
                }
                else {
                    printUsage();
                }
                break;
            case 't':
                timeout = std::stoi(optarg);
                break;
            default:
                printUsage();
        }
    }

    if(optind != argc || host.empty() || resource.empty() || port.empty() || timeout <= 0) {
        printUsage();
    }

    Radio tmp(host, port, timeout);
    radio = &tmp;
    radio->play(resource, metadata);

    //Speaker speaker;
    //speaker.test("239.10.11.12", 11000);

}
