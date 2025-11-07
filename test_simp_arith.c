#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char* argv[])
{
    int a = 5;
    int b = 3;
    int result;
    
    printf(1, "Testing simple_arithmetic_syscall...\n");
    
    printf(1, "\n--- Test 1 ---\n");
    printf(1, "Input: a=%d, b=%d\n", a, b);
    printf(1, "Expected: (5+3)*(5-3) = 8*2 = 16\n\n");
    
    result = simp_arith(a, b);
    
    printf(1, "Result returned from syscall: %d\n", result);
    
    printf(1, "\n--- Test 2 ---\n");
    a = 10;
    b = 4;
    printf(1, "Input: a=%d, b=%d\n", a, b);
    printf(1, "Expected: (10+4)*(10-4) = 14*6 = 84\n");
    result = simp_arith(a, b);
    printf(1, "Result: %d\n", result);
    
    printf(1, "\n--- Test 3 ---\n");
    a = 7;
    b = 7;
    printf(1, "Input: a=%d, b=%d\n", a, b);
    printf(1, "Expected: (7+7)*(7-7) = 14*0 = 0\n");
    result = simp_arith(a, b);
    printf(1, "Result: %d\n", result);
    
    exit();
}