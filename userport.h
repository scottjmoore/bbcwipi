#ifndef __USERPORT_H__
#define __USERPORT_H__

#include <stdio.h>
#include "pico/stdlib.h"

#include "debug.h"

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

void userport_init_gpio();
void userport_set_dir(int dir);

#endif //  __USERPORT_H__