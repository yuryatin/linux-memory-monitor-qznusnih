#ifndef SYSFS_WRITER_H_qznusnih
#define SYSFS_WRITER_H_qznusnih

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/random.h>

int write_to_sysfs(volatile uint8_t * addrToWatch);

#endif