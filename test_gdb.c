#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char* argv[])
{
    int pid;

    printf(1, "Calling getpid()...\n");
    pid = getpid();
    printf(1, "My PID is: %d\n", pid);

    exit();
}