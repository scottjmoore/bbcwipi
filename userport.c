#include "userport.h"

void userport_init_gpio(void) {
    debug(DEBUG_INFO, "userport_init_gpio(void)\n");
    
    gpio_init(USERPORT_CB1);
    gpio_pull_up(USERPORT_CB1);
    gpio_set_dir(USERPORT_CB1, GPIO_OUT);
    gpio_put(USERPORT_CB1, 1);

    gpio_init(USERPORT_CB2);
    gpio_pull_down(USERPORT_CB2);
    gpio_set_dir(USERPORT_CB2, GPIO_IN);

    gpio_init(BUFFER_PB_DIR);
    gpio_set_dir(BUFFER_PB_DIR, GPIO_OUT);
    gpio_put(BUFFER_PB_DIR, 1);

    gpio_init(USERPORT_PB0);
    gpio_init(USERPORT_PB1);
    gpio_init(USERPORT_PB2);
    gpio_init(USERPORT_PB3);
    gpio_init(USERPORT_PB4);
    gpio_init(USERPORT_PB5);
    gpio_init(USERPORT_PB6);
    gpio_init(USERPORT_PB7);

    userport_set_dir(GPIO_OUT);
}

void userport_set_dir(int dir) {
    debug(DEBUG_INFO, "userport_set_dir(int dir = %d)\n",dir);

    gpio_put(BUFFER_PB_DIR, 1 - dir);

    //gpio_init(USERPORT_PB0);
    gpio_set_dir(USERPORT_PB0, dir);
    //gpio_init(USERPORT_PB1);
    gpio_set_dir(USERPORT_PB1, dir);
    //gpio_init(USERPORT_PB2);
    gpio_set_dir(USERPORT_PB2, dir);
    //gpio_init(USERPORT_PB3);
    gpio_set_dir(USERPORT_PB3, dir);
    //gpio_init(USERPORT_PB4);
    gpio_set_dir(USERPORT_PB4, dir);
    //gpio_init(USERPORT_PB5);
    gpio_set_dir(USERPORT_PB5, dir);
    //gpio_init(USERPORT_PB6);
    gpio_set_dir(USERPORT_PB6, dir);
    //gpio_init(USERPORT_PB7);
    gpio_set_dir(USERPORT_PB7, dir);
}