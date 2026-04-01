#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/mman.h>
#include <sys/types.h> //Includes pid_t (A type that contains PID)
#include <sys/wait.h> //Includes wait and waitpid function
#include <time.h> //Include features related to time
#include <signal.h>
#include <stdbool.h>
#include <errno.h>