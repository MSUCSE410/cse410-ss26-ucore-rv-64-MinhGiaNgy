#include "console.h"
#include "defs.h"
#include "loader.h"
#include "sync.h"
#include "syscall.h"
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
	case FD_PIPE:
		return pipewrite(f->pipe, va, len);
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
	case FD_PIPE:
		return piperead(f->pipe, va, len);
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

uint64 sys_gettimeofday(uint64 val, int _tz)
{
	struct proc *p = curr_proc();
	uint64 cycle = get_cycle();
	TimeVal t;
	t.sec = cycle / CPU_FREQ;
	t.usec = (cycle % CPU_FREQ) * 1000000 / CPU_FREQ;
	copyout(p->pagetable, val, (char *)&t, sizeof(TimeVal));
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

uint64 sys_pipe(uint64 fdarray)
{
	struct proc *p = curr_proc();
	uint64 fd0, fd1;
	struct file *f0, *f1;
	if (f0 < 0 || f1 < 0) {
		return -1;
	}
	f0 = filealloc();
	f1 = filealloc();
	if (pipealloc(f0, f1) < 0)
		goto err0;
	fd0 = fdalloc(f0);
	fd1 = fdalloc(f1);
	if (fd0 < 0 || fd1 < 0)
		goto err0;
	if (copyout(p->pagetable, fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
	    copyout(p->pagetable, fdarray + sizeof(uint64), (char *)&fd1,
		    sizeof(fd1)) < 0) {
		goto err1;
	}
	return 0;

err1:
	p->files[fd0] = 0;
	p->files[fd1] = 0;
err0:
	fileclose(f0);
	fileclose(f1);
	return -1;
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

int sys_thread_create(uint64 entry, uint64 arg)
{
	struct proc *p = curr_proc();
	int tid = allocthread(p, entry, 1);
	if (tid < 0) {
		errorf("fail to create thread");
		return -1;
	}
	struct thread *t = &p->threads[tid];
	t->trapframe->a0 = arg;
	t->state = RUNNABLE;
	add_task(t);
	return tid;
}

int sys_gettid()
{
	return curr_thread()->tid;
}

int sys_waittid(int tid)
{
	if (tid < 0 || tid >= NTHREAD) {
		errorf("unexpected tid %d", tid);
		return -1;
	}
	struct thread *t = &curr_proc()->threads[tid];
	if (t->state == T_UNUSED || tid == curr_thread()->tid) {
		return -1;
	}
	if (t->state != EXITED) {
		return -2;
	}
	memset((void *)t->kstack, 7, KSTACK_SIZE);
	t->tid = -1;
	t->state = T_UNUSED;
	return t->exit_code;
}

/*
*	LAB5: (3) In the TA's reference implementation, here defines funtion
*					int deadlock_detect(const int available[LOCK_POOL_SIZE],
*						const int allocation[NTHREAD][LOCK_POOL_SIZE],
*						const int request[NTHREAD][LOCK_POOL_SIZE])
*				for both mutex and semaphore detect, you can also
*				use this idea or just ignore it.
*/

static int deadlock_detect(const int available[LOCK_POOL_SIZE], // available resources
			   const int allocation[NTHREAD][LOCK_POOL_SIZE], // resources allocated to each thread
			   const int request[NTHREAD][LOCK_POOL_SIZE]) // resources requested by each thread
{
	int work[LOCK_POOL_SIZE]; // work vector to track available resources during detection
	int finish[NTHREAD]; // finish vector to track which threads can finish with current work

	// copy available -> work
	for (int j = 0; j < LOCK_POOL_SIZE; j++) {
		work[j] = available[j];
	}

	// initialize finish
	for (int i = 0; i < NTHREAD; i++) {
		finish[i] = 0;
	}

	int changed = 1; // flag to check if we can find a thread that can finish in this iteration

	// try to find a thread that can finish
	while (changed){
		changed = 0;

		// find a thread that can finish
		for (int i = 0; i < NTHREAD; i++){
			if (finish[i]) continue; // already finished

			// check if request[i] <= work
			int can_finish = 1;
			for (int j = 0; j < LOCK_POOL_SIZE; j++){
				if (request[i][j] > work[j]){
					can_finish = 0;
					break;
				}
			}

			// if thread i can finish, release its resources and mark it as finished
			if (can_finish){
				for (int j = 0; j < LOCK_POOL_SIZE; j++){
					work[j] += allocation[i][j];
				}
				finish[i] = 1;
				changed = 1;
			}
		}
	}

	// check if all finished
	for (int i =0; i < NTHREAD; i++){
		if (!finish[i]){
			return 1; // deadlock
		}
	}
	return 0; // safe
}

int sys_mutex_create(int blocking)
{
	struct proc *p = curr_proc(); // get current process
	struct mutex *m = mutex_create(blocking); // create mutex

	// error handling
	if (m == NULL) {
		errorf("fail to create mutex: out of resource");
		return -1;
	}
	// LAB5: (4-1) You may want to maintain some variables for detect here
	int mutex_id = m - p->mutex_pool; // calculate mutex id based on its position in the pool

	// initialize deadlock detection data
	p->mutex_available[mutex_id] = 1;

	// initialize allocation and request for all threads to 0
	for (int i = 0; i < NTHREAD; i++){
		p->mutex_allocation[i][mutex_id] = 0;
		p->mutex_request[i][mutex_id] = 0;
	}
	debugf("create mutex %d", mutex_id); // log mutex creation with its id
	return mutex_id; // return mutex id to user
}

// lock the mutex with given id, return 0 if success, -1 if fail, -0xDEAD if deadlock detected
int sys_mutex_lock(int mutex_id)
{
	struct proc *p = curr_proc(); // get current process
	struct thread *t = curr_thread(); // get current thread
	int tid = t->tid; // get thread id

	// error handling for invalid mutex id
	if (mutex_id < 0 || mutex_id >= curr_proc()->next_mutex_id) {
		errorf("Unexpected mutex id %d", mutex_id);
		return -1;
	}

	// mark request 
	p->mutex_request[tid][mutex_id] = 1;

	// run deadlock detection
	if (p->deadlock_detect_enabled &&
		deadlock_detect(p->mutex_available,
						p->mutex_allocation,
						p->mutex_request)){
		p->mutex_request[tid][mutex_id] = 0; // reset request
		errorf("deadlock detected on mutex %d", mutex_id); // log deadlock detection
		return -0xDEAD; // return special code for deadlock detection
	}
	// LAB5: (4-1) You may want to maintain some variables for detect
	//       or call your detect algorithm here
	
	// actually acquire lock
	mutex_lock(&p->mutex_pool[mutex_id]);

	// update state
	p->mutex_request[tid][mutex_id] = 0; // reset request
	p->mutex_available[mutex_id]--; // mark mutex as unavailable
	p->mutex_allocation[tid][mutex_id]++; // mark mutex as allocated to this thread
	return 0;
}

// unlock the mutex with given id, return 0 if success, -1 if fail
int sys_mutex_unlock(int mutex_id)
{
	struct proc *p = curr_proc(); // get current process
	struct thread *t = curr_thread(); // get current thread
	int tid = t->tid; // get thread id

	// error handling for invalid mutex id
	if (mutex_id < 0 || mutex_id >= p->next_mutex_id) {
		errorf("Unexpected mutex id %d", mutex_id);
		return -1;
	}

	// update deadlock tracking
	if (p->mutex_allocation[tid][mutex_id] > 0){
		p->mutex_allocation[tid][mutex_id]--;
		p->mutex_available[mutex_id]++;
	}
	// LAB5: (4-1) You may want to maintain some variables for detect here
	mutex_unlock(&p->mutex_pool[mutex_id]); // actually release lock
	return 0; // return success
}

// create a semaphore with given resource count, return semaphore id if success, -1 if fail
int sys_semaphore_create(int res_count)
{
	struct proc *p = curr_proc(); // get current process
	struct semaphore *s = semaphore_create(res_count); // create semaphore with given resource count

	// error handling
	if (s == NULL) {
		errorf("fail to create semaphore: out of resource");
		return -1;
	}
	// LAB5: (4-2) You may want to maintain some variables for detect here
	int sem_id = s - p->semaphore_pool; // calculate semaphore id based on its position in the pool

	// initialize deadlock detection data
	p->semaphore_available[sem_id] = res_count;

	// initialize allocation and request for all threads to 0
	for (int i = 0; i < NTHREAD; i++){
		p->semaphore_allocation[i][sem_id] = 0;
		p->semaphore_request[i][sem_id] = 0;
	}

	debugf("create semaphore %d", sem_id); // log semaphore creation with its id
	return sem_id; // return semaphore id to user
}

// release the semaphore with given id, return 0 if success, -1 if fail
int sys_semaphore_up(int semaphore_id)
{
	struct proc *p = curr_proc(); // get current process
	struct thread *t = curr_thread(); // get current thread
	int tid = t->tid; // get thread id

	// error handling for invalid semaphore id
	if (semaphore_id < 0 ||
	    semaphore_id >= p->next_semaphore_id) {
		errorf("Unexpected semaphore id %d", semaphore_id);
		return -1;
	}

	// update deadlock tracking
	if (p->semaphore_allocation[tid][semaphore_id] > 0){
		p->semaphore_allocation[tid][semaphore_id]--;
		p->semaphore_available[semaphore_id]++;
	}
	// LAB5: (4-2) You may want to maintain some variables for detect here
	semaphore_up(&p->semaphore_pool[semaphore_id]); // actually release resource
	return 0;
}

// acquire the semaphore with given id, return 0 if success, -1 if fail, -0xDEAD if deadlock detected
int sys_semaphore_down(int semaphore_id)
{
	struct proc *p = curr_proc(); // get current process
	struct thread *t = curr_thread(); // get current thread
	int tid = t->tid; // get thread id

	// error handling for invalid semaphore id
	if (semaphore_id < 0 ||
	    semaphore_id >= p->next_semaphore_id) {
		errorf("Unexpected semaphore id %d", semaphore_id);
		return -1;
	}

	// mark request
	p->semaphore_request[tid][semaphore_id] = 1;

	// run deadlock detection
	if (p->deadlock_detect_enabled &&
		deadlock_detect(p->semaphore_available,
				p->semaphore_allocation,
				p->semaphore_request)){
		p->semaphore_request[tid][semaphore_id] = 0; // reset request
		errorf("deadlock detected on semaphore %d", semaphore_id);
		return -0xDEAD;
	}

	// LAB5: (4-2) You may want to maintain some variables for detect
	//       or call your detect algorithm here

	// actually acquire resource
	semaphore_down(&p->semaphore_pool[semaphore_id]);

	// update state
	p->semaphore_request[tid][semaphore_id] = 0;
	p->semaphore_available[semaphore_id]--;
	p->semaphore_allocation[tid][semaphore_id]++;

	// testing purposes ONLY ONLY ONLY ONLY

	return 0;
}

int sys_condvar_create()
{
	struct condvar *c = condvar_create();
	if (c == NULL) {
		errorf("fail to create condvar: out of resource");
		return -1;
	}
	int cond_id = c - curr_proc()->condvar_pool;
	debugf("create condvar %d", cond_id);
	return cond_id;
}

int sys_condvar_signal(int cond_id)
{
	if (cond_id < 0 || cond_id >= curr_proc()->next_condvar_id) {
		errorf("Unexpected condvar id %d", cond_id);
		return -1;
	}
	cond_signal(&curr_proc()->condvar_pool[cond_id]);
	return 0;
}

int sys_condvar_wait(int cond_id, int mutex_id)
{
	if (cond_id < 0 || cond_id >= curr_proc()->next_condvar_id) {
		errorf("Unexpected condvar id %d", cond_id);
		return -1;
	}
	if (mutex_id < 0 || mutex_id >= curr_proc()->next_mutex_id) {
		errorf("Unexpected mutex id %d", mutex_id);
		return -1;
	}
	cond_wait(&curr_proc()->condvar_pool[cond_id],
		  &curr_proc()->mutex_pool[mutex_id]);
	return 0;
}

// LAB5: (2) you may need to define function enable_deadlock_detect here

// enable or disable deadlock detection, return 0 if success, -1 if fail
int sys_enable_deadlock_detect(int enabled)
{
	// error handling for invalid flag
	if (enabled != 0 && enabled != 1) {
		errorf("invalid deadlock detect flag %d", enabled);
		return -1;
	}
	curr_proc()->deadlock_detect_enabled = enabled; // set flag in current process
	return 0;
}

extern char trap_page[];

void syscall()
{
	struct trapframe *trapframe = curr_thread()->trapframe;
	int id = trapframe->a7, ret;
	uint64 args[6] = { trapframe->a0, trapframe->a1, trapframe->a2,
			   trapframe->a3, trapframe->a4, trapframe->a5 };
	if (id != SYS_write && id != SYS_read && id != SYS_sched_yield) {
		debugf("syscall %d args = [%x, %x, %x, %x, %x, %x]", id,
		       args[0], args[1], args[2], args[3], args[4], args[5]);
	}
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
	// case SYS_nanosleep:
	// 	ret = sys_nanosleep(args[0]);
	// 	break;
	case SYS_sched_yield:
		ret = sys_sched_yield();
		break;
	case SYS_gettimeofday:
		ret = sys_gettimeofday(args[0], args[1]);
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
	case SYS_pipe2:
		ret = sys_pipe(args[0]);
		break;
	case SYS_thread_create:
		ret = sys_thread_create(args[0], args[1]);
		break;
	case SYS_gettid:
		ret = sys_gettid();
		break;
	case SYS_waittid:
		ret = sys_waittid(args[0]);
		break;
	case SYS_mutex_create:
		ret = sys_mutex_create(args[0]);
		break;
	case SYS_mutex_lock:
		ret = sys_mutex_lock(args[0]);
		break;
	case SYS_mutex_unlock:
		ret = sys_mutex_unlock(args[0]);
		break;
	case SYS_semaphore_create:
		ret = sys_semaphore_create(args[0]);
		break;
	case SYS_semaphore_up:
		ret = sys_semaphore_up(args[0]);
		break;
	case SYS_semaphore_down:
		ret = sys_semaphore_down(args[0]);
		break;
	case SYS_condvar_create:
		ret = sys_condvar_create();
		break;
	case SYS_condvar_signal:
		ret = sys_condvar_signal(args[0]);
		break;
	case SYS_condvar_wait:
		ret = sys_condvar_wait(args[0], args[1]);
		break;
	// LAB5: (2) you may need to add case SYS_enable_deadlock_detect here
	case SYS_enable_deadlock_detect:
		ret = sys_enable_deadlock_detect(args[0]);
		break;
	default:
		ret = -1;
		errorf("unknown syscall %d", id);
	}
	curr_thread()->trapframe->a0 = ret;
	if (id != SYS_write && id != SYS_read && id != SYS_sched_yield) {
		debugf("syscall %d ret %d", id, ret);
	}
}