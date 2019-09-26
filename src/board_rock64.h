// Pin modes

#define INPUT            0
#define OUTPUT           1

// Pullup/down/none

#define PUD_OFF          0
#define PUD_DOWN         1
#define PUD_UP           2

// Interrupt levels

#define INT_EDGE_SETUP      0
#define INT_EDGE_FALLING    1
#define INT_EDGE_RISING     2
#define INT_EDGE_BOTH       3

int wiringPiSetupGpio(void);
int wiringPiISR(int pin, int mode, void (*function)(void));
void pinMode(int pin, int mode);
void pullUpDnControl(int pin, int mode);
int digitalRead(int pin);
void digitalWrite(int pin, int value);

