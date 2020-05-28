#include "radio.h"
#include "err.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <iostream>

Radio::Radio(std::string address, std::string port) {
    connectToRadio(address, port);
    metaint = 0;
}

void Radio::play(std::string resource, bool metadata) {
    sendRequest(resource, metadata);
    receiveHeader();

    bool work = true;
    bool readMetadata = metaint > 0;
    int toMeta = metaint, metaLen = 0;
    while(work) {
        ssize_t readLen = readStream();

        if(!readMetadata) {
            printAudio(0, readLen);
        }
        else {
            ssize_t pos = 0;

            while(pos < readLen) {
                // print metadata
                if(metaLen > 0) {
                    ssize_t len = std::min((ssize_t)metaLen, readLen - pos);
                    printMetadata(pos, len);

                    metaLen -= len;
                    pos += len;
                }
                // receive metadata length
                else if(toMeta == 0) {
                    metaLen = buffer[pos++] * 16;
                    toMeta = metaint;
                }
                // print audio
                else {
                    ssize_t len = std::min((ssize_t)toMeta, readLen - pos);
                    printAudio(pos, len);

                    pos += len;
                    toMeta -= len;
                }
            }
        }
    }
}

void Radio::connectToRadio(std::string address, std::string port) {
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
}

void Radio::sendRequest(std::string resource, bool metadata) {
    std::string request;
    request.append("GET ");
    request.append(resource);
    request.append(" HTTP/1.0\r\n");
    request.append("Icy-MetaData:");
    request.append((metadata) ? "1" : "0");
    request.append("\r\n");
    request.append("\r\n");

    int res = write(sock, request.c_str(), request.length());
    if(res != request.length()) {
        syserr("write failed");
    }

}

void Radio::receiveHeader() {
    FILE* stream = fdopen(sock, "r");

    char* line;
    size_t len  = 0;

    if(getline(&line, &len, stream) < 0) {
        syserr("no status response / read error");
    }
    parseStatus(line);
    free(line);

    bool readHeader = true;
    while(readHeader) {
        line = NULL;
        len = 0;
        if(getline(&line, &len, stream) <= 0) {
            syserr("read error");
        }

        if(parseHeaderLine(line)) {
            readHeader = false;
        }
        free(line);
    }
}

void Radio::parseStatus(std::string line) {
    size_t pos = line.find("200");
    if(pos == std::string::npos) {
        syserr("status no OK\nreceived status: %.s", line.length(), line.c_str());
    }
}

bool Radio::parseHeaderLine(std::string line) {
    if(line.compare("\r\n") == 0) {
        return true;
    }

    std::string delimiter = ":";
    size_t pos = 0;
    if((pos = line.find(delimiter)) != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1, line.length());

        //removing \r\n
        size_t len = value.length();
        value[len - 2] = value[len - 1] = '\0';

        if(key.compare("icy-metaint") == 0) {
            metaint = std::stoi(value);
        }
    }

    return false;
}

ssize_t Radio::readStream() {
    ssize_t len = read(sock, buffer, buffLen);
    if(len < 0) {
        syserr("read error");
    }

    return len;
}

void Radio::printAudio(ssize_t start, ssize_t len) {
    for(ssize_t i = start; i < len; i++) {
        std::cout << buffer[i];
    }
}

void Radio::printMetadata(ssize_t start, int len) {
    char* metadata = buffer + start;
    fprintf(stderr, "%.*s", len, metadata);
}
