#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define SYSTEM_CALL_ADDR 1500
#define TIMER_ADDR 1000
#define USER_STACK_ADDR 999
#define SYSTEM_STACK_ADDR 1999

int PC = 0;
int SP = USER_STACK_ADDR;
int IR = 0;
int AC = 0;
int X = 0;
int Y = 0;
int mode = 0; /* 0: user, 1: system */
int g_pipefd1[2], g_pipefd2[2];

int cpu_read_mem (int addr); /* send read request to mem */
int cpu_write_mem (int addr, int data); /* send write request to mem */
void load_value();
void load_addr();
void load_ind_addr();
void load_idx_x_addr();
void load_idx_y_addr();
void load_sp_x();
void store_addr();
void get_rand();
void put_port();
void add_x();
void add_y();
void sub_x();
void sub_y();
void copy_to_x();
void copy_from_x();
void copy_to_y();
void copy_from_y();
void copy_to_sp();
void copy_from_sp();
void jump_addr();
void jump_ifeq_addr();
void jump_ifne_addr();
void call_addr();
void ret();
void inc_x();
void dec_x();
void push();
void pop();
void itr();
void i_ret(); /* 30 */
void end(); /* 50 */
void timer();
void fetch();
void execute();
int init_cpu(int pipefd1[2], int pipefd2[2]);
int running_cpu(int input_X);

void (*oper[51])(void) = {NULL,
                         load_value,
                         load_addr,
                         load_ind_addr,
                         load_idx_x_addr,
                         load_idx_y_addr,
                         load_sp_x,
                         store_addr,
                         get_rand,
                         put_port,
                         add_x,
                         add_y,
                         sub_x,
                         sub_y,
                         copy_to_x,
                         copy_from_x,
                         copy_to_y,
                         copy_from_y,
                         copy_to_sp,
                         copy_from_sp,
                         jump_addr,
                         jump_ifeq_addr,
                         jump_ifne_addr,
                         call_addr,
                         ret,
                         inc_x,
                         dec_x,
                         push,
                         pop,
                         itr,
                         i_ret,
                         NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, end};

int
cpu_read_mem (int addr)
{
  if (mode == 0 && addr > 999)
    printf("Memory violation: accessing system address %d in user mode\n", addr);
  addr = (addr << 20) & 0x7FF00000;
  write(g_pipefd1[1], &addr, sizeof(int));
  read(g_pipefd2[0], &addr, sizeof(int));
  return addr;
}

int
cpu_write_mem (int addr, int data)
{
  if (mode == 0 && addr > 999)
    printf("Memory violation: accessing system address %d in user mode\n", addr);
  addr = ((addr << 20) & 0x7FF00000) | (0x80000000 |( data & 0x000FFFFF));
  write(g_pipefd1[1], &addr, sizeof(int));
  return addr;
}

/* 1 */
void
load_value ()
{
  AC = cpu_read_mem(PC);
  PC += 1;
}

/* 2 */
void
load_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  AC = cpu_read_mem(a);
}

/* 3 */
void
load_ind_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  a = cpu_read_mem(a);
  AC = cpu_read_mem(a);
}

/* 4 */
void
load_idx_x_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  AC = cpu_read_mem(a + X);
}

/* 5 */
void
load_idx_y_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  AC = cpu_read_mem(a + Y);
}

/* 6 */
void
load_sp_x ()
{
  AC = cpu_read_mem(SP + X);
}

/* 7 */
void
store_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  cpu_write_mem(a, AC);
}

/* 8 */
void
get_rand ()
{
  srand(time(NULL));
  AC = rand()%100 + 1;
}

/* 9 */
void
put_port ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  if (a == 1)
    printf("%d", AC);
  else
    printf("%c", AC);
}

/* 10 */
void
add_x ()
{
  AC += X;
}

/* 11 */
void
add_y ()
{
  AC += Y;
}

/* 12 */
void
sub_x ()
{
  AC -= X;
}

/* 13 */
void
sub_y ()
{
  AC -= Y;
}

/* 14 */
void
copy_to_x ()
{
  X = AC;
}

/* 15 */
void
copy_from_x ()
{
  AC = X;
}

/* 16 */
void
copy_to_y ()
{
  Y = AC;
}

/* 17 */
void
copy_from_y ()
{
  AC = Y;
}

/* 18 */
void
copy_to_sp ()
{
  SP = AC;
}

/* 19 */
void
copy_from_sp ()
{
  AC = SP;
}

/* 20 */
void
jump_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  PC = a;
}

/* 21 */
void
jump_ifeq_addr ()
{
  int a;
  a = cpu_read_mem(PC);
  PC += 1;
  if (AC == 0){
    PC = a;
  }
}

/* 22 */
void
jump_ifne_addr ()
{
  int a;
  a = cpu_read_mem(PC);
  PC += 1;
  if (AC != 0){
    PC = a;
  }
}

/* 23 */
void
call_addr ()
{
  int a = cpu_read_mem(PC);
  PC += 1;
  SP -= 1;
  cpu_write_mem(SP, PC);
  PC = a;
}

/* 24 */
void
ret ()
{
  int a = cpu_read_mem(SP);
  SP += 1;
  PC = a;
}

/* 25 */
void
inc_x ()
{
  X += 1;
}

/* 26 */
void
dec_x ()
{
  X -= 1;
}

/* 27 */
void
push ()
{
  SP -= 1;
  cpu_write_mem(SP, AC);
}

/* 28 */
void
pop ()
{
  AC = cpu_read_mem(SP);
  SP += 1;
}

/* 29 */
void
itr ()
{
  if (mode == 1)
    return;
  mode = 1;
  cpu_write_mem(SYSTEM_STACK_ADDR - 1, SP);
  SP = SYSTEM_STACK_ADDR - 2;
  cpu_write_mem(SP, PC);
  PC = SYSTEM_CALL_ADDR;
}

/* 30 */
void
i_ret ()
{
  PC = cpu_read_mem(SP);
  SP += 1;
  SP = cpu_read_mem(SP);
  mode = 0;
}

/* 50 */
void
end ()
{
  int a = -1;
  write(g_pipefd1[1], &a, sizeof(int)); /* offline the memory */
  IR = -1; /* set IR = -1 to offline the cpu */
}

/* timer */
void
timer()
{
  if (mode == 1)
    return;
  mode = 1;
  cpu_write_mem(SYSTEM_STACK_ADDR - 1, SP);
  SP = SYSTEM_STACK_ADDR - 2;
  cpu_write_mem(SP, PC);
  PC = TIMER_ADDR;
}

/* fetch */
void
fetch ()
{
  /* printf("fetch instruction at %d\n", PC); */
  IR = cpu_read_mem(PC);
  PC += 1;
}

/* execute */
void
execute ()
{
  oper[IR]();
  /* printf("PC: %d, IR: %d, AC: %d, X: %d, Y: %d, SP: %d\n", */
  /*        PC, IR, AC, X, Y, SP); */
}

/* init cpu */
int
init_cpu(int pipefd1[2], int pipefd2[2])
{
  g_pipefd1[0] = pipefd1[0];
  g_pipefd1[1] = pipefd1[1];
  g_pipefd2[0] = pipefd2[0];
  g_pipefd2[1] = pipefd2[1];
  SP = USER_STACK_ADDR;
  PC = 0;
  IR = 0;
  AC = 0;
  X = 0;
  Y = 0;
}

/* running cpu */
int
running_cpu(int input_X)
{
  int *p = calloc(1, sizeof(int));
  read(g_pipefd2[0], p, sizeof(int));
  int i;
  if (*p != 0)
    return -1;
  i = 0;
  while(IR != -1)
    {
      fetch();
      execute();
      if (mode==0)
        i++;
      if (input_X > 1 && i >= input_X)
        {
          timer();
          i = 0;
        }
    }
  return 0;
}
