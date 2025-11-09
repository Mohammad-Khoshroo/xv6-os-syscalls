#include "types.h"
#include "stat.h"
#include "user.h"

#define WORK 80000000

void busy(int id, int work){
  volatile int i;
  for(i=0;i<work;i++) { }  
  printf(1, "child %d done\n", id);
}

int
main(void)
{
  int pid1 = fork();
  if(pid1 == 0){ printf(1, "child1 start\n"); busy(1, WORK); exit(); }

  int pid2 = fork();
  if(pid2 == 0){ printf(1, "child2 start\n"); busy(2, WORK); exit(); }

  set_priority(pid1, 0);
  set_priority(pid2, 2);

  wait();
  wait();
  exit();
}
