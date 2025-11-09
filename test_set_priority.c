// test_priority.c
#include "types.h"
#include "stat.h"
#include "user.h"

#define WORK 100000000

void busy(int id, int work) {
  volatile long i;
  for(i = 0; i < work; i++){
    // do nothing but keep CPU busy
  }
  printf(1, "child %d done\n", id);
}

int main(int argc, char *argv[]) {
  int pid1, pid2;
  int wpid;

  pid1 = fork();
  if(pid1 < 0) {
    printf(1, "fork failed\n");
    exit();
  }
  if(pid1 == 0) {
    // child 1
    busy(1, WORK);
    exit();
  }

  pid2 = fork();
  if(pid2 < 0) {
    printf(1, "fork failed\n");
    exit();
  }
  if(pid2 == 0) {
    // child 2
    busy(2, WORK);
    exit();
  }

  // parent: set priorities -- make child1 high, child2 low
  if(set_priority(pid1, 0) < 0) {
    printf(1, "set_priority failed for %d\n", pid1);
  }
  if(set_priority(pid2, 2) < 0) {
    printf(1, "set_priority failed for %d\n", pid2);
  }

  // wait for children
  while((wpid = wait()) >= 0){
    // parent prints in wait return or you can print here
  }

  exit();
}
