
// library 宏定义是为了包括定义在unistd.h中的内嵌汇编代码等信息
#define __LIBRARY__

// .h头文件所在的默认目录就是include/，在代码中可以不用明确指出位置
// 如果不是UNIX标准头文件(比如用户定义头文件)就需要指明确定位置，并且用双引号。
// unistd是标准符号常数与类型文件
#include <unistd.h>

// 这里主要定义了tm结构和有关时间的函数原型
#include <time.h>

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

//
// main.c会在移动到内核态之后才会执行fork(),避免在内核空间发生写时复制(copy on write)
// 在执行了move_to_user_mode()之后，该程序就会在任务0中运行，任务0是所有即将创建的进程的父进程，
//他的子进程会复制它的堆栈，所以说，我们尽量保证main.c运行在任务0的时候，不要有其他任何对堆栈的操作，以免弄乱堆栈，导致子进程的不可预料的混乱。
//

// 这是内嵌宏代码，在C中嵌入汇编，调用Linux中的系统调用128号中断 0x80，该中断是所有系统调用的入口！
//该语句实际上是int fork()创建进程的系统调用。其中，syscal0表示该系统调用无参数，1表示系统调用有一个参数，see include/unistd.h line 133
static inline _syscall0(int,fork)

// int pause() 系统调用，暂停进程，直到接收到一个信号
static inline _syscall0(int,pause)

// int setup(void *BOIS)系统调用，仅用于linux的初始化，并且只在这里被调用！
static inline _syscall1(int,setup,void *,BIOS)

// int sysc()系统调用，更新文件系统
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
extern long kernel_mktime(struct tm * tm);		// 系统开机时间
extern long startup_time;

/*
 * This is set up by the setup-routine at boot-time
 */

// 以下这些程序由setup.s开机引导时设置
#define EXT_MEM_K (*(unsigned short *)0x90002)	// 1M以后的扩展内存大小(kb)
#define DRIVE_INFO (*(struct drive_info *)0x90080)	// 硬盘参数表基址
#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)	// 根文件系统所在设备号

/*
 * Yeah, yeah, it's ugly, but I cannot find how to do this correctly
 * and this seems to work. I anybody has more info on the real-time
 * clock I'd be interested. Most of this was trial and error, and some
 * bios-listing reading. Urghh.
 */

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

	time.tm_mon--;		// tm_mon中的月份是0-11

	// 调用mktime.c中函数，计算从1970.1.1 0时起到目前进过的秒数，作为开机时间
	startup_time = kernel_mktime(&time);
}

static long memory_end = 0;		//机器物理内存大小(字节数)
static long buffer_memory_end = 0;	// cache末端地址
static long main_memory_start = 0;	// 主存（即将用于分页的）起始地址

struct drive_info { char dummy[32]; } drive_info;	// 用于存放硬盘参数表信息


void main(void)		/* This really IS void, no error here. */
{			/* The startup routine assumes (well, ...) this */
/*
 * Interrupts are still disabled. Do necessary setups, then
 * enable them
 */

	// 中断还是被屏蔽着......
 	ROOT_DEV = ORIG_ROOT_DEV;
 	drive_info = DRIVE_INFO;
	memory_end = (1<<20) + (EXT_MEM_K<<10);		// 内存大小=1MB + 扩展内存×1024字节
	memory_end &= 0xfffff000;					// 忽略不到4 kb（一页）的字节数

	// 如果内存超过16 MB，就按16 MB 来计算,就是说Linux-0.11最大支持16MB物理内存，其中 0-1MB内存空间用于内核系统（实际上是0-640KB）
	if (memory_end > 16*1024*1024)
		memory_end = 16*1024*1024;

	// 如果内存超过12MB，就设置缓冲区末端=4 MB
	if (memory_end > 12*1024*1024) 
		buffer_memory_end = 4*1024*1024;

	else if (memory_end > 6*1024*1024)
		buffer_memory_end = 2*1024*1024;
	else
		buffer_memory_end = 1*1024*1024;


	// 设置主存起始位置为缓冲区末端
	main_memory_start = buffer_memory_end;

// 如果定义了内存虚拟盘，则初始化虚拟盘，此时主存将减少。
#ifdef RAMDISK
	main_memory_start += rd_init(main_memory_start, RAMDISK*1024);
#endif

	// 以下是内核初始化工作
	mem_init(main_memory_start,memory_end);
	trap_init();		// 陷阱门（硬件中断向量）
	blk_dev_init();		// 块设备初始化
	chr_dev_init();		// 字符设备初始化
	tty_init();			// tty初始化
	time_init();		// 设置设备开机时间
	sched_init();		// 进程调度程序初始化（加载任务0的tr, ldtr）
	buffer_init(buffer_memory_end);		// 缓冲区初始化，建立内存链表
	hd_init();
	floppy_init();
	sti();
	move_to_user_mode();
	if (!fork()) {		/* we count on this going ok */
		init();			// 这个语句是在任务 1 中执行
	}
/*
 *   NOTE!!   For any other task 'pause()' would mean we have to get a
 * signal to awaken, but task0 is the sole exception (see 'schedule()')
 * as task 0 gets activated at every idle moment (when no other tasks
 * can run). For task0 'pause()' just means we go check if some other
 * task can run, and if not we return here.
 */

	// 这里可以看出，pause()系统调用会把任务 0 切换成可中断等待状态，再执行调度函数
	// 调度函数只要发现系统中没有其他进程可以执行的时候，就会执行任务 0，与任务0所处的进程状态无关！
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

static char * argv[] = { "-/bin/sh",NULL };		// 这里多了一个连字符？？
static char * envp[] = { "HOME=/usr/root", NULL };

// 这个init()运行在任务1中，首先对shell程序进行初始化，然后加载程序并执行。
void init(void)
{
	int pid,i;

	// 这是一个系统调用。用来读取硬盘参数包括分区表信息，安装根文件系统设备，对应的函数是sys_setup()
	setup((void *) &drive_info);

	// 以读写方式打开设备/dev/tty0，它是Linux的终端控制台
	// 由于这是系统第一次打开文件系统，所以文件句柄一定是 1,该句柄是stdin，以读写的方式打开是为了复制句柄stdout和stderr
	(void) open("/dev/tty0",O_RDWR,0);

	// 复制句柄，产生句柄一号，sdtout
	(void) dup(0);	

	// 复制句柄，产生句柄二号，stderr
	(void) dup(0);

	// 缓冲区块数和每块大小（1024b）
	printf("%d buffers = %d bytes buffer space\n\r",NR_BUFFERS,
		NR_BUFFERS*BLOCK_SIZE);
	// 空闲内存大小
	printf("Free mem: %d bytes\n\r",memory_end-main_memory_start);

	// 尝试创建一个子进程（2号进程）
	// 其中，如果创建成功，对于新创建的子进程，fork()会返回0；对于父进程，fork()会返回子进程的pid。创建失败，返回一个负值
	// !(pid), 也就是当pid=0时，这个if逻辑才为真，所以说，该if下的语句只有子进程（2号进程才会执行！！！）
	if (!(pid=fork())) {
		close(0);	// 关闭句柄0，即stdin
		if (open("/etc/rc",O_RDONLY,0))		// 以只读方式打开设备/etc/rc
			_exit(1);						// 打开失败，退出

		// execve()可以深入了解一下。
		execve("/bin/sh",argv_rc,envp_rc);	// 将该进程替换成/bin/sh进程，即shell进程，并执行shell进程！

		_exit(2);							// 若 execve()执行失败，退出
	}


	// TODO: 但是这里我有个疑问，pid在这里只是一个局部变量，fork()一次后，对于父进程和子进程，如何做到 变量pid 的不同？
	//
	// pid > 0，所以这是父进程（1号进程）执行的逻辑
	if (pid>0)

		// wait() 等待子进程的停止或终止，返回子进程的pid，
		while (pid != wait(&i))				// pid != 子进程的pid
			/* nothing */;
			// 也就是继续等待

	// 如果代码能够运行到这里，说明上面创建的2号进程已经停止或者终止了，这个时候，再创建一个子进程，如果出错，继续创建。
	// 对于所创建的子进程，关闭所有之前打开的句柄，新创建一个新的会话，重新打开/dev/tty0，作为stdin，再次复制句柄，执行shell程序
	// 但是注意，这次执行shell的参数变了！
	// 然后，父进程还是等待子进程结束，如果子进程结束了，输出，然后继续重试，形成一个无限大的“死”循环。

	while (1) {

		// 创建子进程失败，继续...
		if ((pid=fork())<0) {
			printf("Fork failed in init\r\n");
			continue;
		}
		// 新的子进程执行的逻辑
		if (!pid) {
			close(0);close(1);close(2);
			setsid();							// 创建一个新的会话
			(void) open("/dev/tty0",O_RDWR,0);
			(void) dup(0);
			(void) dup(0);
			_exit(execve("/bin/sh",argv,envp));
		}

		// 不断试探，看看新创建的子进程是否已经停止
		while (1)
			// 已经停止，退出试探
			if (pid == wait(&i))
				break;		
		printf("\n\rchild %d died with code %04x\n\r",pid,i);
		sync();									// 同步，刷新缓冲区
	}
	
	// 这里的_exit()直接是一个系统调用(sys_exit)
	// exit()是C的一个库函数，它会先执行相关清除工作，关闭IO库等，再执行sys_exit系统调用
	_exit(0);	/* NOTE! _exit, not exit() */
}


/*
 * 最后，关于这个main.c里的几个知识点做个总结：
 * 
 * 1. CMOS
 * 	何为CMOS，全名互补金属氧化物半导体，实际上是由计算机内部电池供电的64或128字节的RAM内存块，是系统时钟芯片的一部分。
 * 	具体先不做深入了解。
 *
 * 2. 系统调用fork()创建进程
 * 	通过fork()来创建一个和父进程几乎一样的进程，这个子进程与父进程执行同样的代码，但是拥有自己独立的数据空间和环境参数。
 *	在父进程中，执行了一次fork()，如果执行失败，会返回一个负值；如果执行成功，对于父进程和子进程的返回值是不一样的：
 *	父进程得到子进程的进程标识符，即pid，子进程得到0。这也是代码里面区分父子进程的一个方法。
 *	一旦执行了fork()，如果创建成功，就会有两个进程在执行这个程序。但是他们执行的内容不同，新创建的子进程会从fork()语句的下一条语句开始执行，
 *	父进程还是执行原来的程序。要注意：父进程和子进程执行的顺序是随机 的！！如果我们不加以限制的话。
 *
 *
