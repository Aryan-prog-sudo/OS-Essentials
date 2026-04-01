/*
 * No changes are allowed to this file
 */
#define _GNU_SOURCE//just unhiding all the special types
#include<math.h>//only for ceil function
#include<signal.h>
#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>

void load_and_run_elf(char** exe);
void loader_cleanup();