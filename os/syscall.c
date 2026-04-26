#include "syscall.h"
#include "console.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

uint64 console_write(uint64 va, uint64 len)
{
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	tracef("write size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return len;
}

uint64 console_read(uint64 va, uint64 len)
{
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	tracef("read size = %d", len);
	for (int i = 0; i < len; ++i) {
		int c = consgetc();
		str[i] = c;
	}
	copyout(p->pagetable, va, str, len);
	return len;
}

uint64 sys_write(int fd, uint64 va, uint64 len)
{
	if (fd < 0 || fd > FD_BUFFER_SIZE)
		return -1;
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	if (f == NULL) {
		errorf("invalid fd %d\n", fd);
		return -1;
	}
	switch (f->type) {
	case FD_STDIO:
		return console_write(va, len);
	case FD_INODE:
		return inodewrite(f, va, len);
	default:
		panic("unknown file type %d\n", f->type);
	}
}

uint64 sys_read(int fd, uint64 va, uint64 len)
{
	if (fd < 0 || fd > FD_BUFFER_SIZE)
		return -1;
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	if (f == NULL) {
		errorf("invalid fd %d\n", fd);
		return -1;
	}
	switch (f->type) {
	case FD_STDIO:
		return console_read(va, len);
	case FD_INODE:
		return inoderead(f, va, len);
	default:
		panic("unknown file type %d\n", f->type);
	}
}

__attribute__((noreturn)) void sys_exit(int code)
{
	exit(code);
	__builtin_unreachable();
}

uint64 sys_sched_yield()
{
	yield();
	return 0;
}

int sys_task_info(TaskInfo* ti) {
	struct proc *p = curr_proc();
	TaskInfo *uti = (TaskInfo *)useraddr(p->pagetable, (uint64)ti);
	if (uti == 0)
		return -1;

	uti->status = Running;
	uti->time = get_cycle() * 1000 / CPU_FREQ - p->time;
	memmove(uti->syscall_times, p->syscall_times, sizeof(uti->syscall_times));
	return 0;
}

uint64 sys_mmap(uint64 start, uint64 len, int port, int flag, int fd)
{
	if (len == 0) return 0;

	if ((port & ~0x7) != 0) return -1;
	if ((port & 0x7) == 0) return -1;
	if (start != PGROUNDDOWN(start)) return -1;

	len = PGROUNDUP(len);

	int pte_flag = PTE_U | PTE_V;
	if (port & 0x1) pte_flag |= PTE_R;
	if (port & 0x2) pte_flag |= PTE_W;
	if (port & 0x4) pte_flag |= PTE_X;

	struct proc *p = curr_proc();
	uint64 npages = len / PGSIZE;

	for (uint64 i = 0; i < npages; i++) {
		uint64 va = start + i * PGSIZE;
		if (walkaddr(p->pagetable, va) != 0)
			return -1;
	}

	for (uint64 i = 0; i < npages; i++) {
		uint64 va = start + i * PGSIZE;
		void *pa = kalloc();
		if (pa == 0) return -1;
		if (walkaddr(p->pagetable, va) != 0) return -1;
		if (mappages(p->pagetable, va, PGSIZE, (uint64)pa, pte_flag) < 0) return -1;
	}

	return 0;
}

uint64 sys_munmap(uint64 start, uint64 len)
{
	if (len == 0) return 0;
	if (start != PGROUNDDOWN(start)) return -1;

	len = PGROUNDUP(len);

	struct proc *p = curr_proc();
	uint64 npages = len / PGSIZE;

	for (int i = 0; i < npages; i++) {
		uint64 va = start + i * PGSIZE;
		uint64 vpa = useraddr(p->pagetable, va);

		if (vpa == 0) return -1;
		uvmunmap(p->pagetable, va, 1, 1);
	}

	return 0;
}

uint64 sys_gettimeofday(TimeVal *val, int _tz)
{
	struct proc *p = curr_proc();
	TimeVal *uval = (TimeVal *)useraddr(p->pagetable, (uint64)val);
	if (uval == 0)
		return -1;

	uint64 cycle = get_cycle();
	
	uval->sec = cycle / CPU_FREQ;
	uval->usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	return 0;
}

uint64 sys_getpid()
{
	return curr_proc()->pid;
}

uint64 sys_getppid()
{
	struct proc *p = curr_proc();
	return p->parent == NULL ? IDLE_PID : p->parent->pid;
}

uint64 sys_clone()
{
	debugf("fork!");
	return fork();
}

static inline uint64 fetchaddr(pagetable_t pagetable, uint64 va)
{
	uint64 *addr = (uint64 *)useraddr(pagetable, va);
	return *addr;
}

uint64 sys_exec(uint64 path, uint64 uargv)
{
	struct proc *p = curr_proc();
	char name[MAX_STR_LEN];
	copyinstr(p->pagetable, name, path, MAX_STR_LEN);
	uint64 arg;
	static char strpool[MAX_ARG_NUM][MAX_STR_LEN];
	char *argv[MAX_ARG_NUM];
	int i;
	for (i = 0; uargv && (arg = fetchaddr(p->pagetable, uargv));
	     uargv += sizeof(char *), i++) {
		copyinstr(p->pagetable, (char *)strpool[i], arg, MAX_STR_LEN);
		argv[i] = (char *)strpool[i];
	}
	argv[i] = NULL;
	return exec(name, (char **)argv);
}

uint64 sys_wait(int pid, uint64 va)
{
	struct proc *p = curr_proc();
	int *code = (int *)useraddr(p->pagetable, va);
	return wait(pid, code);
}

uint64 sys_spawn(uint64 va)
{
	// project3
	struct proc *p = curr_proc();
	char name[MAX_STR_LEN];
	if (copyinstr(p->pagetable, name, va, MAX_STR_LEN) < 0)
		return -1;
	return spawn(name);
}

uint64 sys_set_priority(uint64 prio)
{
	// project3
    if (prio < 2) return -1;
	curr_proc()->priority = prio;
	return prio;
}

uint64 sys_openat(uint64 va, uint64 omode, uint64 _flags)
{
	struct proc *p = curr_proc();
	char path[200];
	copyinstr(p->pagetable, path, va, 200);
	return fileopen(path, omode);
}

uint64 sys_close(int fd)
{
	if (fd < 0 || fd > FD_BUFFER_SIZE)
		return -1;
	struct proc *p = curr_proc();
	struct file *f = p->files[fd];
	if (f == NULL) {
		errorf("invalid fd %d", fd);
		return -1;
	}
	fileclose(f);
	p->files[fd] = 0;
	return 0;
}

int sys_fstat(int fd,uint64 stat){
	//TODO: your job is to complete the syscall
	return fstat(fd, stat);
}

int sys_linkat(int olddirfd, uint64 oldpath, int newdirfd, uint64 newpath, uint64 flags){
	//TODO: your job is to complete the syscall
	return linkat(oldpath, newpath);
}

int sys_unlinkat(int dirfd, uint64 name, uint64 flags){
	//TODO: your job is to complete the syscall
	return unlinkat(name);
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_proc()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	tracef("syscall %d args = [%x, %x, %x, %x, %x, %x]", id, args[0],
	       args[1], args[2], args[3], args[4], args[5]);

	curr_proc()->syscall_times[id]++;

	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_read:
		ret = sys_read(args[0], args[1], args[2]);
		break;
	case SYS_openat:
		ret = sys_openat(args[0], args[1], args[2]);
		break;
	case SYS_close:
		ret = sys_close(args[0]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
		// __builtin_unreachable();
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	case SYS_getpid:
		ret = sys_getpid();
		break;
	case SYS_getppid:
		ret = sys_getppid();
		break;
	case SYS_clone: // SYS_fork
		ret = sys_clone();
		break;
	case SYS_execve:
		ret = sys_exec(args[0], args[1]);
		break;
	case SYS_wait4:
		ret = sys_wait(args[0], args[1]);
		break;
	case SYS_fstat:
	    ret = sys_fstat(args[0],args[1]);
		break;
	case SYS_linkat:
	    ret = sys_linkat(args[0],args[1],args[2],args[3],args[4]);
		break;
	case SYS_unlinkat:
	    ret = sys_unlinkat(args[0],args[1],args[2]);
		break;
	case SYS_spawn:
		ret = sys_spawn(args[0]);
		break;
	case SYS_task_info:
		ret = sys_task_info((TaskInfo *)args[0]);
		break;
	case SYS_mmap:
		ret = sys_mmap((uint64)args[0], (uint64)args[1],
			       (int)args[2], (int)args[3], (int)args[4]);
		break;
	case SYS_munmap:
		ret = sys_munmap((uint64)args[0], (uint64)args[1]);
		break;
	case SYS_setpriority:
		ret = sys_set_priority((uint64)args[0]);
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}