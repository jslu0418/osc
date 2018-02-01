/* This file implements memory's functions
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_ADDR 1999
#define STRING_LENGTH 1024

typedef struct mem{
  int a[MAX_ADDR+1];
} *ptr_mem;

int
read_mem (int addr, ptr_mem mem)
{
  return mem->a[addr];
}

int
write_mem (int addr, ptr_mem mem, int data)
{
  if ((data & 0x00080000) != 0)
    data = data | 0xFFF00000;
  mem->a[addr] = data;
}

int
load_program(ptr_mem mem, char *filename)
{
  int i;
  int data;
  int addr = -1;
  char str_buf[STRING_LENGTH];
  FILE *prog_txt;
  prog_txt = fopen (filename, "r");
  if (prog_txt == NULL)
    {
      printf ("File could not be opened.\n");
      return -1;
    }
  else
    {
      i = 0;
      while (fgets(str_buf, STRING_LENGTH, prog_txt) != NULL)
        {
          if (str_buf[0] == '.') {
            addr = atoi((char *)(str_buf+1));
            if (addr != -1)
              {
                i = addr;
                continue;
              }
          }
          data = atoi(str_buf);
          if (str_buf[0] != '0' && data == 0)
            continue;
          mem->a[i++] = data;
        }
    }
  return i;
}

int
print_program(ptr_mem mem)
{
  int i;
  for (i=0; i<=MAX_ADDR; i++)
    {
      printf("%d: %d\n", i, mem->a[i]);
    }
  return 0;
}

int
run_memory(ptr_mem mem, int pipefd1[2], int pipefd2[2])
{
  int *p = calloc(1, sizeof(int));
  printf("Memory is online now:\n");
  *p = 0;
  write(pipefd2[1], p, sizeof(int));

  while(read(pipefd1[0], p, sizeof(int)) != -1)
    {
      /* printf("Receive request: %d\n", *p); */
      if (*p == -1)
        {
          /* printf("Offline memory\n");*/
          return 0;
        }
      if ((*p & 0x80000000) == 0)
        {
          /* postive intger -> read -> 11bit address */
          *p = read_mem((*p & 0x7FF00000) >> 20, mem);
          write(pipefd2[1], p, sizeof(int));
        }
      else
        {
          /* negative integer -> write -> 11bit address and 20bit data */
          write_mem((*p & 0x7FF00000) >> 20, mem, (*p & 0x000FFFFF));
        }
    }
}

int
init_memory (char *filename, int pipefd1[2], int pipefd2[2])
{
  ptr_mem p_mem = calloc(1, sizeof(struct mem));
  int i;
  int ret;
  i = load_program(p_mem, filename);
  /* print_program(p_mem); /\* debug for print loaded program *\/ */
  ret = run_memory(p_mem, pipefd1, pipefd2);
  return 0;
}
