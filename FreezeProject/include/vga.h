#ifndef VGA_H
#define VGA_H
#include<stdint.h>
extern volatile uint16_t* vga;
extern int row;
extern int col;
extern uint8_t color;

void putc(char c);
void print(const char* s);
void clear();
void erase_last_char();
void print_int(int v);
void print_hex(unsigned int v);
#endif