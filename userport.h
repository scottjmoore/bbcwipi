#ifndef __USERPORT_H__
#define __USERPORT_H__

#include <stdio.h>
#include "pico/stdlib.h"

#include "debug.h"

/*
GPIO Pin in/out

BBC Userport:
    PB0 <-> 74LVC245A(1) <-> GPIO0
    PB1 <-> 74LVC245A(1) <-> GPIO1
    PB2 <-> 74LVC245A(1) <-> GPIO2
    PB3 <-> 74LVC245A(1) <-> GPIO3
    PB4 <-> 74LVC245A(1) <-> GPIO4
    PB5 <-> 74LVC245A(1) <-> GPIO5
    PB6 <-> 74LVC245A(1) <-> GPIO6
    PB7 <-> 74LVC245A(1) <-> GPIO7

    CB2 <-> 74LVC245A(2) --> GPIO8
    CB1 <-> 74LVC245A(2) <-- GPIO9

Hardware Control:
    74LVC245A(1) DIR <-- GPIO10 
*/

static const int USERPORT_PB0 = 0;
static const int USERPORT_PB1 = 1;
static const int USERPORT_PB2 = 2;
static const int USERPORT_PB3 = 3;
static const int USERPORT_PB4 = 4;
static const int USERPORT_PB5 = 5;
static const int USERPORT_PB6 = 6;
static const int USERPORT_PB7 = 7;
static const int USERPORT_CB2 = 8;
static const int USERPORT_CB1 = 9;

static const int BUFFER_PB_DIR = 10;

void userport_init_gpio();
void userport_set_dir(int dir);

#endif //  __USERPORT_H__