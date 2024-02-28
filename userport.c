#include "userport.h"

void userport_init_gpio(void) {
    
    gpio_init(USERPORT_CB1);
    gpio_set_dir(USERPORT_CB1, GPIO_OUT);
    gpio_init(USERPORT_CB2);
    gpio_set_dir(USERPORT_CB2, GPIO_IN);

    userport_set_dir(GPIO_OUT);
}

void userport_set_dir(int dir) {
    gpio_init(USERPORT_PB0);
    gpio_set_dir(USERPORT_PB0, dir);
    gpio_init(USERPORT_PB1);
    gpio_set_dir(USERPORT_PB1, dir);
    gpio_init(USERPORT_PB2);
    gpio_set_dir(USERPORT_PB2, dir);
    gpio_init(USERPORT_PB3);
    gpio_set_dir(USERPORT_PB3, dir);
    gpio_init(USERPORT_PB4);
    gpio_set_dir(USERPORT_PB4, dir);
    gpio_init(USERPORT_PB5);
    gpio_set_dir(USERPORT_PB5, dir);
    gpio_init(USERPORT_PB6);
    gpio_set_dir(USERPORT_PB6, dir);
    gpio_init(USERPORT_PB7);
    gpio_set_dir(USERPORT_PB7, dir);
}