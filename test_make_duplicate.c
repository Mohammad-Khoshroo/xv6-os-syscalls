
#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  if(argc != 2){
    printf(2, "usage: duplicate <src>\n");
    exit();
  }
  int r = make_duplicate(argv[1]);
  if(r == 0){
    printf(1, "duplicated %s -> %s_copy\n", argv[1], argv[1]);
  } else if (r == -1){
    printf(2, "ERR: source file not found: %s\n", argv[1]);
  } else {
    printf(2, "ERR: duplicate failed for %s\n", argv[1]);
  }
  exit();
}
