#ifndef RETARGET_H_INCLUDED
#define RETARGET_H_INCLUDED

#include <unistd.h>

#define NO_USART 0xFFFFFFFFU

extern void retarget(int file, uint32_t usart);
extern int _write(int file, char *ptr, int len);

extern void print_hex(uint32_t x);
extern void print(const char* s);
extern void println(const char* s);

#endif
