#include "mbed.h"
#include "millis.h"

volatile unsigned long  _millis;

void ticker_SysTick_Handler(void) {
    _millis++;
}

unsigned long millis(void) {
    return _millis;
}
