#ifndef INPUT_H
#define INPUT_H

unsigned char inb(unsigned short port);
void get_input(char* buffer);
char scancode_to_ascii(unsigned char sc);
#endif
