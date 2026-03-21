#include "vga.h"
void putc(char c){
    if(c=='\n'){
        row++;
        col = 0;
        /* mirror new line */
        serial_putc('\r');
        serial_putc('\n');
        return;
    }

    if(row >= 25) row = 0;

    vga[row*80 + col] = (color << 8) | c;

    col++;
    if(col >= 80){ col = 0; row++; }

    /* mirror to serial so -nographic shows output */
    serial_putc(c);
}

void print(const char* s){ for(int i=0; s[i]; i++) putc(s[i]); }

void clear(){
    for(int i=0;i<80*25;i++) vga[i] = (color<<8) | ' ';
    row = 0; col = 0;
    /* clear serial terminal */
    serial_print("\x1b[2J\x1b[H");
}
void erase_last_char(){
    if(col > 0){
        col--;
        vga[row*80 + col] = (0x07 << 8) | ' ';
        serial_putc('\b'); serial_putc(' '); serial_putc('\b');
    } else if(row > 0){
        row--; col = 79;
        vga[row*80 + col] = (0x07 << 8) | ' ';
        serial_putc('\b'); serial_putc(' '); serial_putc('\b');
    }
}