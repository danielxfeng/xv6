#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// To implement the trace, some changes should be also made:
// 1 modify kernel/proc.h to add a property of trace_mask.
// 2 modify kernel/syscall.h to add an index.
// 3 modify user/user.h to add the declaration.
// 4 modify user/usys.pl to add an entry item.
// 5 modify kernel/sysproc.c to add a function sys_trace.
// 6 modify kernel/syscall.c to add an item to syscalls, add an array names, add printf command.。
// 7 modify kernel/proc.c to modify function fork, to copy trace_mask from parent proc to child proc.。

int
main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if (trace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}
