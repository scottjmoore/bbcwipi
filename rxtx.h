#ifndef __RXTX_H__
#define __RXTX_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"

#include "boards/pico_w.h"

#include "lwip/netif.h"

#include "debug.h"
#include "userport.h"


void rxtx_recv_mode();
void rxtx_send_mode();

void rxtx_send_byte(uint8_t tx_byte);
uint8_t rxtx_recv_byte();

//inline static uint8_t rxtx_burst_send_byte();
inline static uint8_t rxtx_burst_recv_byte() {
    while (gpio_get(USERPORT_CB2) == 0);
    while (gpio_get(USERPORT_CB2) == 1);

    uint8_t rx_byte = 
        gpio_get(USERPORT_PB0) |
        gpio_get(USERPORT_PB1) << 1 |
        gpio_get(USERPORT_PB2) << 2 |
        gpio_get(USERPORT_PB3) << 3 |
        gpio_get(USERPORT_PB4) << 4 |
        gpio_get(USERPORT_PB5) << 5 |
        gpio_get(USERPORT_PB6) << 6 |
        gpio_get(USERPORT_PB7) << 7;

    return rx_byte;
}

void rxtx_send_word(uint16_t tx_int);
uint16_t rxtx_recv_word();

void rxtx_send_dword(uint32_t tx_int);
uint32_t rxtx_recv_dword();

void rxtx_send_string(const uint8_t *tx_str);
void rxtx_recv_string(uint8_t *rx_str, int max_len);

void rxtx_send_block(const uint8_t *tx_block, int size);
void rxtx_recv_block(uint8_t *rx_block, int size);

void rxtx_send_ip(const ip_addr_t tx_ip);

#endif // __RXTX_H__