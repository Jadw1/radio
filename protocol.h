#ifndef RADIO_PROTOCOL_H
#define RADIO_PROTOCOL_H

#include <cstdint>
#include <string>

enum protocol_type {
    ERROR = 0,
    DISCOVER = 1,
    IAM = 2,
    KEEPALIVE = 3,
    AUDIO = 4,
    METADATA = 6
};

/* len in uint16 blocks (1 len = 2 bytes) */
protocol_type getProtocolType(uint16_t* buffer, size_t len);
/* len in uint16 blocks (1 len = 2 bytes) */
void setHeader(uint16_t* buffer, size_t len, protocol_type type, uint16_t dataLen);
/* len in char blocks (1 len = 1 byte) */
std::string getData(char* buffer, size_t len);

#endif //RADIO_PROTOCOL_H
