#ifndef __COMMANDS_H__
#define __COMMANDS_H__

/*

===============================================================================
0x00 - Interface status
===============================================================================
On call
-------------------------------------------------------------------------------
    Send bytes          : 0     4
    Receive bytes       : 1     54
    Command             : 2     0x00
    Sub command         : 3     0x00
-------------------------------------------------------------------------------
On return
-------------------------------------------------------------------------------
    Status              : 4
    RSSI                : 5-8
    Channel             : 9
    Interface IP        : 10-13
    Interface gateway   : 14-17
    Interface netmask   : 18-21
    SSID                : 22-54
===============================================================================

===============================================================================
0x01 - Connect
===============================================================================
On call
-------------------------------------------------------------------------------
    Send bytes          : 0     100
    Receive bytes       : 1     5
    Command             : 2     0x01
    Sub command         : 3     0x00
    SSID                : 4-35
    Password            : 36-99
    Timeout             : 100
-------------------------------------------------------------------------------
On return
-------------------------------------------------------------------------------
    Return code         : 3
===============================================================================

===============================================================================
0x02 - Disconnect
===============================================================================
On call
-------------------------------------------------------------------------------
    Send bytes          : 0     4
    Receive bytes       : 1     5
    Command             : 2     0x02
    Sub command         : 3     0x00
-------------------------------------------------------------------------------
On return
-------------------------------------------------------------------------------
    Return code         : 3
===============================================================================

===============================================================================
0x10 - Send ICMP
===============================================================================
On call
-------------------------------------------------------------------------------
    Send bytes          : 0     16
    Receive bytes       : 1     4
    Command             : 2     0x10
    Sub command         : 3     0x00
    Destination IP      : 4-7
    Type                : 8
    Code                : 9
    Packet size         : 10-11
    Packet data address : 12-15
-------------------------------------------------------------------------------
On return
-------------------------------------------------------------------------------
    Return code         : 3
===============================================================================

===============================================================================
0x11 - Receive ICMP
===============================================================================
On call
-------------------------------------------------------------------------------
    Send bytes          : 0     8
    Receive bytes       : 1     12
    Command             : 2     0x11
    Sub command         : 3     0x00
    Packet data address : 4-7
-------------------------------------------------------------------------------
On return
-------------------------------------------------------------------------------
    Source IP           : 4-7
    Type                : 8
    Code                : 9
    Packet size         : 10-11
===============================================================================

*/

static const uint8_t command_wipi_status = 0x00;
static const uint8_t command_connect =0x01;
static const uint8_t command_disconnect = 0x02;
static const uint8_t command_send_icmp = 0x10;
static const uint8_t command_receive_icmp = 0x11;
static const uint8_t command_send_udp = 0x20;
static const uint8_t command_send_tcp = 0x30;

typedef struct {
    uint8_t result;
} command_result_t, *command_result_ptr;

typedef struct {
    uint8_t connected;
    int32_t rssi;
    uint8_t channel;
    uint8_t mac[6];
    ip_addr_t ip;
    ip_addr_t gateway;
    ip_addr_t netmask;
    char ssid[32];
} command_status_data_t, *command_status_data_ptr;

typedef struct {
    uint8_t     ssid[32];
    uint8_t     password[64];
    uint8_t     timeout;
} command_connect_data_t, *command_connect_data_ptr;

typedef struct {
    uint32_t    destination_ip;
    uint8_t     type;
    uint8_t     code;
    uint16_t    packet_size;
    uint8_t     packet_data[1024];
} command_send_icmp_data_t, *command_send_icmp_data_ptr;

typedef struct {
    uint32_t    source_ip;
    uint8_t     type;
    uint8_t     code;
    uint16_t    packet_size;
    uint8_t     packet_data[1024];
} command_receive_icmp_data_t, *command_receive_icmp_data_ptr;

#endif //  __COMMANDS_H__

