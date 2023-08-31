// Library for generating a UUID4 on ESP32
// Mike Abbott - 2019
// MIT License

#ifndef UUID_GEN
#define UUID_GEN

#include "esp_random.h"

void GenerateUUIDv4(char *UUID) {
    uint8_t bytes[16];
    int i;
    for (i = 0; i < 16; i++) {
        bytes[i] = esp_random();
    }

    // Set version (4) and variant (2) fields
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    // Convert bytes to hexadecimal string representation
    sprintf(UUID, "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
            bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
};

#endif
