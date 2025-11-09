#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


int sys_simp_arith(void) {
  
  struct trapframe *tf = myproc()->tf; 
  
  int a = tf->ebx;
  int b = tf->ecx;
  
  cprintf("First  arg a (from EBX): %d\n", a);
  cprintf("Second arg b (from ECX): %d\n", b);
  
  return simp_arith(a, b);
}


int
sys_show_process_family(void)
{
  int pid;
  if (argint(0, &pid) < 0) return -1;
  return show_process_family(pid);
}

int
sys_grep_sys(void)
{
  char* keyword; 
  char* filename;
  char* user_buf;
  int buf_size;

  if (argstr(0, &keyword) < 0) return -1;
  if (argstr(1, &filename) < 0) return -1;
  if (argptr(2, &user_buf, sizeof(char*)) < 0) return -1;
  if (argint(3, &buf_size) < 0) return -1;

  return grep_sys(keyword, filename, user_buf, buf_size);
}

int
sys_set_priority(void)
{
  int pid, priority;
  if (argint(0, &pid) < 0) return -1;
  if (argint(1, &priority) < 0) return -1;
  return set_priority(pid, priority);
}
