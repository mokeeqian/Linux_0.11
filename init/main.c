
<<<<<<< HEAD
// library 宏定义是为了包括定义在unistd.h中的内嵌汇编代码等信息
#define __LIBRARY__

// .h头文件所在的默认目录就是include/，在代码中可以不用明确指出位置
// 如果不是UNIX标准头文件(比如用户定义头文件)就需要指明确定位置，并且用双引号。
// unistd是标准符号常数与类型文件
#include <unistd.h>

// 这里主要定义了tm结构和有关时间的函数原型
#include <time.h>	
=======

#define __LIBRARY__
#include <unistd.h>
#include <time.h>
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0

/*
 * we need this inline - forking from kernel space will result
 * in NO COPY ON WRITE (!!!), until an execve is executed. This
 * is no problem, but for the stack. This is handled by not letting
 * main() use the stack at all after fork(). Thus, no function
 * calls - which means inline code for fork too, as otherwise we
 * would use the stack upon exit from 'fork()'.
 *
 * Actually only pause and fork are needed inline, so that there
 * won't be any messing with the stack from main(), but we define
 * some others too.
 */
<<<<<<< HEAD

// main.c会在移动到内核态之后才会执行fork(),避免在内核空间发生写时复制(copy on write)
// 在执行了move_to_user_mode()之后，该程序就会在任务0中运行，任务0是所有即将创建的进程的父进程，他的子进程会复制它的堆栈，所以说，我们尽量保证main.c运行在任务0的时候，不要有其他任何对堆栈的操作，以免弄乱堆栈，导致子进程的不可预料的混乱。

// 这是内嵌宏代码，在C中嵌入汇编，调用Linux中的系统调用128号中断 0x80，该中断是所有系统调用的入口！该语句实际上是int fork()创建进程的系统调用。其中，syscal0表示该系统调用无参数，1表示系统调用有一个参数，see include/unistd.h line 133
static inline _syscall0(int,fork)

// int pause() 系统调用，暂停进程，直到接收到一个信号
static inline _syscall0(int,pause)

// int setup(void *BOIS)系统调用，仅用于linux的初始化，并且只在这里被调用！
static inline _syscall1(int,setup,void *,BIOS)

// int sysc()系统调用，更新文件系统
=======
static inline _syscall0(int,fork)
static inline _syscall0(int,pause)
static inline _syscall1(int,setup,void *,BIOS)
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0
static inline _syscall0(int,sync)

#include <linux/tty.h>
#include <linux/sched.h>
#include <linux/head.h>
#include <asm/system.h>
#include <asm/io.h>

#include <stddef.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <linux/fs.h>

static char printbuf[1024];

extern int vsprintf();
extern void init(void);
extern void blk_dev_init(void);
extern void chr_dev_init(void);
extern void hd_init(void);
extern void floppy_init(void);
extern void mem_init(long start, long end);
extern long rd_init(long mem_start, int length);
<<<<<<< HEAD
extern long kernel_mktime(struct tm * tm);		// 系统开机时间
=======
extern long kernel_mktime(struct tm * tm);
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0
extern long startup_time;

/*
 * This is set up by the setup-routine at boot-time
 */
<<<<<<< HEAD

// 以下这些程序由setup.s开机引导时设置
#define EXT_MEM_K (*(unsigned short *)0x90002)	// 1M以后的扩展内存大小(kb)
#define DRIVE_INFO (*(struct drive_info *)0x90080)	// 硬盘参数表基址
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)	// 根文件系统所在设备号
=======
#define EXT_MEM_K (*(unsigned short *)0x90002)
#define DRIVE_INFO (*(struct drive_info *)0x90080)
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */

<<<<<<< HEAD
// 读取CMOS实时时钟信息
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \		// 0x70是写端口号，0x80 | addr 是要读取的CMOS内存地址
inb_p(0x71); \					// 0x71是读端口号
})

// 将BCD，码转换为二进制数字
#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

// 读取CMOS时钟，设置开机时间即startup_time
static void time_init(void)
{
	struct tm time;		// tm 结构定义在include/time.h中

	// 需要知道的是CMOS的访问速度很慢，为了减小误差，在读取完下列循环中的数值之后，若此时CMOS中的秒数发生了变化，就重新读取这些数值，可以把误差控制在1秒之内。
=======
#define CMOS_READ(addr) ({ \
outb_p(0x80|addr,0x70); \
inb_p(0x71); \
})

#define BCD_TO_BIN(val) ((val)=((val)&15) + ((val)>>4)*10)

static void time_init(void)
{
	struct tm time;

>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0
	do {
		time.tm_sec = CMOS_READ(0);
		time.tm_min = CMOS_READ(2);
		time.tm_hour = CMOS_READ(4);
		time.tm_mday = CMOS_READ(7);
		time.tm_mon = CMOS_READ(8);
		time.tm_year = CMOS_READ(9);
	} while (time.tm_sec != CMOS_READ(0));
	BCD_TO_BIN(time.tm_sec);
	BCD_TO_BIN(time.tm_min);
	BCD_TO_BIN(time.tm_hour);
	BCD_TO_BIN(time.tm_mday);
	BCD_TO_BIN(time.tm_mon);
	BCD_TO_BIN(time.tm_year);
<<<<<<< HEAD
	time.tm_mon--;		// tm_mon中的月份是0-11

	// 调用mktime.c中函数，计算从1970.1.1 0时起到目前进过的秒数，作为开机时间
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;		//机器物理内存大小(字节数)
static long buffer_memory_end = 0;	// cache末端地址
static long main_memory_start = 0;	// 主存（即将用于分页的）起始地址

struct drive_info { char dummy[32]; } drive_info;	// 用于存放硬盘参数表信息
=======
	time.tm_mon--;
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;
static long buffer_memory_end = 0;
static long main_memory_start = 0;

struct drive_info { char dummy[32]; } drive_info;
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0

void main(void)		/* This really IS void, no error here. */
{			/* The startup routine assumes (well, ...) this */
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */
<<<<<<< HEAD

	// 中断还是被屏蔽着......
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	memory_end = (1<<20) + (EXT_MEM_K<<10);		// 内存大小=1MB + 扩展内存×1024字节
	memory_end &= 0xfffff000;					// 忽略不到4 kb（一页）的字节数

	// 如果内存超过16 MB，就按16 MB 来计算
	if (memory_end > 16*1024*1024)
		memory_end = 16*1024*1024;

	// 如果内存超过12MB，就设置缓冲区末端=4 MB
	if (memory_end > 12*1024*1024) 
		buffer_memory_end = 4*1024*1024;

=======
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	memory_end = (1<<20) + (EXT_MEM_K<<10);
	memory_end &= 0xfffff000;
	if (memory_end > 16*1024*1024)
		memory_end = 16*1024*1024;
	if (memory_end > 12*1024*1024) 
		buffer_memory_end = 4*1024*1024;
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0
	else if (memory_end > 6*1024*1024)
		buffer_memory_end = 2*1024*1024;
	else
		buffer_memory_end = 1*1024*1024;
<<<<<<< HEAD

	// 设置主存起始位置为缓冲区末端
=======
>>>>>>> 61962ee7a6153684bc79c00ed14c7b57dad3ece0
	main_memory_start = buffer_memory_end;
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);
#endif
	mem_init(main_memory_start,memory_end);
	trap_init();
	blk_dev_init();
	chr_dev_init();
	tty_init();
	time_init();
	sched_init();
	buffer_init(buffer_memory_end);
	hd_init();
	floppy_init();
	sti();
	move_to_user_mode();
	if (!fork()) {		/* we count on this going ok */
		init();
	}
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */
	for(;;) pause();
}

static int printf(const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	write(1,printbuf,i=vsprintf(printbuf, fmt, args));
	va_end(args);
	return i;
}

static char * argv_rc[] = { "/bin/sh", NULL };
static char * envp_rc[] = { "HOME=/", NULL };

static char * argv[] = { "-/bin/sh",NULL };
static char * envp[] = { "HOME=/usr/root", NULL };

void init(void)
{
	int pid,i;

	setup((void *) &drive_info);
	(void) open("/dev/tty0",O_RDWR,0);
	(void) dup(0);
	(void) dup(0);
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);
	if (!(pid=fork())) {
		close(0);
		if (open("/etc/rc",O_RDONLY,0))
			_exit(1);
		execve("/bin/sh",argv_rc,envp_rc);
		_exit(2);
	}
	if (pid>0)
		while (pid != wait(&i))
			/* nothing */;
	while (1) {
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		if (!pid) {
			close(0);close(1);close(2);
			setsid();
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));
		}
		while (1)
			if (pid == wait(&i))
				break;
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();
	}
	
	// 这里的_exit()直接是一个系统调用(sys_exit)
	// exit()是C的一个库函数，它会先执行相关清除工作，关闭IO库等，再执行sys_exit系统调用
	_exit(0);	/* NOTE! _exit, not exit() */
}
