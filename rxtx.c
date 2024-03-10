#include "rxtx.h"

static inline bool rxtx_send_ack_wait_cb2(bool wait_cb2) {
    debug(DEBUG_INFO, "rxtx_send_ack_wait_cb2(bool wait_cb2 = %d)\n", wait_cb2);

    int32_t timeout_count=0;
    do {
        sleep_us(1);
        if (gpio_get(USERPORT_CB2) == wait_cb2) {
            gpio_put(USERPORT_CB1,0);
            sleep_us(1);
            gpio_put(USERPORT_CB1,1);
        }
        if (++timeout_count > 100) {
            return true;
        };
    } while (gpio_get(USERPORT_CB2) == wait_cb2);

    return false;
}

void rxtx_recv_mode() {
    debug(DEBUG_INFO, "rxtx_recv_mode()\n");

    while (gpio_get(USERPORT_CB2) == 0);
    userport_set_dir(GPIO_IN);
}

void rxtx_send_mode() {
    debug(DEBUG_INFO, "rxtx_send_mode()\n");

    while (gpio_get(USERPORT_CB2) == 1);
    userport_set_dir(GPIO_OUT);
}

void rxtx_send_byte(char tx_byte) {
    debug(DEBUG_INFO, "rxtx_send_byte(char tx_byte = %02x:'%c')\n", tx_byte, (tx_byte>=32) && (tx_byte<=126) ? tx_byte : '.');

    while (gpio_get(USERPORT_CB2) == 0);
    
    gpio_put(USERPORT_PB0, tx_byte & 1);
    gpio_put(USERPORT_PB1, tx_byte & 2);
    gpio_put(USERPORT_PB2, tx_byte & 4);
    gpio_put(USERPORT_PB3, tx_byte & 8);
    gpio_put(USERPORT_PB4, tx_byte & 16);
    gpio_put(USERPORT_PB5, tx_byte & 32);
    gpio_put(USERPORT_PB6, tx_byte & 64);
    gpio_put(USERPORT_PB7, tx_byte & 128);

    rxtx_send_ack_wait_cb2(1);
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

    debug(DEBUG_INFO, "rxtx_recv_byte() = %02x:'%c'\n", rx_byte, (rx_byte>=32) && (rx_byte<=126) ? rx_byte : '.');

    rxtx_send_ack_wait_cb2(0);

    return rx_byte;
}

void rxtx_send_int(int tx_int) {
    rxtx_send_byte(((char*)&tx_int)[0]);
    rxtx_send_byte(((char*)&tx_int)[1]);
    rxtx_send_byte(((char*)&tx_int)[2]);
    rxtx_send_byte(((char*)&tx_int)[3]);
}

void rxtx_send_string(const char *tx_str) {
    debug(DEBUG_INFO, "rxtx_send_string(const char *tx_str = '%s')\n",tx_str);

    do {
        rxtx_send_byte(*tx_str);
    } while (*(tx_str++));
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

void rxtx_send_ip(const ip_addr_t tx_ip) {
    rxtx_send_byte(((char*)(&(tx_ip)))[0]);
    rxtx_send_byte(((char*)(&(tx_ip)))[1]);
    rxtx_send_byte(((char*)(&(tx_ip)))[2]);
    rxtx_send_byte(((char*)(&(tx_ip)))[3]);
}