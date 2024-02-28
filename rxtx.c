#include "rxtx.h"

void rxtx_send_byte(char tx_byte) {
        while (gpio_get(USERPORT_CB2) == 0);

        gpio_put(USERPORT_PB0, tx_byte & 1);
        gpio_put(USERPORT_PB1, tx_byte & 2);
        gpio_put(USERPORT_PB2, tx_byte & 4);
        gpio_put(USERPORT_PB3, tx_byte & 8);
        gpio_put(USERPORT_PB4, tx_byte & 16);
        gpio_put(USERPORT_PB5, tx_byte & 32);
        gpio_put(USERPORT_PB6, tx_byte & 64);
        gpio_put(USERPORT_PB7, tx_byte & 128);

        gpio_put(USERPORT_CB1,0);
        sleep_us(1);
        gpio_put(USERPORT_CB1,1);
        sleep_us(1);
        gpio_put(USERPORT_CB1,0);
        sleep_us(1);
        gpio_put(USERPORT_CB1,1);

        while (gpio_get(USERPORT_CB2) == 1);
}

void rxtx_send_string(const char *tx_str) {
    while (*tx_str) {
        rxtx_send_byte(*tx_str);
        tx_str++;
    }
}