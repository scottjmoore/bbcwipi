#ifndef __USERPORT_H__
#define __USERPORT_H__

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

    */

    const uint8_t USERPORT_PB0 = 0;
    const uint8_t USERPORT_PB1 = 1;
    const uint8_t USERPORT_PB2 = 2;
    const uint8_t USERPORT_PB3 = 3;
    const uint8_t USERPORT_PB4 = 4;
    const uint8_t USERPORT_PB5 = 5;
    const uint8_t USERPORT_PB6 = 6;
    const uint8_t USERPORT_PB7 = 7;
    const uint8_t USERPORT_CB2 = 8;
    const uint8_t USERPORT_CB1 = 9;
    
#endif //  __USERPORT_H__