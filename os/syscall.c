#include "syscall.h"
#include "defs.h"
#include "loader.h"
#include "syscall_ids.h"
#include "timer.h"
#include "trap.h"

static inline uint64 cycle_to_ms(uint64 cycle)
{
	return (cycle / CPU_FREQ) * 1000 + (cycle % CPU_FREQ) * 1000 / CPU_FREQ;
}

uint64 sys_write(int fd, uint64 va, uint len)
{
	debugf("sys_write fd = %d va = %x, len = %d", fd, va, len);
	if (fd != STDOUT)
		return -1;
	struct proc *p = curr_proc();
	char str[MAX_STR_LEN];
	int size = copyinstr(p->pagetable, str, va, MIN(len, MAX_STR_LEN));
	debugf("size = %d", size);
	for (int i = 0; i < size; ++i) {
		console_putchar(str[i]);
	}
	return size;
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
uint64 sys_getpid(void)
{
    return curr_proc()->pid;   
}
/**
 * CH4: user passes a USER VIRTUAL ADDRESS, cannot directly dereference it.
 * Use useraddr (or copyout) to write results back. :contentReference[oaicite:4]{index=4}
 */
uint64 sys_gettimeofday(TimeVal *val, int _tz)
{
	(void)_tz;

	if (val == 0)
		return -1;

	struct proc *p = curr_proc();
	uint64 uva = (uint64)val;

	uint64 cycle = get_cycle();
	TimeVal tv;
	tv.sec = cycle / CPU_FREQ;
	tv.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;

	// Fast path: translate VA->PA and write directly if it doesn't cross a page.
	uint64 pa = useraddr(p->pagetable, uva);
	if (pa == 0)
		return -1;

	if ((uva & (PGSIZE - 1)) <= (PGSIZE - (uint64)sizeof(TimeVal))) {
		TimeVal *dst = (TimeVal *)pa;
		dst->sec = tv.sec;
		dst->usec = tv.usec;
		return 0;
	}

	// Safe path: handle potential page crossing.
	return (copyout(p->pagetable, uva, (char *)&tv, sizeof(TimeVal)) < 0) ? -1 : 0;
}

// ---- Project2 / CH4: mmap + munmap ----
// Spec: syscall ID 222 for mmap, 215 for munmap; port bits and errors as described. :contentReference[oaicite:5]{index=5}

static inline int port_to_pte_perm(int port)
{
    int perm = PTE_U;
    if (port & 0x1) perm |= PTE_R;
    if (port & 0x2) perm |= PTE_W;   
    if (port & 0x4) perm |= PTE_X;
    return perm;
}

uint64 sys_mmap(void *start, uint64 len, int port, int flag, int fd)
{
    (void)flag;
    (void)fd;

    if (start == 0) return -1;

    uint64 va0 = (uint64)start;
    if (va0 % PGSIZE != 0) return -1;

    if (len == 0) return 0;
    if (len > (1ULL << 30)) return -1;

    if ((port & ~0x7) != 0) return -1;
    if ((port & 0x7) == 0) return -1;

    uint64 sz = PGROUNDUP(len);
    struct proc *p = curr_proc();

    int perm = port_to_pte_perm(port);

    // error if any page already mapped in [va0, va0+sz) :contentReference[oaicite:1]{index=1}
    for (uint64 va = va0; va < va0 + sz; va += PGSIZE) {
        if (useraddr(p->pagetable, va) != 0) {
            return -1;
        }
    }

    for (uint64 va = va0; va < va0 + sz; va += PGSIZE) {
        void *pa_page = kalloc();
        if (pa_page == 0) return -1;

        memset(pa_page, 0, PGSIZE);

        if (mappages(p->pagetable, va, PGSIZE, (uint64)pa_page, perm) != 0)
            return -1;
    }

    return 0;
}

uint64 sys_munmap(void *start, uint64 len)
{
    if (start == 0) return -1;

    uint64 va0 = (uint64)start;
    if (va0 % PGSIZE != 0) return -1;

    if (len == 0) return 0;

    uint64 sz = PGROUNDUP(len);
    struct proc *p = curr_proc();

    // error if any unmapped page exists in [va0, va0+sz) :contentReference[oaicite:2]{index=2}
    for (uint64 va = va0; va < va0 + sz; va += PGSIZE) {
        if (useraddr(p->pagetable, va) == 0) {
            return -1;
        }
    }

    uvmunmap(p->pagetable, va0, sz / PGSIZE, 1);
    return 0;
}



/*
 * CH4: taskinfo user pointer is also a USER VA, so we must not write directly.
 * Use copyout (recommended for cross-page safety). :contentReference[oaicite:15]{index=15}
 */
uint64 sys_task_info(TaskInfo *info)
{
	if (info == 0)
		return -1;

	struct proc *p = curr_proc();

	TaskInfo kinfo;
	memset(&kinfo, 0, sizeof(kinfo));

	kinfo.status = Running;

	for (int i = 0; i < MAX_SYSCALL_NUM; i++) {
		kinfo.syscall_times[i] = p->syscall_times[i];
	}

	if (p->start_time_ms == (uint64)-1) {
		kinfo.time = 0;
	} else {
		uint64 now_ms = cycle_to_ms(get_cycle());
		kinfo.time = (int)(now_ms - p->start_time_ms);
	}

	return (copyout(p->pagetable, (uint64)info, (char *)&kinfo, sizeof(kinfo)) < 0) ? -1 : 0;
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

	// syscall counter for taskinfo
	if (id >= 0 && id < MAX_SYSCALL_NUM) {
		curr_proc()->syscall_times[id]++;
	}

	switch (id) {
	case SYS_write:
		ret = sys_write(args[0], args[1], args[2]);
		break;
	case SYS_exit:
		sys_exit(args[0]);
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday((TimeVal *)args[0], args[1]);
		break;
	case SYS_task_info:
		ret = sys_task_info((TaskInfo *)args[0]);
		break;

	// CH4 mmap/munmap syscalls :contentReference[oaicite:16]{index=16}
	case SYS_mmap:
		ret = sys_mmap((void *)args[0], args[1], (int)args[2], (int)args[3], (int)args[4]);
		break;
	case SYS_munmap:
		ret = sys_munmap((void *)args[0], args[1]);
		break;
	case SYS_getpid:
		ret = sys_getpid();
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}

	trapframe->a0 = ret;
	tracef("syscall ret %d", ret);
}