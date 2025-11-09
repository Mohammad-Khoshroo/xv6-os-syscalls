#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char **argv) {
  if (argc < 3) {
    printf(1, "usage: test_grep <keyword> <filename>\n");
    exit();
  }
  char buf[256];
  int n = grep_syscall(argv[1], argv[2], buf, sizeof(buf));
  if (n < 0) {
    printf(1, "not found or error\n");
  } else {
    printf(1, "found: %s\n", buf);
  }
  exit();
}