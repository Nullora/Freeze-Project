#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>
#define SERIAL_PORT 0x3F8

static inline void outb(unsigned short port, unsigned char val);
static inline unsigned char inb(unsigned short port);
void serial_init(void);
static int serial_is_transmit_empty();
void serial_putc(char c);
void serial_print(const char* s);
int serial_available(void);
char serial_getc(void);

#endif