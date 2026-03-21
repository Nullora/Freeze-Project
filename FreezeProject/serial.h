#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#define SERIAL_PORT 0x3F8

void serial_init(void);
void serial_putc(char c);
void serial_print(const char* s);
int serial_available(void);
char serial_getc(void);

#endif