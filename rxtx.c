#include "rxtx.h"

static inline void rxtx_send_ack() {
    gpio_put(USERPORT_CB1,0);
    sleep_us(2);
    gpio_put(USERPORT_CB1,1);
    sleep_us(1);
    gpio_put(USERPORT_CB1,0);
    sleep_us(2);
    gpio_put(USERPORT_CB1,1);
}
void rxtx_recv_mode() {
    debug(DEBUG_INFO, "rxtx_recv_mode()\n");

    while (gpio_get(USERPORT_CB2) == 0);
    gpio_put(10, 1);
    userport_set_dir(GPIO_IN);
}

void rxtx_send_mode() {
    debug(DEBUG_INFO, "rxtx_send_mode()\n");

    while (gpio_get(USERPORT_CB2) == 1);
    gpio_put(10, 0);
    userport_set_dir(GPIO_OUT);
}

void rxtx_send_byte(char tx_byte) {
    //debug(DEBUG_INFO, "rxtx_send_byte(char tx_byte = %02x)\n", tx_byte);

    //(DEBUG_INFO, "Waiting for CB2 to be high\n");
    while (gpio_get(USERPORT_CB2) == 0);

    //debug(DEBUG_INFO, "Sending byte %02x\n", tx_byte);
    gpio_put(USERPORT_PB0, tx_byte & 1);
    gpio_put(USERPORT_PB1, tx_byte & 2);
    gpio_put(USERPORT_PB2, tx_byte & 4);
    gpio_put(USERPORT_PB3, tx_byte & 8);
    gpio_put(USERPORT_PB4, tx_byte & 16);
    gpio_put(USERPORT_PB5, tx_byte & 32);
    gpio_put(USERPORT_PB6, tx_byte & 64);
    gpio_put(USERPORT_PB7, tx_byte & 128);

    //debug(DEBUG_INFO, "Waiting for CB2 to be low\n");
    
    rxtx_send_ack();
    while (gpio_get(USERPORT_CB2) == 1);
}

char rxtx_recv_byte() {
    while (gpio_get(USERPORT_CB2) == 1);

    char rx_byte = 
        gpio_get(USERPORT_PB0) |
        gpio_get(USERPORT_PB1) << 1 |
        gpio_get(USERPORT_PB2) << 2 |
        gpio_get(USERPORT_PB3) << 3 |
        gpio_get(USERPORT_PB4) << 4 |
        gpio_get(USERPORT_PB5) << 5 |
        gpio_get(USERPORT_PB6) << 6 |
        gpio_get(USERPORT_PB7) << 7;

    rxtx_send_ack();
    while (gpio_get(USERPORT_CB2) == 0);

    return rx_byte;
}

void rxtx_send_string(const char *tx_str) {
    debug(DEBUG_INFO, "rxtx_send_string(const char *tx_str = '%s')\n",tx_str);

    while (*tx_str) {
        rxtx_send_byte(*tx_str);
        tx_str++;
    }
}

void rxtx_recv_string(char *rx_str, int max_len) {
    int i = 0;
    while (i < max_len) {
        rx_str[i] = rxtx_recv_byte();
        if (rx_str[i] == '\r') break;
        i++;
    }
    rx_str[i] = 0;

    debug(DEBUG_INFO, "rxtx_recv_string(char *tx_str = '%s', int max_len = %d)\n",rx_str,max_len);
}

void rxtx_send_block(const char *tx_block, int size) {
    debug(DEBUG_INFO, "rxtx_send_block(const char *tx_block = {...}, int size = %d)\n",size);

    int i = 0;
    while (i < size) {
        rxtx_send_byte(tx_block[i++]);
    }
}

void rxtx_recv_block(char *rx_block, int size) {
    debug(DEBUG_INFO, "rxtx_recv_block(char *rx_block = {...}, int size = %d)\n",size);

    int i = 0;
    while (i < size) {
        rx_block[i++] = rxtx_recv_byte();
    }
}

