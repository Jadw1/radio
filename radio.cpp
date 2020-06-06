#include "radio.h"
#include "err.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <iostream>

Radio::Radio(const std::string& address, const std::string& port, int timeout) {
    connectToRadio(address, port);
    metaint = 0;
    this->timeout = timeout;
    this->work = true;
}

Radio::Radio(const std::string& address, const std::string& port, int timeout, int speakerPort, const std::string& multicast, int T) : Radio(address, port, timeout) {
    speaker.connect(speakerPort, T, multicast);
}

void Radio::play(const std::string& resource, bool metadata) {
    sendRequest(resource, metadata);
    receiveHeader();

    bool readMetadata = metaint > 0;
    size_t toMeta = metaint;
    while(work) {
        int ret = waitToRead();
        if(ret < 0) {
            break;
        }

        if(!readMetadata) {
            size_t readLen = readStream();
            speaker.play(buffer, 0, readLen, false);
        }
        else {
            if(toMeta > 0) {
                size_t len = std::min(toMeta, buffLen);
                size_t readLen = readStream(len);
                speaker.play(buffer, 0, readLen, false);

                toMeta -= readLen;
            }
            else {
                readStream(1);
                size_t metaLen = buffer[0] * 16;
                toMeta = metaint;

                if(metaLen == 0)
                    continue;
                readStream(metaLen);
                speaker.play(buffer, 0, metaLen, true);
            }
        }

        speaker.work();
    }

    disconnect();
}

void Radio::connectToRadio(const std::string& address, const std::string& port) {
    struct addrinfo addr_hints, *addr_result;

    memset(&addr_hints, 0, sizeof(struct addrinfo));
    addr_hints.ai_family = AF_INET;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(address.c_str(), port.c_str(), &addr_hints, &addr_result);
    if(err != 0) {
        if(err == EAI_SYSTEM)
            syserr("getaddrinfo: %s", gai_strerror(err));
        else
            fatal("getaddrinfo: %s", gai_strerror(err));
    }

    sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if(sock < 0)
        syserr("socket");

    if(connect(sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        syserr("connect");
    freeaddrinfo(addr_result);

    fd.fd = sock;
    fd.events = POLLIN;
    fd.revents = 0;
}

void Radio::sendRequest(const std::string& resource, bool metadata) const {
    std::string request;
    request.append("GET ");
    request.append(resource);
    request.append(" HTTP/1.0\r\n");
    request.append("Icy-MetaData:");
    request.append((metadata) ? "1" : "0");
    request.append("\r\n");
    request.append("\r\n");

    ssize_t res = write(sock, request.c_str(), request.length());
    if(res < 0) {
        syserr("write");
    }
    else if((size_t)res != request.length()) {
        fatal("partial write");
    }

}

void Radio::receiveHeader() {
    FILE* stream = fdopen(sock, "r");

    char* line;
    size_t len  = 0;

    int ret = waitToRead();
    if(ret < 0) {
        disconnect();
        return;
    }
    if(getline(&line, &len, stream) < 0) {
        syserr("no status response / read error");
    }
    parseStatus(line);
    free(line);

    bool readHeader = true;
    while(readHeader) {
        line = nullptr;
        len = 0;
        ret = waitToRead();
        if(ret < 0) {
            disconnect();
            return;
        }
        if(getline(&line, &len, stream) <= 0) {
            syserr("read error");
        }

        if(parseHeaderLine(line)) {
            readHeader = false;
        }
        free(line);
    }
}

void Radio::parseStatus(const std::string& line) {
    size_t pos = line.find("200");
    if(pos == std::string::npos) {
        syserr("status no OK\nreceived status: %.s", line.length(), line.c_str());
    }
}

bool Radio::parseHeaderLine(const std::string& line) {
    if(line == "\r\n") {
        return true;
    }

    std::string delimiter = ":";
    size_t pos;
    if((pos = line.find(delimiter)) != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1, line.length());

        /*
        //removing \r\n
        size_t len = value.length();
        value[len - 2] = value[len - 1] = '\0';
         */
        value.pop_back();
        value.pop_back();

        if(key == "icy-metaint") {
            metaint = std::stoi(value);
        }
        else if(key == "icy-name") {
            name = value;
            speaker.setName(name);
        }
    }

    return false;
}

size_t Radio::readStream(size_t len) {
    ssize_t readLen = read(sock, buffer, len);
    if(readLen < 0) {
        syserr("read error");
    }

    return readLen;
}

int Radio::waitToRead() {
    int ret = poll(&fd, 1, timeout * 1000);
    if(ret == 0) {
        std::cerr << "timeout\n";
        return -1;
    }
    else if(ret < 0 && errno == EINTR) {
        return -2;
    }
    else if(ret < 0) {
        syserr("poll error");
    }
    return 0;
}

void Radio::disconnect() {
    speaker.disconnect();
    close(sock);
}

void Radio::stopPlaying() {
    work = false;
}
