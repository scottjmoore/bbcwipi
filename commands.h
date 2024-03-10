#ifndef __COMMANDS_H__
#define __COMMANDS_H__

static const uint8_t command_wipi_status = 0x00;
static const uint8_t command_connect =0x01;
static const uint8_t command_disconnect = 0x02;

typedef struct {
    uint8_t result;
} command_result_t;

typedef struct {
    uint8_t ssid[32];
    uint8_t password[64];
    uint8_t timeout;
} command_connect_data_t;

#endif //  __COMMANDS_H__

