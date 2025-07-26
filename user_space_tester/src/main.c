#include "sysfs_writer.h"

#define N_BYTES_TO_WATCH 2
#define BYTE 1

pid_t pid;  // own process ID
static volatile uint8_t theData[N_BYTES_TO_WATCH] = {0};

int main(void) {
    struct timespec pause = {
        .tv_sec = 0,
        .tv_nsec = 10000
    };
    pid = getpid();
    printf("\tqznusnih_user_space_tester started with PID %d\n", pid);
    uint8_t buf;

    for (int i = 0; i < N_BYTES_TO_WATCH; ++i) {
        if(write_to_sysfs(&theData[i])) return -1;
        nanosleep(&pause, NULL);
        ssize_t ret = getrandom(&buf, BYTE, 0);
        if (ret != BYTE) {
            fprintf(stderr, "getrandom failed to read a byte: %zd\n", ret);
            return -1;
        }
        theData[i] = buf;
        printf("new theData[%d] = %02x\n", i, (unsigned)theData[i]); // this print also triggers a read event 
    }
    return 0;
}