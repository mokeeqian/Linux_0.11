/*
 *  linux/lib/_exit.c
 *
 *  (C) 1991  Linus Torvalds
 */

#define __LIBRARY__
#include <unistd.h>

volatile void _exit(int exit_code)
{
	// IA32有256个中断向量， 0x80是linux 系统调用的唯一中断号
	__asm__("int $0x80"::"a" (__NR_exit),"b" (exit_code));
}
