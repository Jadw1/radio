#ifndef RADIO_SPEAKER_H
#define RADIO_SPEAKER_H

#include <string>
#include <climits>
#include <poll.h>
#include <map>
#include <cstring>
#include <netinet/in.h>

struct sockaddrComp {
    bool operator()(const struct sockaddr& ad1, const struct sockaddr& ad2) const {
        int result = strncmp(ad1.sa_data, ad2.sa_data, 14);
        return result > 0;
    }
};

class Speaker {
public:
    Speaker();
    void connect(int port, int timeout, std::string address);

    void setName(const std::string& name);
    void play(char* buff, size_t start, size_t len, bool isMeta);
    void work();

private:
    bool asProxy;
    std::string name;
    int sock;
    int timeout;
    struct pollfd fd;
    std::map<struct sockaddr, time_t, sockaddrComp> timeMap;

    std::string buffer;
    bool isMetadata;

    void print();
    void listenBroadcast();
    void listenMulticast(const std::string& address);
    void handleSocketInput();

    void keepAlive(const struct sockaddr& addr);
    void addClient(const struct sockaddr& addr);
    void sendIam(const struct sockaddr& addr);
};

#endif //RADIO_SPEAKER_H
