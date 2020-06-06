#ifndef RADIO_RADIO_H
#define RADIO_RADIO_H

#include <string>
#include <poll.h>
#include "speaker.h"

constexpr size_t buffLen = 60000;

class Radio {
public:
    Radio(const std::string& address, const std::string& port, int timeout);
    void play(const std::string& resource, bool metadata);
    void stopPlaying();

private:
    Speaker speaker;
    int sock;
    int metaint;
    int timeout;
    bool work;
    std::string name;

    struct pollfd fd;

    /* buffLen must be at least 4080 */
    char buffer[buffLen];

    void connectToRadio(const std::string& address, const std::string& port);
    void sendRequest(const std::string& resource, bool metadata) const;
    void receiveHeader();
    static void parseStatus(const std::string& line);
    bool parseHeaderLine(const std::string& line);
    size_t readStream(size_t len = buffLen);
    void printAudio(size_t start, size_t len);
    void printMetadata(size_t start, int len);
    int waitToRead();
    void disconnect() const;
};

#endif //RADIO_RADIO_H