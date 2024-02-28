#ifndef __RXTX_H__
#define __RXTX_H__

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "boards/pico_w.h"

#include "userport.h"

void rxtx_send_byte(char tx_byte);
void rxtx_send_string(const char *tx_str);

#endif // __RXTX_H__