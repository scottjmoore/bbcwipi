#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "boards/pico_w.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "hardware/vreg.h"
#include "hardware/clocks.h"

#include "rxtx.h"

/*
GPIO Pinout

BBC Userport PB0 <-> GPIO0
BBC Userport PB1 <-> GPIO1
BBC Userport PB2 <-> GPIO2
BBC Userport PB3 <-> GPIO3
BBC Userport PB4 <-> GPIO4
BBC Userport PB5 <-> GPIO5
BBC Userport PB6 <-> GPIO6
BBC Userport PB7 <-> GPIO7
BBC Userport CB2 --> GPIO8
BBC Userport CB1 <-- GPIO9

74LVC245A(1) DIR <-- GPIO10 
*/

#define LINE_LENGTH 16

int main()
{
    bool led=1;

    stdio_init_all();

    if (cyw43_arch_init()) {
        debug(DEBUG_ERR,"Wi-Fi init failed");
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);

    userport_init_gpio();
    userport_set_dir(GPIO_IN);

    char rx_block[256];
    char tx_block[256];

    while (true) {
        rxtx_recv_mode();
        char rx_byte = rxtx_recv_byte();
        rxtx_recv_string(rx_block, 256);
        debug(DEBUG_LOG,"%s = %d\n", rx_block, rx_byte);

        //for (int i = 0; i < 256; i++) {
            //tx_block[i] = rx_byte;
        //}

        rxtx_send_mode();
        rxtx_send_block(tx_block, 256);
    }
}