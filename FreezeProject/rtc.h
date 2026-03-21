#ifndef RTC_H
#define RTC_H
unsigned char cmos_read(unsigned char reg);
int bcd_to_bin(unsigned char val);
void read_rtc(int* sec,int* min,int* hour,int* day,int* mon,int* year);
void print_2digit(int v);
extern const char* months[];
#endif