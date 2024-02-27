#include "userport.h"

void userport_init_gpio(void) {
    
    userport_set_dir(GPIO_OUT);
    gpio_init(8);
    gpio_set_dir(8, GPIO_IN);
    gpio_init(9);
    gpio_set_dir(9, GPIO_OUT);
    gpio_put(9,1);
    gpio_set_dir(10, GPIO_OUT);
    gpio_put(10,0);
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