#ifndef __RXTX_H__
#define __RXTX_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "boards/pico_w.h"

#include "debug.h"
#include "userport.h"

void rxtx_recv_mode();
void rxtx_send_mode();

void rxtx_send_byte(char tx_byte);
char rxtx_recv_byte();

void rxtx_send_string(const char *tx_str);
void rxtx_recv_string(char *rx_str, int max_len);

void rxtx_send_block(const char *tx_block, int size);
void rxtx_recv_block(char *rx_block, int size);

#endif // __RXTX_H__