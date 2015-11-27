#ifndef RETARGET_H_INCLUDED
#define RETARGET_H_INCLUDED

#include <unistd.h>

#define NO_USART 0xFFFFFFFFU

extern void retarget(int file, uint32_t usart);
extern int _write(int file, char *ptr, int len);

#endif
