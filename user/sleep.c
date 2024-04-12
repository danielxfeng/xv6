#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    int timer;
    if (argc < 2) {
        write(1, "Please input number of ticks\n", 28);
        exit(1);
    }
    timer = atoi(argv[1]);
    sleep(timer);
    exit(0);
}