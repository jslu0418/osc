#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* not use in the final program */
int
running_interactive_cpu(int pipefd1[2], int pipefd2[2])
{
  int *p = calloc(1, sizeof(int));
  int ret;
  while(1)
    {
      printf("Please input a command: ");
      ret = scanf("%d", p);
      write(pipefd1[1], p, sizeof(int));
      if(*p == -1)
        {
          return 0;
        }
    }
}

int
main(int argc, char **argv)
{
  char *prog_name;
  int input_X = 0;
  if (argc > 1)
    prog_name = argv[1];
  if (argc > 2)
    input_X = atoi(argv[2]);
  pid_t pid;
  int pipefd1[2], pipefd2[2];
  int ret;
  ret = pipe(pipefd1);
  if (ret == -1)
    {
      printf("Error at create pipe\n");
      exit(-1);
    }
    ret = pipe(pipefd2);
  if (ret == -1)
    {
      printf("Error at create pipe\n");
      exit(-1);
    }
  pid = fork();
  switch(pid)
    {
    case -1:
      printf("Error at create child process\n");
      exit(-1);
    case 0:
      init_memory(prog_name, pipefd1, pipefd2);
      exit(0);
    default:
      /* printf("This is parent process.\n"); */
      init_cpu(pipefd1, pipefd2);
      running_cpu(input_X);
    }
  waitpid(-1, NULL, 0);
  /* printf("Offline CPU.\n"); */
  return 0;
}
