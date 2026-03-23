#include "vga.h"
#include "serial.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t*)0xB8000)

volatile uint16_t* vga = VGA_MEMORY;

int row = 0, col = 0;
uint8_t color = 0x02;

void scroll() {
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga[(y - 1) * VGA_WIDTH + x] = vga[y * VGA_WIDTH + x];
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++) {
        vga[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = (color << 8) | ' ';
    }

    row = VGA_HEIGHT - 1;
}

void putc(char c) {
    if (c == '\n') {
        col = 0;
        row++;

        serial_putc('\r');
        serial_putc('\n');

        if (row >= VGA_HEIGHT) {
            scroll();
        }
        return;
    }

    if (c == '\r') return;

    if (col >= VGA_WIDTH) {
        col = 0;
        row++;
    }

    if (row >= VGA_HEIGHT) {
        scroll();
    }

    vga[row * VGA_WIDTH + col] = (color << 8) | (uint8_t)c;
    col++;

    serial_putc(c);
}

void print(const char* s) {
    for (int i = 0; s[i]; i++) {
        putc(s[i]);
    }
}

void clear() {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = (color << 8) | ' ';
    }

    row = 0;
    col = 0;

    serial_print("\x1b[2J\x1b[H");
}

void erase_last_char() {
    if (col > 0) {
        col--;
    } else if (row > 0) {
        row--;
        col = VGA_WIDTH - 1;
    } else {
        return;
    }

    vga[row * VGA_WIDTH + col] = (color << 8) | ' ';

    serial_putc('\b');
    serial_putc(' ');
    serial_putc('\b');
}

void print_int(int v) {
    char buf[16];
    int i = 0;

    if (v == 0) {
        putc('0');
        return;
    }

    if (v < 0) {
        putc('-');
        v = -v;
    }

    while (v > 0) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        putc(buf[j]);
    }
}

void print_hex(unsigned int v) {
    const char* hex = "0123456789ABCDEF";
    char buf[9];

    for (int i = 0; i < 8; i++) {
        buf[7 - i] = hex[v & 0xF];
        v >>= 4;
    }

    buf[8] = 0;

    print("0x");
    print(buf);
}
