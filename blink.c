#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "boards/pico_w.h"

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
        printf("Wi-Fi init failed");
        return -1;
    }

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led);

    for (int i=0; i<=7; i++) {
        gpio_init(i);
        gpio_set_dir(i, GPIO_OUT);
    }

    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);
    gpio_init(9);
    gpio_set_dir(9, GPIO_OUT);
    gpio_put(9,1);
    gpio_set_dir(10, GPIO_OUT);
    gpio_put(10,0);

    char out[256] = "This is some test text sent from the Pi Pico to the BBC Master over the userport!";
    int j = 0;
    while (true) {
        char c = out[j++ % 256];
        if (c == 0) {
            j = 0;
        }

        while (gpio_get(8) == 0);

        gpio_put(0, c & 1);
        gpio_put(1, c & 2);
        gpio_put(2, c & 4);
        gpio_put(3, c & 8);
        gpio_put(4, c & 16);
        gpio_put(5, c & 32);
        gpio_put(6, c & 64);
        gpio_put(7, c & 128);

        //printf("%c %2x %d\n",c,c,j);

        gpio_put(9,0);
        sleep_us(3);
        gpio_put(9,1);

        while (gpio_get(8) == 1);

        //sleep_us(10000);
    }

    int port[256];

    while (true) {
        gpio_put(9,0);
        sleep_us(5);
        gpio_put(9,1);
        while (gpio_get(8) == 1);
        while (gpio_get(8) == 0);
        int block_len =
            gpio_get(0) |
            gpio_get(1) << 1 |
            gpio_get(2) << 2 |
            gpio_get(3) << 3 |
            gpio_get(4) << 4 |
            gpio_get(5) << 5 |
            gpio_get(6) << 6 |
            gpio_get(7) << 7;

        if (block_len == 0) {
            block_len = 256;
        }
        for (int i = 0; i < block_len; i++) {
            gpio_put(9,0);
            sleep_us(5);
            gpio_put(9,1);
            while (gpio_get(8) == 1);
            while (gpio_get(8) == 0);
            port[i] =
                gpio_get(0) |
                gpio_get(1) << 1 |
                gpio_get(2) << 2 |
                gpio_get(3) << 3 |
                gpio_get(4) << 4 |
                gpio_get(5) << 5 |
                gpio_get(6) << 6 |
                gpio_get(7) << 7;
        }

        int j = 1;

        for (int i = 0; i < block_len; i++) {
            if (j++ < LINE_LENGTH) {
                printf("%02x ", port[i]);
            } else {
                printf("%02x : ", port[i]);
                for (int k = 1; k <= LINE_LENGTH; k++) {
                    if ((port[i+k-LINE_LENGTH] >= 32) && (port[i+k-LINE_LENGTH] <= 126)) {
                        printf("%c",port[i+k-LINE_LENGTH]);
                    } else {
                        printf(".");
                    }
                }
                j = 1;
                printf("\n");
            }
        }

        if (j != 1) {
            for (int k = j; k <= LINE_LENGTH; k++) {
                printf("-- ");
            }
            printf(": ");
            for (int k = 1; k <= LINE_LENGTH; k++) {
                if ((k < j) && (port[block_len-j+k] >= 32) && (port[block_len-j+k] <= 126)) {
                    printf("%c",port[block_len-j+k]);
                } else {
                    printf(".");
                }
            }
            printf("\n");
        }

        printf("------------------------------------------------------------------\n");
    }
}