#include "sysfs_writer.h"

extern pid_t pid;
static const char * sysfsPath = "/sys/kernel/qznusnih/set_address_to_debug";

int write_to_sysfs(volatile uint8_t * addrToWatch) {
    FILE * sysfs = fopen(sysfsPath, "w");
    if (!sysfs) {
        fprintf(stderr, "write_to_sysfs attemped to open for writing the file %s and failed: %s\n", sysfsPath, strerror(errno));
        return -1;
    }

    if (fprintf(sysfs, "%#lx %d\n", (unsigned long) addrToWatch, (int) pid) < 0) {
        fprintf(stderr, "write_to_sysfs attempted to write \"%#lx %d\" to %s and failed: %s", (unsigned long) addrToWatch, (int) pid, sysfsPath, strerror(errno));
        fclose(sysfs);
        return -1;
    } else {
        printf(" wrote \"%#lx %d\" to %s successfully\n", (unsigned long) addrToWatch, (int) pid, sysfsPath);
    }
    fflush(sysfs);
    fclose(sysfs);
    return 0;
}
