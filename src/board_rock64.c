#include "board_rock64.h"

int wiringPiSetupGpio(void)
{
    return 0;
}

int wiringPiISR(int pin, int mode, void (*function)(void))
{
    return 0;
}

void pinMode(int pin, int mode)
{
}

void pullUpDnControl(int pin, int mode)
{
}

int digitalRead(int pin)
{
    return 0;
}

void digitalWrite(int pin, int value)
{
}
