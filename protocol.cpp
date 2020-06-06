#include "protocol.h"
#include <arpa/inet.h>

protocol_type getProtocolType(uint16_t* buffer, size_t len) {
    if(len < 1) {
        return ERROR;
    }

    uint16_t proto = ntohs(buffer[0]);
    protocol_type protoType;
    switch (proto) {
        case DISCOVER:
            protoType = DISCOVER;
            break;
        case IAM:
            protoType = IAM;
            break;
        case KEEPALIVE:
            protoType = KEEPALIVE;
            break;
        case AUDIO:
            protoType = AUDIO;
            break;
        case METADATA:
            protoType = METADATA;
            break;
        default:
            protoType = ERROR;
            break;
    }

    return protoType;
}

void setHeader(uint16_t* buffer, size_t len, protocol_type type, uint16_t dataLen) {
    if(len < 2) {
        return;
    }

    buffer[0] = htons(type);
    buffer[1] = htons(dataLen);
}

std::string getData(char* buffer, size_t len) {
    if(len < 4) {
        return "ERROR";
    }

    uint16_t* tmp = (uint16_t*)buffer;
    size_t dataLen = (size_t)ntohs(tmp[1]);

    if(dataLen + 4 != len) {
        return "ERROR";
    }

    return std::string(buffer + 4, dataLen);
}