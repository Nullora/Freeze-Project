__attribute__((section(".multiboot")))
const unsigned int multiboot_header[] = {
    0x1BADB002,
    0x00000003,
    -(0x1BADB002 + 0x00000003)
};

#include "memory.h"
#include "timer.h"
#include <stdint.h>


/* serial I/o*/
extern void serial_putc(char c);
extern void serial_print(const char* s);
extern int serial_available(void);
extern char serial_getc(void);

/* reset outb */
static inline void outb(unsigned short port, unsigned char val){
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

volatile uint16_t* vga = (uint16_t*)0xB8000;
int row = 0, col = 0;

uint8_t color = 0x02;

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

// PORT iNPUT
unsigned char inb(unsigned short port){
    unsigned char ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Scancode
char scancode_to_ascii(unsigned char sc){
    char map[128] = {
        0,27,'1','2','3','4','5','6','7','8','9','0','-','=',8,
        9,'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
        'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
        'z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
    };
    if(sc < sizeof(map)) return map[sc];
    return 0;
}

/* backspacing thats it */
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

// ===== RTC TIME (REAL DATE) =====

// read CMOS register
unsigned char cmos_read(unsigned char reg){
    outb(0x70, reg);
    return inb(0x71);
}

// convert BCD -> binary
int bcd_to_bin(unsigned char val){
    return (val & 0x0F) + ((val / 16) * 10);
}

// read time from RTC
void read_rtc(int* sec,int* min,int* hour,int* day,int* mon,int* year){
    *sec  = bcd_to_bin(cmos_read(0x00));
    *min  = bcd_to_bin(cmos_read(0x02));
    *hour = bcd_to_bin(cmos_read(0x04));
    *day  = bcd_to_bin(cmos_read(0x07));
    *mon  = bcd_to_bin(cmos_read(0x08));
    *year = bcd_to_bin(cmos_read(0x09)) + 2000;
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

// print 2 digits (for time)
void print_2digit(int v){
    if(v < 10) putc('0');
    print_int(v);
}

// month names
const char* months[] = {
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"
};

// SERIAL input-aware reader
void get_input(char* buffer){
    int i = 0;
    while(1){
        if(serial_available()){
            char c = serial_getc();
            if(c == '\r') c = '\n';
            if(c == '\n'){ buffer[i]=0; putc('\n'); return; }
            if(c==8 || c==127){ if(i>0){ i--; erase_last_char(); } continue; }
            if(i < 127){ buffer[i++] = c; putc(c); }
            continue;
        }

        unsigned char sc = inb(0x60);
        if(sc & 0x80) continue;
        char c = scancode_to_ascii(sc);
        if(!c) continue;
        if(c=='\n'){ buffer[i]=0; putc('\n'); return; }
        if(c==8 && i>0){ i--; erase_last_char(); continue; }
        if(i < 127){ buffer[i++] = c; putc(c); }
    }
}

// strcmp
int strcmp(const char* a,const char* b){
    int i=0;
    while(a[i] && b[i]){
        if(a[i]!=b[i]) return 0;
        i++;
    }
    return a[i]==b[i];
}

/* expose bss symbol */
extern unsigned char __bss_start;
extern unsigned char __bss_end;

/* hex value */
void print_hex(unsigned int v){
    const char* hex = "0123456789ABCDEF";
    char buf[9];
    for(int i=0;i<8;i++) buf[7-i] = hex[v & 0xF], v >>= 4;
    buf[8]=0;
    print("0x");
    print(buf);
}

int startswith(const char* s,const char* p){ int i=0; while(p[i]){ if(s[i]!=p[i]) return 0; i++; } return 1; }

// commands
void handle_command(char *buf){
    if(strcmp(buf,"help")==1){
        print("=== SYSTEM ===\n");
        print("uname, date, id, who, ps, top, lsmod, systemctl, shutdown\n");
        print("=== TEXT ===\n");
        print("echo, sed, awk, wc, head\n");
        print("=== FILE ===\n");
        print("ls, pwd, file, stat, chown, ln, du\n");
        print("=== PROCESS ===\n");
        print("kill, wait, exit, sleep\n");
        print("=== USER ===\n");
        print("useradd, groups, sudo\n");
        print("=== DEV ===\n");
        print(" make, bash, sh, man, which, whereis\n");
        print("=== OTHER ===\n");
        print("clear, about, version, info, test, reboot, hl\n");
    } else if(strcmp(buf,"clear")==1){
        clear();
    } else if(strcmp(buf,"-r")==1){
        print("Safety implementations deny this action.\n");
        print_hex((unsigned int)&__bss_start); print(" - "); print_hex((unsigned int)&__bss_end); print("\n");
    } else if(strcmp(buf,"about")==1){
        print("The FreezeOS Is an operating system created by Clashnewbme.\n");
    } else if(strcmp(buf,"apps")==1){
        print("FreezeProject/applications/talktoyourself.fp\n");
        print("FreezeProject/applications/typingtest.fp\n");
        print("FreezeProject/applications/adarkplace.fp\n");
        print("To run these applications just type their file name so to run typing test type, (typingtest.fp) this will run the app\n");
    } else if(strcmp(buf,"fork while forking")==1){
        print("Forking while forking...\n");
        print("Forking while forking...\n"); outb(0x64,0xFE); for(;;);
    } else if(strcmp(buf,"version")==1){
        print("Freeze Project 0.5\n");
    } else if(strcmp(buf,"date")==1){
        int sec,min,hour,day,mon,year;

        read_rtc(&sec,&min,&hour,&day,&mon,&year);

    // CMOS validation
        if(mon < 1 || mon > 12){
            print("RTC error\n");
            return;
        }

        print(months[mon-1]);
        putc(' ');

        print_int(day);
        putc(' ');

        print_int(year);
        putc(' ');

        print_2digit(hour);
        putc(':');
        print_2digit(min);
        putc(':');
        print_2digit(sec);

        print(" UTC\n");
    } else if(strcmp(buf,"flipped date")==1){
        int year,min,hour,day,mon,sec;

        read_rtc(&year,&min,&hour,&day,&mon,&sec);

        if(mon < 1 || mon > 12){
            print("RTC error\n");
            return;
        }

        print(months[mon-1]);
        putc(' ');

        print_int(day);
        putc(' ');

        print_int(year);
        putc(' ');

        print_2digit(hour);
        putc(':');
        print_2digit(min);
        putc(':');
        print_2digit(sec);

        print(" UTC\n");
    } else if(strcmp(buf,"id")==1){
        print("uid=0(root) gid=0(root) groups=0(root),4(adm),27(sudo)\n");
    } else if(strcmp(buf,"who")==1){
        print("root     pts/0        2026-02-26 05:00 (:0)\n");
    } else if(strcmp(buf,"ps")==1){
        print("PID USER COMMAND\n1   root kernel\n2   root systemd\n");
    } else if(strcmp(buf,"top")==1){
        print("PID %%CPU %%MEM COMMAND\n");
        print("1   Unknown    Unknown    kernel\n");
    } else if(strcmp(buf,"lsmod")==1){
        print("Module       Size\nserial_core  2048\n");
    } else if(strcmp(buf,"systemctl")==1){
        print("Usage: systemctl [start|stop|status|restart] <service>\n");
    } else if(strcmp(buf,"shutdown")==1){
        print("Shutting down...\n"); outb(0x64,0xFE); for(;;);
    } else if(strcmp(buf,"ls")==1){
        print("boot/  kernel.bin  grub/  README.md\n");
    } else if(strcmp(buf,"open editor")==1){
        print("Opening editor....\n");
        print("\033[96m=== \033[95mfile.fp Editor\033[96m ===\033[0m\n");
        print("\033[92mText file succesfully created\033[0m\n");
        print("\033[93mUse type to write\033[0m\n\n");
        print("\033[94m--------------------------------\033[0m\n");
        print("\033[92mText file typer: \033[93mVersion 0.34\033[0m\n");
    } else if(strcmp(buf,"edit")==1){
        print("Opened file.fp\n");
        print("Editing file:\n"); get_input(buf); print(buf); putc('\n');
    } else if(startswith(buf,"edit")){
        print("Updated file:\n");
        print(buf + 5); putc('\n');
    } else if(strcmp(buf,"stat")==1){
        print("File: kernel.bin Size: 2.5\n");
    } else if(strcmp(buf,"chown")==1){
        print("File owner changed\n");
    } else if(strcmp(buf,"ln")==1){
        print("Creating symlink...\n");
    } else if(startswith(buf,"cat ")){
        print("Folders: Textfiles, FreezeProject, grub, iso, memory, \n");
    } else if(strcmp(buf,"echo")==1){
        print("Type something:\n"); get_input(buf); print(buf); putc('\n');
    } else if(startswith(buf,"echo ")){
        print(buf + 5); putc('\n');
    } else if(strcmp(buf,"talktoyourself.fp")==1){
        print("Opening FreezeProject/applications/talktoyourself.fp\n");
        print("        \n");
        print("        \n");
        print("\033[92m");
        print("01010100 01101000 01100101 00100000 01000110 01110010 01100101 01100101 01111010 01100101\n");
        print("01010000 01110010 01101111 01101010 01100101 01100011 01110100 00100000 00110000 00110001\n");
        print("11100010 10011000 10000101 00100000 01010011 01111001 01110011 01110100 01100101 01101101\n");
        print("00000000 11111111 10101010 01010101 11001100 00110011 11110000 00001111 10100101 01011010\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001\n");
        print("\033[0m");
        print("        \n");
        print("        \n");
        print("you> "); get_input(buf); print(buf); putc('\n');
    } else if(startswith(buf,"you> ")){
        print(buf + 5); putc('\n');
        } else if(strcmp(buf,"typingtest.fp")==1){
        print("Opening FreezeProject/applications/typingtest.fp\n");
        print("\033[92m");
        print("01010100 01101000 01100101 00100000 01000110 01110010 01100101 01100101 01111010 01100101 00100000 01010000 01110010 01101111 01101010 01100101 01100011 01110100\n");
        print("00110001 00110000 00110001 00110000 11111111 00000000 10101010 01010101 11001100 00110011 11110000 00001111 10011001 01100110 01011010 10100101 01111110 10000001\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("01101001 01101110 01100110 01101001 01101110 01101001 01110100 01100101 00100000 01110000 01100001 01110100 01110100 01100101 01110010 01101110 01110011 00100000\n");
        print("00001111 11110000 00111100 11000011 01011010 10100101 01100110 10011001 10000001 01111110 00100100 01000010 11000011 00111100 10101001 01010110 01100110 10011001\n");
        print("11111111 11111111 00000000 00000000 10101010 10101010 01010101 01010101 11001100 11001100 00110011 00110011 11110000 11110000 00001111 00001111 01111110 10000001\n");
        print("01000100 01100001 01110100 01100001 00100000 01110011 01110100 01110010 01100101 01100001 01101101 00100000 01100001 01100011 01110100 01101001 01110110 01100101\n");
        print("10100101 01011010 10011001 01100110 11000011 00111100 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000 11111111 00000000 10101010 01010101\n");
        print("01110011 01101001 01100111 01101110 01100001 01101100 00100000 01101100 01101111 01110011 01110100 00101110 00101110 00101110 00100000 01110010 01100101 01110011\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("01110010 01100101 01110011 01110100 01101111 01110010 01100101 01100100 00100000 00110000 00110001 00110001 00110000 00110001 00110000 00110001 00110000 00110001\n");
        print("11110000 00001111 11001100 00110011 10101010 01010101 00000000 11111111 10011001 01100110 01011010 10100101 00111100 11000011 01111110 10000001 01000010 00100100\n");
        print("01000101 01101110 01100100 00100000 01101111 01100110 00100000 01110011 01110100 01110010 01100101 01100001 01101101 00101110 00101110 00101110 00001010 00001010\n");
        print("10101010 01010101 11111111 00000000 11001100 00110011 11110000 00001111 00111100 11000011 01011010 10100101 01100110 10011001 01111110 10000001 01010110 10101001\n");
        print("11100011 00011100 01111110 10000001 01010101 10101010 00110011 11001100 00001111 11110000 11000011 00111100 10100101 01011010 01100110 10011001 11111111 00000000\n");
        print("00000000 11111111 01010101 10101010 00110011 11001100 00001111 11110000 01111110 10000001 01000010 00100100 11000011 00111100 10101001 01010110 10011001 01100110\n");
        print("01100011 01101111 01100100 01100101 00100000 01100110 01101100 01101111 01110111 00100000 01101001 01101110 01101001 01110100 01101001 01100001 01110100 01100101\n");
        print("00110000 00110001 00110000 00110001 11110000 00001111 11001100 00110011 10101010 01010101 11111111 00000000 01111110 10000001 01011010 10100101 01100110 10011001\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("11111111 00000000 10101010 01010101 11001100 00110011 11110000 00001111 01100110 10011001 01011010 10100101 00111100 11000011 01111110 10000001 01000010 00100100\n");
        print("01010011 01111001 01110011 01110100 01100101 01101101 00100000 01101111 01110110 01100101 01110010 01101100 01101111 01100001 01100100 00101110 00101110 00101110\n");
        print("00001111 11110000 00111100 11000011 01011010 10100101 01100110 10011001 10000001 01111110 00100100 01000010 11000011 00111100 10101001 01010110 01100110 10011001\n");
        print("11110000 11110000 00001111 00001111 11001100 11001100 00110011 00110011 10101010 10101010 01010101 01010101 11111111 11111111 00000000 00000000 01111110 10000001\n");
        print("01010100 01101000 01100101 00100000 01000110 01110010 01100101 01100101 01111010 01100101 00100000 01010000 01110010 01101111 01101010 01100101 01100011 01110100\n");
        print("00110001 00110000 00110001 00110000 11111111 00000000 10101010 01010101 11001100 00110011 11110000 00001111 10011001 01100110 01011010 10100101 01111110 10000001\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("01101001 01101110 01100110 01101001 01101110 01101001 01110100 01100101 00100000 01110000 01100001 01110100 01110100 01100101 01110010 01101110 01110011 00100000\n");
        print("00001111 11110000 00111100 11000011 01011010 10100101 01100110 10011001 10000001 01111110 00100100 01000010 11000011 00111100 10101001 01010110 01100110 10011001\n");
        print("11111111 11111111 00000000 00000000 10101010 10101010 01010101 01010101 11001100 11001100 00110011 00110011 11110000 11110000 00001111 00001111 01111110 10000001\n");
        print("01000100 01100001 01110100 01100001 00100000 01110011 01110100 01110010 01100101 01100001 01101101 00100000 01100001 01100011 01110100 01101001 01110110 01100101\n");
        print("10100101 01011010 10011001 01100110 11000011 00111100 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000 11111111 00000000 10101010 01010101\n");
        print("01110011 01101001 01100111 01101110 01100001 01101100 00100000 01101100 01101111 01110011 01110100 00101110 00101110 00101110 00100000 01110010 01100101 01110011\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("01110010 01100101 01110011 01110100 01101111 01110010 01100101 01100100 00100000 00110000 00110001 00110001 00110000 00110001 00110000 00110001 00110000 00110001\n");
        print("11110000 00001111 11001100 00110011 10101010 01010101 00000000 11111111 10011001 01100110 01011010 10100101 00111100 11000011 01111110 10000001 01000010 00100100\n");
        print("01000101 01101110 01100100 00100000 01101111 01100110 00100000 01110011 01110100 01110010 01100101 01100001 01101101 00101110 00101110 00101110 00001010 00001010\n");
        print("10101010 01010101 11111111 00000000 11001100 00110011 11110000 00001111 00111100 11000011 01011010 10100101 01100110 10011001 01111110 10000001 01010110 10101001\n");
        print("11100011 00011100 01111110 10000001 01010101 10101010 00110011 11001100 00001111 11110000 11000011 00111100 10100101 01011010 01100110 10011001 11111111 00000000\n");
        print("00000000 11111111 01010101 10101010 00110011 11001100 00001111 11110000 01111110 10000001 01000010 00100100 11000011 00111100 10101001 01010110 10011001 01100110\n");
        print("01100011 01101111 01100100 01100101 00100000 01100110 01101100 01101111 01110111 00100000 01101001 01101110 01101001 01110100 01101001 01100001 01110100 01100101\n");
        print("00110000 00110001 00110000 00110001 11110000 00001111 11001100 00110011 10101010 01010101 11111111 00000000 01111110 10000001 01011010 10100101 01100110 10011001\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("11111111 00000000 10101010 01010101 11001100 00110011 11110000 00001111 01100110 10011001 01011010 10100101 00111100 11000011 01111110 10000001 01000010 00100100\n");
        print("01010011 01111001 01110011 01110100 01100101 01101101 00100000 01101111 01110110 01100101 01110010 01101100 01101111 01100001 01100100 00101110 00101110 00101110\n");
        print("00001111 11110000 00111100 11000011 01011010 10100101 01100110 10011001 10000001 01111110 00100100 01000010 11000011 00111100 10101001 01010110 01100110 10011001\n");
        print("11110000 11110000 00001111 00001111 11001100 11001100 00110011 00110011 10101010 10101010 01010101 01010101 11111111 11111111 00000000 00000000 01111110 10000001\n");
        print("\033[0m");
        print("Welcome to THE Typing Test!\n");
        print("Type the word as shown.\n");
        print("        \n");

        print("Word: supercalifragilisticexpialidocious\n");
        print("you> "); get_input(buf);

        if(strcmp(buf,"supercalifragilisticexpialidocious")==1){
            print("Correct!\n");
        } else {
            print("Wrong! The fully correct word is 'supercalifragilisticexpialidocious'\n");
        }

        print("        \n");

        print("Word: pseudopseudohypoparathyroidism\n");
        print("you> "); get_input(buf);

        if(strcmp(buf,"pseudopseudohypoparathyroidism")==1){
            print("Correct!\n");
        } else {
            print("Wrong! The correct word is 'pseudopseudohypoparathyroidism'\n");
        }

        print("        \n");

        print("Word: antidisestablishmentarianism\n");
        print("you> "); get_input(buf);

        if(strcmp(buf,"antidisestablishmentarianism")==1){
            print("Correct!\n");
        } else {
            print("Wrong! The correct word was 'antidisestablishmentarianism'\n");
        }

        print("        \n");
        print("Game Over!\n");

        } else if(strcmp(buf,"adarkplace.fp")==1){
        print("Opening FreezeProject/applications/adarkplace.fp\n");
        print("\033[92m");
        print("01010100 01101000 01100101 00100000 01000110 01110010 01100101 01100101 01111010 01100101 00100000 01010000 01110010 01101111 01101010 01100101 01100011 01110100\n");
        print("11111111 00000000 10101010 01010101 11001100 00110011 11110000 00001111 10011001 01100110 01011010 10100101 01111110 10000001 01010110 10101001 00111100 11000011\n");
        print("00001111 11110000 00111100 11000011 01011010 10100101 01100110 10011001 10000001 01111110 00100100 01000010 11000011 00111100 10101001 01010110 01100110 10011001\n");
        print("11111111 11111111 00000000 00000000 10101010 10101010 01010101 01010101 11001100 11001100 00110011 00110011 11110000 11110000 00001111 00001111 01111110 10000001\n");
        print("01000100 01100001 01110100 01100001 00100000 01110011 01110100 01110010 01100101 01100001 01101101 00100000 01100001 01100011 01110100 01101001 01110110 01100101\n");
        print("10100101 01011010 10011001 01100110 11000011 00111100 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000 11111111 00000000 10101010 01010101\n");
        print("01110011 01101001 01100111 01101110 01100001 01101100 00100000 01101100 01101111 01110011 01110100 00101110 00101110 00101110 00100000 01110010 01100101 01110011\n");
        print("11100011 00011100 01111110 10000001 01010101 10101010 00110011 11001100 00001111 11110000 11000011 00111100 10100101 01011010 01100110 10011001 11111111 00000000\n");
        print("00000000 11111111 01010101 10101010 00110011 11001100 00001111 11110000 01111110 10000001 01000010 00100100 11000011 00111100 10101001 01010110 10011001 01100110\n");
        print("01100011 01101111 01100100 01100101 00100000 01100110 01101100 01101111 01110111 00100000 01101001 01101110 01101001 01110100 01101001 01100001 01110100 01100101\n");
        print("00110000 00110001 00110000 00110001 11110000 00001111 11001100 00110011 10101010 01010101 11111111 00000000 01111110 10000001 01011010 10100101 01100110 10011001\n");
        print("01010110 10101001 00111100 11000011 01100110 10011001 01000010 00100100 01111110 10000001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000\n");
        print("11111111 00000000 10101010 01010101 11001100 00110011 11110000 00001111 01100110 10011001 01011010 10100101 00111100 11000011 01111110 10000001 01000010 00100100\n");
        print("01010011 01111001 01110011 01110100 01100101 01101101 00100000 01101111 01110110 01100101 01110010 01101100 01101111 01100001 01100100 00101110 00101110 00101110\n");
        print("00001111 11110000 00111100 11000011 01011010 10100101 01100110 10011001 10000001 01111110 00100100 01000010 11000011 00111100 10101001 01010110 01100110 10011001\n");
        print("11110000 11110000 00001111 00001111 11001100 11001100 00110011 00110011 10101010 10101010 01010101 01010101 11111111 11111111 00000000 00000000 01111110 10000001\n");
        print("10101010 01010101 11110000 00001111 11001100 00110011 00111100 11000011 01111110 10000001 01010110 10101001 10011001 01100110 01011010 10100101 00000000 11111111\n");
        print("11000011 00111100 10101001 01010110 01100110 10011001 11100011 00011100 01010101 10101010 00110011 11001100 00001111 11110000 01111110 10000001 01000010 00100100\n");
        print("01000101 01101110 01100100 00100000 01101111 01100110 00100000 01110011 01110100 01110010 01100101 01100001 01101101 00101110 00101110 00101110 00001010 00001010\n");
        print("11111111 00000000 00000000 11111111 10101010 01010101 01010101 10101010 11001100 00110011 00110011 11001100 11110000 00001111 00001111 11110000 01111110 10000001\n");
        print("\033[0m");
        print("        \n");
        print("\033[95m=== A dark place ===\033[0m\n");
        print("Type to choose your path\n");
        print("        \n");

        print("\033[96mYou wake up in a dark place.\033[0m\n");
        print("Go 'forward' or 'stay'?\n");
        print("you> "); get_input(buf);

        if(strcmp(buf,"stay")==1){
            print("\033[90mYou wait... nothing happens...\033[0m\n");
            print("\033[91mYou fade away.\033[0m\n");
        }

        else if(strcmp(buf,"forward")==1){

            print("\033[96mYou walk forward and find a shadow.\033[0m\n");
            print("Do you 'fight' or 'run'?\n");
            print("you> "); get_input(buf);

            if(strcmp(buf,"run")==1){
                print("\033[94mYou escape safely.\033[0m\n");
                print("\033[92mYou survive.\033[0m\n");
            }

            else if(strcmp(buf,"fight")==1){

                // SCENE 3
                print("\033[91mThe shadow attacks!\033[0m\n");
                print("Type 'attack' to strike\n");
                print("you> "); get_input(buf);

                if(strcmp(buf,"attack")==1){
                    print("\033[93mYou hit the shadow!\033[0m\n");

                    // FINAL CHOICE
                    print("\033[91mIt strikes back!\033[0m\n");
                    print("Type 'attack' again or 'dodge'\n");
                    print("you> "); get_input(buf);

                    if(strcmp(buf,"dodge")==1){
                        print("\033[92mYou dodged and survived!\033[0m\n");
                    }

                    else if(strcmp(buf,"attack")==1){
                        print("\033[91mYou trade hits...\033[0m\n");
                        print("\033[91mYou fall.\033[0m\n");
                    }

                    else {
                        print("You hesitate...\n");
                        print("\033[91mThe shadow consumes you.\033[0m\n");
                    }
                }

                else {
                    print("You failed to act...\n");
                    print("\033[91mThe shadow consumes you.\033[0m\n");
                }
            }

            else {
                print("Invalid choice.\n");
            }
        }

        else {
            print("Invalid choice.\n");
        }

        print("        \n");
        print("\033[95m=== END ===\033[0m\n");
    } else if(strcmp(buf,"kill")==1 || strcmp(buf,"kill all")==1){
        print("Successfully killed all processes\n");
        print("Restarting to process changes\n"); outb(0x64,0xFE); for(;;);
    } else if(strcmp(buf,"hlr")==1){

        print("\033[41m"); 
        print("                            -- Highlight color red Selected --                           \n");
    } else if(strcmp(buf,"hlb")==1){

        print("\033[44m");
        print("                         -- Highlight color Blue Selected --                                \n");
    } else if(strcmp(buf,"hlm")==1){

        print("\033[45m");
        print("                        -- Highlight color Magenta Selected --                             \n");

    } else if(strcmp(buf,"hlg")==1){

    print("\033[42m");
    print("                        -- Highlight color Green Selected --                             \n");

    } else if(strcmp(buf,"sleep")==1){
        print("Sleeping...\n"); outb(0x64,0xFE);
    } else if(strcmp(buf,"exit")==1){
        print("Exiting...\n"); outb(0x64,0xFE); for(;;);
    } else if(strcmp(buf,"useradd")==1){
        print("Unable to do so.\n");
    } else if(strcmp(buf,"colors")==1){
        print("\033[31m&&& \033[32m&&& \033[33m&&& \033[34m&&& \033[35m&&&\n");
        print("\033[36m&&& \033[31m&&& \033[32m&&& \033[33m&&& \033[34m&&&\n");
        print("\033[35m&&& \033[36m&&& \033[31m&&& \033[32m&&& \033[33m&&&\n");
        print("\033[34m&&& \033[35m&&& \033[36m&&& \033[31m&&& \033[32m&&&\n");
        print("\033[33m&&& \033[34m&&& \033[35m&&& \033[36m&&& \033[31m&&&\n");
        print("\033[0m\n"); 
    } else if(strcmp(buf,"Install /image/colored-sky")==1){
        print("\033[34m&&& \033[34m&&& \033[34m&&& \033[31m&&& \033[33m&&& \033[34m&&&\n");
        print("\033[34m&&& \033[33m&&& \033[31m&&& \033[34m&&& \033[34m&&& \033[33m&&&\n");
        print("\033[32m&&& \033[32m&&& \033[30m&&& \033[30m&&& \033[31m&&& \033[32m&&&\n");
        print("\033[32m&&& \033[30m&&& \033[31m&&& \033[30m&&& \033[32m&&& \033[32m&&&\n");
        print("\033[31m&&& \033[33m&&& \033[31m&&& \033[33m&&& \033[31m&&& \033[33m&&&\n");
        print("\033[30m&&& \033[30m&&& \033[31m&&& \033[30m&&& \033[30m&&& \033[31m&&&\n");
        print("\033[0m\n");
    } else if(strcmp(buf,"Install /image/room")==1){
        print("\033[37m&&& \033[37m&&& \033[37m&&& \033[37m&&& \033[37m&&& \033[37m&&& \033[37m&&& \033[37m&&&\n");
        print("\033[37m&&& \033[37m&&& \033[33m&&& \033[37m&&& \033[33m&&& \033[37m&&& \033[37m&&& \033[37m&&&\n");

        print("\033[34m&&& \033[34m&&& \033[36m&&& \033[36m&&& \033[36m&&& \033[34m&&& \033[34m&&& \033[34m&&&\n");
        print("\033[34m&&& \033[36m&&& \033[36m&&& \033[33m&&& \033[36m&&& \033[36m&&& \033[34m&&& \033[34m&&&\n");
        print("\033[34m&&& \033[36m&&& \033[33m&&& \033[33m&&& \033[33m&&& \033[36m&&& \033[36m&&& \033[34m&&&\n");
        print("\033[34m&&& \033[36m&&& \033[36m&&& \033[33m&&& \033[36m&&& \033[36m&&& \033[36m&&& \033[34m&&&\n");

        print("\033[33m&&& \033[33m&&& \033[33m&&& \033[31m&&& \033[31m&&& \033[33m&&& \033[33m&&& \033[33m&&&\n");
        print("\033[33m&&& \033[31m&&& \033[31m&&& \033[31m&&& \033[31m&&& \033[31m&&& \033[31m&&& \033[33m&&&\n");

        print("\033[30m&&& \033[30m&&& \033[30m&&& \033[33m&&& \033[30m&&& \033[30m&&& \033[30m&&& \033[30m&&&\n");
        print("\033[30m&&& \033[33m&&& \033[30m&&& \033[33m&&& \033[30m&&& \033[33m&&& \033[30m&&& \033[30m&&&\n");

        print("\033[30m&&& \033[30m&&& \033[33m&&& \033[30m&&& \033[30m&&& \033[33m&&& \033[30m&&& \033[30m&&&\n");
        print("\033[30m&&& \033[33m&&& \033[30m&&& \033[33m&&& \033[30m&&& \033[30m&&& \033[33m&&& \033[30m&&&\n");

        print("\033[33m&&& \033[33m&&& \033[33m&&& \033[33m&&& \033[33m&&& \033[33m&&& \033[33m&&& \033[33m&&&\n");

        print("\033[0m\n"); 
    } else if(strcmp(buf,"Dev")==1){
        print("\033[96m=== \033[95mFreeze Project\033[96m ===\033[0m\n");
        print("\033[92mhttps://freezeos.org/\033[0m\n");
        print("\033[93mDeveloped by @Clashnewbme, @Crystal_Nitr0, and others.\033[0m\n\n");
        print("\033[94m--------------------------------\033[0m\n");
        print("\033[92mCurrently: \033[93mVersion 0.62\033[0m\n");
    } else if(strcmp(buf,"sh")==1){
        print("POSIX shell\n");
    } else if(strcmp(buf, "freezefetch") == 1){
        print("\n");
        
        print("        █████████████        FreezeOS\n");
        print("     ███████████████████     ------------\n");
        print("   ███████████████████████   OS: FreezeOS\n");
        print("  █████████████████████████  Kernel: x86\n");
        print(" ████████  ███████  ████████ Version 0.62\n");
        print(" ███████    █████    ███████ \n");
        print(" ███████   ██   ██   ███████ \n");
        print("  ███████   \\___/   ███████ \n");
        print("   ███████████████████████   \n");
        print("     ███████████████████     \n");
        print("        █████████████        \n");
        print("   ==                 ==    \n");

        print("\n");

        print("   ");
        print("\033[40m   \033[0m");
        print("\033[100m   \033[0m");
        print("\033[47m   \033[0m");
        print("\033[97m   \033[0m");
        print("\033[46m   \033[0m");
        print("\033[44m   \033[0m"); 
        print("\033[106m   \033[0m"); 
        print("\033[107m   \033[0m"); 

        print("\n\n");
    } else if(strcmp(buf,"true")==1){
        print("\n");
    } else if(strcmp(buf,"false")==1){
        print("\n");
    } else if(strcmp(buf,"info")==1 || strcmp(buf,"kernel")==1 || strcmp(buf,"test")==1){
        print("Freeze OS v0.61\n");
        print("Created by Clashnewbme and Crystal_Nitr0\n");
     } else if(strcmp(buf,"FreezeOS")==1 || strcmp(buf,"freezeos")==1 || strcmp(buf,"Freeze")==1 || strcmp(buf,"freeze")==1){
         print("Freeze\n");
    } else if(strcmp(buf,":(){:|:&};:")==1){
        print("Forking :(){:|:&};:...\n");
        print("\033[94m--------------------------------\033[0m\n");
        print("Forking :(){:|:&};:...\n"); outb(0x64,0xFE); for(;;);
    } else if(strcmp(buf,"Import /chkrootkit/*")==1){
        print("Denied\n");
        print("Restarting System\n"); outb(0x64,0xFE); for(;;);
    } else if(strcmp(buf,"reboot")==1){
        print("Rebooting...\n"); outb(0x64,0xFE); for(;;);
    } else {
        print("Command not found.\n");
    }
}

// SHELL
void shell(){
    char buf[128];
    while(1){
        print("\033[95mFreeze-OS>\033[0m ");
        get_input(buf);
        if(startswith(buf,"sudo ")){
            print("[sudo] ");
            char *rest = buf + 5;
            handle_command(rest);
        } else if(startswith(buf,"freeze ")){
            print("[freeze] ");
            char *rest = buf + 5;
            handle_command(rest);
        } else {
            handle_command(buf);
        }
    }
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
