#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

__attribute__((noreturn))
void
prime_helper(int self, int p[2]) {
    printf("prime %d\n", self);
    int value;
    int has_next = 0; // if the child process created.
    int np[2];
    close(p[1]);
    if (pipe(np) == -1) { // create the pipe to its child.
        fprintf(2, "create pipe error, self = %d.\n", self);
        exit(1);
    }
    while (1) { // iterate to read from pipe.
        int result = read(p[0], &value, sizeof(int));
        if (result == 0) { // writer of the pipe to parent is closed.
            close(p[0]); // close the reader of the pipe to parent.
            close(np[1]); // close the writer of the pipe to child.
            wait(0);
            exit(0);
        }
        if (result == -1) { // pipe to parent error.
            fprintf(2, "read from pipe error, self is %d.\n", self);
            close(np[1]);
            wait(0);
            exit(1);
        }
        if (value <= self || value % self == 0) { // drop the smaller data or not prime.
            continue;
        }
        if (!has_next && fork() == 0) { // create child process when not existed.
            prime_helper(value, np);
        } else {
            if (!has_next) { // when child process has just been created.
                close(np[0]);
                has_next = 1;
            }
            if (write(np[1], &value, sizeof(int)) == -1) { // pass the value to child.
                fprintf(2, "write to pipe error, self: %d, value: %d.\n", self, value);
                close(np[1]);
                wait(0);
                exit(1);
            }
        }
    }
}

int
main(int argc, char *argv[]) {
    int p[2];
    int value = 3; // start from 3, because we dealt with 2 separately.
    if (pipe(p) == -1) { // create the initial pipe.
        fprintf(2, "create pipe error.\n");
        exit(1);
    }
    if (fork() == 0) { // process 2.
        prime_helper(2, p);
    } else { // main process.
        close(p[0]);
        while (value <= 35) { // iterate to send value until 35.
            if (write(p[1], &value, sizeof(int)) == -1) {
                fprintf(2, "write to pipe error, value = %d.\n", value);
                close(p[1]);
                exit(1);
            }
            value++;
        }
        close(p[1]); // close the pipe.
        wait(0);
        exit(0);
    }
}
