#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#include "board_rock64.h"

#define GPIO_PINS (32)

#ifdef ROCK64_EMMC
// can use some pins if boot from eMMC (not using SD card)
static const int gpioToRock64[GPIO_PINS] = {
     -1,  -1,  89,  88,  60,  70,   2,  76, 104,  98,  67,  96,  38,  32,  64,  65,
     37,  66,  67,  33,  36,  35, 100, 101, 102, 103,  34,   0,  -1,  -1,  -1,  -1
};
#else
static const int gpioToRock64[GPIO_PINS] = {
     -1,  -1,  89,  88,  60,  70,   2,  76, 104,  98,  67,  96,  -1,  -1,  64,  65,
     -1,  66,  67,  -1,  -1,  -1, 100, 101, 102, 103,  -1,   0,  -1,  -1,  -1,  -1
};
#endif

typedef void (*funcptr)(void);

static struct gpioRock64 {
    bool export;
    bool activeLow;
    funcptr callback;
    int mode;
} gpio[GPIO_PINS];

int rock64GpioNum(int wpiPinNo)
{
    if ((wpiPinNo < 0)||(wpiPinNo >= GPIO_PINS)) {return -1;}

    return gpioToRock64[wpiPinNo];
}

void rock64GpioPoller(void)
{
    int last_state[GPIO_PINS];

    for (int i = 0; i < GPIO_PINS; i++) {
        last_state[i] = 0;
    }

    while(1) {
        for (int i = 0; i < GPIO_PINS; i++) {
            if (gpio[i].mode == 0) {continue;}

            const int state = digitalRead(i);
            if (last_state[i] == state) {continue;}

            // changed
            last_state[i] = state;

            const int mode = gpio[i].mode;
            if ((state == 1 && (mode == INT_EDGE_BOTH || mode == INT_EDGE_RISING)) ||
                (state == 0 && (mode == INT_EDGE_BOTH || mode == INT_EDGE_FALLING))) {
                gpio[i].callback();
            }
        }
        usleep(1000); // 1ms
    }
}

int rock64GpioIsExported(int r64PinNo)
{
    char buf[128];
    DIR *dirHandle;

    if (r64PinNo == -1) {return 0;}

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d", r64PinNo);
    dirHandle = opendir(buf);

    if (!dirHandle) {
        return 0;
    }

    closedir(dirHandle);

    return 1;
}

void rock64GpioExport(int r64PinNo)
{
    FILE *fileHandle;

    if (r64PinNo == -1) {return;}

    fileHandle = fopen("/sys/class/gpio/export", "w");
    if (!fileHandle) {return;}

    fprintf(fileHandle, "%d", r64PinNo);

    fclose(fileHandle);

    return;
}

void rock64GpioUnexport(int r64PinNo)
{
    FILE *fileHandle;

    if (r64PinNo == -1) {return;}

    fileHandle = fopen("/sys/class/gpio/unexport", "w");
    if (!fileHandle) {return;}

    fprintf(fileHandle, "%d", r64PinNo);

    fclose(fileHandle);

    return;
}

void rock64GpioUnexportAll(void)
{
    for (int i = 0; i < GPIO_PINS; i++) {
        if (gpio[i].export) {
            rock64GpioUnexport(rock64GpioNum(i));
        }
    }
}

int wiringPiSetupGpio(void)
{
    pthread_t thread;

    for (int i = 0; i < GPIO_PINS; i++) {
        gpio[i].export = false;
        gpio[i].activeLow = false;
        gpio[i].callback = NULL;
        gpio[i].mode = 0;
    }

    atexit(rock64GpioUnexportAll);

    pthread_create(&thread, NULL, (void *(*)(void *)) rock64GpioPoller, (void *) NULL);
    pthread_detach(thread);

    return 0;
}

int wiringPiISR(int pin, int mode, void (*function)(void))
{
    gpio[pin].callback = function;
    gpio[pin].mode = mode;

    return 0;
}

void pinMode(int pin, int mode)
{
    char buf[128];
    FILE *fileHandle;
    int r64PinNo = rock64GpioNum(pin);
    if (r64PinNo == -1) {return;}

    if (!rock64GpioIsExported(r64PinNo)) {
        rock64GpioExport(r64PinNo);
        gpio[pin].export = true;
    }

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", r64PinNo);

    fileHandle = fopen(buf, "w");
    if (!fileHandle) {return;}

    if (mode == OUTPUT) {
        fputs("out", fileHandle);
    } else {
        fputs("in", fileHandle);
    }

    fclose(fileHandle);
}

void pullUpDnControl(int pin, int mode)
{
    char buf[128];
    FILE *fileHandle;
    int r64PinNo = rock64GpioNum(pin);
    if (r64PinNo == -1) {return;}

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/active_low", r64PinNo);

    fileHandle = fopen(buf, "w");
    if (!fileHandle) {return;}

    if (mode == PUD_UP) {
        fputc('1', fileHandle);
    } else {
        fputc('0', fileHandle);
    }

    fclose(fileHandle);

    gpio[pin].activeLow = (mode == PUD_UP)?(1):(0);
}

int digitalRead(int pin)
{
    int state = 0;
    char buf[128];
    FILE *fileHandle;
    int r64PinNo = rock64GpioNum(pin);
    if (r64PinNo == -1) {return 0;}

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", r64PinNo);

    fileHandle = fopen(buf, "r");
    if (!fileHandle) {return 0;}

    if (fgetc(fileHandle) == '1') {
        state = 1;
    }

    if (gpio[pin].activeLow) {
        state = !state;
    }

    fclose(fileHandle);

    return state;
}

void digitalWrite(int pin, int value)
{
    char buf[128];
    FILE *fileHandle;
    int r64PinNo = rock64GpioNum(pin);
    if (r64PinNo == -1) {return;}

    snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", r64PinNo);

    fileHandle = fopen(buf, "w");
    if (!fileHandle) {return;}

    if (value) {
        fputc('1', fileHandle);
    } else {
        fputc('0', fileHandle);
    }

    fclose(fileHandle);
}
