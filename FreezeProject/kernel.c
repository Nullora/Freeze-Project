__attribute__((section(".multiboot")))
const unsigned int multiboot_header[] = {
    0x1BADB002,
    0x00000003,
    -(0x1BADB002 + 0x00000003)
};

#include "memory.h"
#include "timer.h"
#include "input.h"
#include "vga.h"
#include "rtc.h"
#include "shell.h"
#include <stdint.h>


/* serial I/o*/
extern void serial_putc(char c);
extern void serial_print(const char* s);
extern int serial_available(void);
extern char serial_getc(void);

/* reset srut */
static inline void outb(unsigned short port, unsigned char val){
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// print integer
void print_int(int v){
    char buf[16];
    int i = 0;

    if(v == 0){
        putc('0');
        return;
    }

    while(v > 0){
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    for(int j = i - 1; j >= 0; j--){
        putc(buf[j]);
    }
}

/* hex value */
void print_hex(unsigned int v){
    const char* hex = "0123456789ABCDEF";
    char buf[9];
    for(int i=0;i<8;i++) buf[7-i] = hex[v & 0xF], v >>= 4;
    buf[8]=0;
    print("0x");
    print(buf);
}

// ENTRY 
void kernel_main(void){
    clear();
    print("\033[96m=== \033[95mFreeze Project\033[96m ===\033[0m\n");
    print("\033[92mhttps://freezeos.org/\033[0m\n");
    print("\033[93mType 'help' for help on learning commands\033[0m\n\n");
    print("\033[94m--------------------------------\033[0m\n");
    print("\033[92mCurrently: \033[93mVersion 0.62\033[0m\n");

    shell();
}
