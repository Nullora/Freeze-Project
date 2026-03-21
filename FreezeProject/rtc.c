#include "rtc.h"
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