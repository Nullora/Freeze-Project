#include <stdint.h>

volatile uint32_t ticks = 0;
volatile uint32_t idle_ticks = 0;

void timer_callback() {
    ticks++;
}
