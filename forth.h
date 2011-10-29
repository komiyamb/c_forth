#ifndef __KOMFORTH__
#define __KOMFORTH__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TRUE  1
#define FALSE 0

#ifdef DEBUG
#  define dprintf(args) fflush(stdout);fputs("debug:", stdout);printf args
#else
#  define dprintf(args) 
#endif

#define FORTH_LIBRARY_NAME "forth.lib"

#define FORTH_WORD_NAME_SIZE 16
#define STDIN_SIZE 256
#define FORTH_DICT_SIZE    1024
#define FORTH_STACK_SIZE    256
#define FORTH_THREAD_SIZE  4096
#define NAME_SPACE_SIZE   65536

#define TYPE_PRIM   0x00
#define TYPE_FIXNUM 0x01
#define TYPE_THREAD 0x02
#define TYPE_STRING 0x03
#define TYPE_MASK 0x03

typedef struct forth_word forth_word;
typedef union forth_exp forth_exp;
typedef struct stack forth_stack;

struct forth_word{
  char* name;
  int immediate;
  forth_exp* thread;
};

union forth_exp{
  int num;
  forth_exp* (*prim)(void);
  forth_exp* thread;
  char* c;
};

#define FORTH_THREAD_END TYPE_THREAD

int get_type(forth_exp e)
{
  return e.num & TYPE_MASK;
}

int get_num(forth_exp e)
{
  return e.num >> 2;
}

void set_num(forth_exp* e, int num)
{
  e->num = (num << 2) | TYPE_FIXNUM;
}

int num_p(forth_exp e)
{
  return get_type(e) == TYPE_FIXNUM;
}

char* get_sym(forth_exp e)
{
  return (char*)((int)e.c & ~TYPE_MASK);
}

void set_sym(forth_exp* e, char* v)
{
  e->c = (char*)((int)v | TYPE_STRING);
}
int sym_p(forth_exp e)
{
  return get_type(e) == TYPE_STRING;
}

typedef void (*type_gp)(void);
type_gp get_prim(forth_exp e)
{
  return (type_gp)((int)e.prim & ~TYPE_MASK);
}

void set_prim(forth_exp* e, type_gp f)
{
  e->prim = (type_gp)((int)f | TYPE_PRIM);
}

int prim_p(forth_exp pc)
{
  return get_type(pc) == TYPE_PRIM;
}

void exec_prim(forth_exp* pc)
{
  type_gp tmp = get_prim(*pc);
  tmp();
}

forth_exp* get_thread(forth_exp e)
{
  return (forth_exp*)((int)e.thread & ~TYPE_MASK);
}

void set_thread(forth_exp* e, forth_exp* th)
{
  e->thread = (forth_exp*)((int)get_thread((forth_exp)th) | TYPE_THREAD);
}

int thread_p(forth_exp pc)
{
  return get_type(pc) == TYPE_THREAD;
}

void set_end(forth_exp* e)
{
  e->num = FORTH_THREAD_END;
}

int pc_end_p(forth_exp pc)
{
  return pc.num == FORTH_THREAD_END;
}

struct stack{
  forth_exp stack[FORTH_STACK_SIZE];
  int index;
};

void init_stack(forth_stack* st)
{
  st->index = 0;
}

forth_exp pop(forth_stack* st)
{
  forth_exp ret;
  if(st->index <= 0){
    fprintf(stderr, "stack under flower\n");
    exit(1);
  }
  st->index--;
  ret = st->stack[st->index];
  //  dprintf(("poped\n"));
  return ret;
}

void push(forth_exp obj, forth_stack* st)
{
  if(st->index >= FORTH_STACK_SIZE){
    fprintf(stderr, "stack over flower\n");
    exit(1);
  }
  st->stack[st->index] = obj;
  st->index++;
  //  dprintf(("pushed\n"));
}

forth_exp peek(forth_stack* st)
{
  return st->stack[st->index - 1];
}

void forth_compile_in(forth_exp exp);
void print_forth_exp(forth_exp* e);
void print_thread(void);
void print_stack(forth_stack* st);
void print_dict(void);
void my_forth(char* v);
void forth_install_stdlib(void);

#endif /* __KOMFORTH__ */
