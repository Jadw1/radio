#ifndef RADIO_RADIO_H
#define RADIO_RADIO_H

#include <string>

class Radio {
public:
    Radio(std::string address, std::string port);
    void play(std::string resource, bool metadata);

private:
    int sock;
    int metaint;

    const static int buffLen = 60000;
    char buffer[buffLen];

    void connectToRadio(std::string address, std::string port);
    void sendRequest(std::string resource, bool metadata);
    void receiveHeader();
    void parseStatus(std::string line);
    bool parseHeaderLine(std::string line);
    ssize_t readStream();
    void printAudio(ssize_t start, ssize_t len);
    void printMetadata(ssize_t start, int len);

};

#endif //RADIO_RADIO_H