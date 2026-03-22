#ifndef SHELL_H
#define SHELL_H
int startswith(const char* s,const char* p);
int strcmp(const char* a,const char* b);
void shell();
void handle_command(char *buf);
extern unsigned char __bss_start;
extern unsigned char __bss_end;
#endif