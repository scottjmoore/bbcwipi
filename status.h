#ifndef __STATUS_H__
#define __STATUS_H__

#include <stdbool.h>
#include <string.h>

#include "lwip/netif.h"

typedef struct wipi_status_t {
    bool is_connected;
    int32_t rssi;
    uint8_t channel;
    uint8_t mac[6];
    ip_addr_t ip;
    ip_addr_t gateway;
    ip_addr_t netmask;
    char ssid[32];
    char *hostname;
} wipi_status_t;

wipi_status_t wipi_status_init();

#endif //  __STATUS_H__
