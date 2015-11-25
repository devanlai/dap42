#ifndef RETARGET_H_INCLUDED
#define RETARGET_H_INCLUDED

#include <unistd.h>

#define NO_UART 0xFFFFFFFFU

extern void retarget(int file, uint32_t uart);
extern int _write(int file, char *ptr, int len);

#endif
