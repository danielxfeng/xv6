#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[]) {
    int ping[2];
    int pong[2];
    if (pipe(ping) == -1) {
        fprintf(2, "create pipe ping error.\n");
        exit(1);
    }
    if (pipe(pong) == -1) {
        fprintf(2, "create pipe pong error.\n");
        exit(1);
    }
    if (fork() == 0) // child thread;
    {
        int buf;
        if (read(ping[0], &buf, sizeof(int)) == -1) {
            fprintf(2, "read ping error.\n");
            exit(1);
        }
        close(ping[0]);
        if (write(pong[1], &buf, sizeof(int)) == -1) {
            fprintf(2, "write pong error.\n");
            exit(1);
        }
        close(pong[1]);
        printf("%d: received ping\n", buf);
        exit(0);
    }else{			// main thread;
        int buf;
        int pid = getpid();
        if (write(ping[1], &pid, sizeof(int)) == -1) {
            fprintf(2, "write ping error.\n");
            exit(1);
        }
        close(ping[1]);
        wait(0);
        if (read(pong[0], &buf, sizeof(int)) == -1) {
            fprintf(2, "read pong error.\n");
            exit(1);
        }
        close(pong[0]);
        printf("%d: received pong\n", pid);
        exit(0);
    }
}