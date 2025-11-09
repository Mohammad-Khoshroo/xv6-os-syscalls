#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

.globl simp_arith
simp_arith:
  pushl %ebp
  movl %esp, %ebp
  movl 8(%ebp), %ebx    # a
  movl 12(%ebp), %ecx   # b
  movl $SYS_simp_arith, %eax
  int $T_SYSCALL
  movl %ebp, %esp
  popl %ebp
  ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
SYSCALL(grep_sys)
SYSCALL(show_process_family)
SYSCALL(make_duplicate)
SYSCALL(set_priority)