#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int main(int argc, char *argv[]) {
    char* args[MAXARG];
    char ch;
    char line[512];
    if (argc < 2) {
        fprintf(2, "Please input parameters.\n");
        exit(1);
    }
    for (int i = 1; i < argc; i++) { // assemble the args from parameters.
        args[i - 1] = argv[i];
    }
    // ‘\0’ or 0 is the stop sign. args[argc - 2] will be set in "args[argc - 1] = line".
    args[argc - 1] = 0;
    int i = 0;
    while (1) {
        if (i >= sizeof(line) - 1) {
            fprintf(2, "line is too long.");
            break;
        }
        if (read(0, &ch, sizeof(char)) == -1) { // iterate the output of fore command.
            fprintf(2, "read from pipe error\n");
            exit(1);
        }
        if (i == 0 && ch == '\n') { // quit when encounter an empty '\n'.
            break;
        }
        if (ch == '\n') { // '\n' is the sign of executing the cmd.
            line[i] = '\0'; // set the '\0' sign.
            i = 0; // reset the index.
            if (fork() == 0) { // child process.
                args[argc - 1] = line; // fill the reserved position.
                exec(args[0], args); // exec the cmd.
                fprintf(2, "exec failed\n"); // normally, exec will not return.
                exit(1);
            } else {
                wait(0);
            }
        } else {
            line[i] = ch; // normally, assemble the parameter from output of fore command.
            i++;
        }
    }
    exit(0);
}
