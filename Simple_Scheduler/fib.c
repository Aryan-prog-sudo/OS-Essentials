#include <stdio.h>
#include <unistd.h>

// This is the crucial include. It will redefine our main() function
// and handle stopping the process so the scheduler can control it.
#include "dummy_main.h"

// Due to the "#define main dummy_main" in dummy_main.h,
// this function is now actually named "dummy_main".
// The real entry point is the main() function inside dummy_main.h.

int fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(int argc, char *argv[]) {
    printf("%d\n", fib(40));
    return 0;
}