#include "forth.h"

int compiling = FALSE;
forth_exp* pc;
forth_stack rstack, pstack;

forth_word dict[FORTH_DICT_SIZE];
int dict_index = 0;

forth_exp forth_thread_code[FORTH_THREAD_SIZE];
int thread_index = 0;

char name_space[NAME_SPACE_SIZE];
int ns_index = 0;
char* lookup_symbol(char* name)
{
  int i;
  for(i = 0; i < ns_index; i++){
    if(!strcmp(name, name_space + i)){
	return name_space + i;
    }
  }
  return NULL;
}
char* add_name(char* name)
{
  char* ret = name_space + ns_index;
  int i, tmp = 0;

  for(i = 0; i < ns_index; i += tmp){
    if(!strcmp(name, name_space + i)){
      return name_space + i;
    }
    tmp = strlen(name_space + i) + 1;
  }

  tmp = (strlen(name) + 4) / 4;
  ns_index += tmp * 4;

  if(ns_index >= NAME_SPACE_SIZE){
    fprintf(stderr, "name space over flower\n");
    exit(1);
  }

  strcpy(ret, name);
  dprintf(("add_name(%s)(%p)\n", ret, ret));
  return ret;
}

forth_exp str_to_exp(char* str)
{
  int tmp = atoi(str);
  forth_exp ret;

  if((str[0] != '0') && (tmp == 0)){//string
    set_sym(&ret, add_name(str));
  }else{//num
    set_num(&ret, tmp);
  }
  return ret;
}

forth_word* forth_lookup(char* w)
{
  int i;
  for(i = dict_index - 1; i >= 0; i--){
    dprintf(("compare(%s),(%s)\n",w, dict[i].name));
    if(!strcmp(w, dict[i].name)){
      return dict + i;
    }
  }
  return NULL;
}

char* forth_dict_name(forth_exp* exp)
{
  int i;
  for(i = dict_index - 1; i >= 0; i--){
    forth_exp *tmp = dict[i].thread;
    //dprintf(("comp(%p),(%p)\n", dict[i].thread, exp));
    if(tmp == exp ||
	(function_p(*tmp) && function_p(*exp)
	 && (get_prim(*tmp) == get_prim(*exp)))){
      return dict[i].name;
    }
  }
  return "NULL";
}
void forth_inner_interpreter(void)
{
  //  dprintf(("begin inner_interpreter\n"));
  do{
     dprintf(("before exec.pc=%p,rstack.index=%d\n", (int*)pc, rstack.index));
    if(pc_end_p(*pc)){
      pc = get_thread(pop(&rstack));
    }else if(function_p(*pc)){
      dprintf(("function_p\n"));
      exec_prim(pc);
    }else if(thread_p(*pc)){
      forth_exp tmp;
      set_thread(&tmp, pc + 1);
      dprintf(("thread_p\n"));
      push(tmp, &rstack);
      pc = get_thread((forth_exp)pc->thread);
    }else{
      dprintf(("push pstack\n"));
      push(*pc, &pstack);
      pc++;
    }
    dprintf(("after exec.pc=%p, pc_end_p=(%d),rstack.index=%d\n", 
	     (int*)pc, pc_end_p(*pc), rstack.index));
  }while((!pc_end_p(*pc)) || (rstack.index != 0));
  //  dprintf(("end innner_interpreter\n"));
}

void add_forth_prim(char* name, int immediate, void (*func)())
{
  forth_exp tmp;

  dict[dict_index].name = add_name(name);
  dict[dict_index].immediate = immediate;
  dict[dict_index].thread = forth_thread_code + thread_index;
  dprintf(("add(%s)pointer=%x\n", name, (int)func));

  if(thread_index + 2 >= FORTH_THREAD_SIZE){
    fprintf(stderr, "too few threaded code memory\n");
    exit(1);
  }

  set_prim(&tmp, func);
  forth_thread_code[thread_index] = tmp;
  thread_index++;
  set_end(forth_thread_code + thread_index);
  thread_index++;

  dict_index++;
  if(dict_index >= FORTH_DICT_SIZE){
    fprintf(stderr, "dict over flower\n");
    exit(1);
  }
}

void F_nop(void)
{
  pc++;
}

void F_mul(void) /* * */
{
  int arg1, arg2;
  forth_exp ret;
  arg1 = get_num(pop(&pstack));
  arg2 = get_num(pop(&pstack));
  set_num(&ret, arg1 * arg2);
  push(ret, &pstack);
  pc++;
}

void F_drop(void)
{
  pop(&pstack);
  pc++;
}

void F_dup(void)
{
  forth_exp d = peek(&pstack);
  push(d, &pstack);
  pc++;
}

void F_swap(void)
{
  forth_exp arg1, arg2;
  arg1 = pop(&pstack);
  arg2 = pop(&pstack);
  push(arg1, &pstack);
  push(arg2, &pstack);
  pc++;
}

void F_print(void)
{
  forth_exp e = pop(&pstack);
  dprintf(("print(%x)\n", e.num));
  if(get_type(e) == TYPE_FIXNUM){
    printf("%d\n", get_num(e));
  }else if (get_type(e) == TYPE_STRING){
    printf("%s\n", get_sym(e));
  }else{
    printf("#<unprintable %x>\n", e.num & ~TYPE_MASK);
  }
  dprintf(("print end\n"));
  pc++;
}

void F_to_r(void) /* >r */
{
  forth_exp tmp = pop(&pstack);
  push(tmp, &rstack);
  pc++;
}

void F_r_to(void) /* r> */
{
  forth_exp tmp = pop(&rstack);
  push(tmp, &pstack);
  pc++;
  dprintf(("r>p\n"));
}

void F_l_bracket(void) /* [ */
{
  compiling = FALSE;
  dprintf(("([)compiling = FALSE\n"));
  pc++;
}

void F_r_bracket(void) /* ] */
{
  compiling = TRUE;
  dprintf(("(])compiling = TRUE\n"));
  pc++;
}

void F_create(void)
{
  forth_exp tmp;
  dprintf(("create! thread=(%p)\n", forth_thread_code + thread_index));
  thread_index++;

  dict[dict_index].name = "";
  dict[dict_index].immediate = FALSE;
  dict[dict_index].thread = forth_thread_code + thread_index;

  set_thread(&tmp, forth_thread_code + thread_index + 2);
  forth_compile_in(tmp);
  thread_index++;
  set_end(forth_thread_code + thread_index);

  dict_index++;
  if(dict_index >= FORTH_DICT_SIZE){
    fprintf(stderr, "dict over flower\n");
    exit(1);
  }
  pc++;
}

void F_name(void)
{
  dict[dict_index - 1].name = get_sym(pop(&pstack));
  pc++;
}

void F_immediate(void)
{
  dict[dict_index - 1].immediate = TRUE;
  pc++;
}

void F_fetch(void) /* @ */
{
  forth_exp ret, tmp = get_thread(pop(&pstack));
  set_thread(&ret, *tmp);
  push(ret, &pstack);
  pc++;
}

void F_store(void)/* ! */
{
  forth_exp tmp1 = pop(&pstack),
    tmp2 = pop(&pstack),
    *loc = get_thread(tmp1),
    *thread = get_thread(tmp2);
  //print_thread();
  dprintf(("loc=(%p),loc->(%p),th=(%p),th->(%p)\n",tmp1,loc,tmp2,thread));
  set_thread(loc, thread);
  dprintf(("loc=(%p),loc->(%p),th=(%p),th->(%p)\n",tmp1,loc,tmp2,thread));
  pc++;
}

void F_not(void)
{
  forth_exp tmp = pop(&pstack);
  set_num(&tmp, !get_num(tmp));
  push(tmp, &pstack);
  pc++;
}

void F_oddp(void)
{
  forth_exp tmp = pop(&pstack);
  set_num(&tmp, !(get_num(tmp) % 2));
  push(tmp, &pstack);
  pc++;
}

void F_evenp(void)
{
  forth_exp tmp = pop(&pstack);
  set_num(&tmp, get_num(tmp) % 2);
  push(tmp, &pstack);
  pc++;
}

#define def_bin(name, sym) void F_##name(void){ \
  forth_exp ret, arg1 = pop(&pstack), arg2 = pop(&pstack); \
  set_num(&ret, get_num(arg1) sym get_num(arg2)); \
  push(ret, &pstack); \
  pc++; \
}
def_bin(eq, ==)
def_bin(plus, +)
def_bin(minus, -)
def_bin(div, /)
def_bin(lt, <)
def_bin(gt, >)
def_bin(le, <=)
def_bin(ge, >=)
def_bin(and, &&)
def_bin(or, ||)

void F_max(void)
{
  forth_exp ret, arg1 = pop(&pstack), arg2 = pop(&pstack);
  int a1 = get_num(arg1), a2 = get_num(arg2);
  set_num(&ret, (a1 > a2) ? a1 : a2);
  push(ret, &pstack);
  pc++;
}

void F_min(void)
{
  forth_exp ret, arg1 = pop(&pstack), arg2 = pop(&pstack);
  int a1 = get_num(arg1), a2 = get_num(arg2);
  set_num(&ret, (a1 < a2) ? a1 : a2);
  push(ret, &pstack);
  pc++;
}

/* equal = */

void F_branch_if(void)
{
  forth_exp tmp = pop(&pstack);
    dprintf(("branch:(%p)(%p)\n", pc+1,get_thread(pc[1])));
  if(pc_end_p(pc[1])){
    pc++;
  }else{
    pc = (get_num(tmp) != 0) ? get_thread(pc[1]) : pc + 2;
  }
}

void F_compile(void)
{
  forth_compile_in(pc[1]);
  pc += 2;
}

void F_here(void)
{
  forth_exp tmp;
  set_thread(&tmp, forth_thread_code + thread_index - 1);
  push(tmp, &pstack);
  pc++;
}

void F_print_pstack(void)
{
  print_stack(&pstack);
  pc++;
}

void F_print_rstack(void)
{
  print_stack(&rstack);
  pc++;
}

void F_print_thread(void)
{
  print_thread();
  pc++;
}

void F_print_dict(void)
{
  print_dict();
  pc++;
}

void forth_install_prims(void)
{
  add_forth_prim("nop", FALSE, F_nop);
  add_forth_prim("*", FALSE, F_mul);
  add_forth_prim("drop", FALSE, F_drop);
  add_forth_prim("dup", FALSE, F_dup);
  add_forth_prim("swap", FALSE, F_swap);
  add_forth_prim("print", FALSE, F_print);
  add_forth_prim(">r", FALSE, F_to_r);
  add_forth_prim("r>", FALSE, F_r_to);

  add_forth_prim("[", TRUE, F_l_bracket);
  add_forth_prim("]", FALSE, F_r_bracket);

  add_forth_prim("create", FALSE, F_create);
  add_forth_prim("name", FALSE, F_name);
  add_forth_prim("immediate", FALSE, F_immediate);

  add_forth_prim("@", FALSE, F_fetch);
  add_forth_prim("!", FALSE, F_store);

  add_forth_prim("not", FALSE, F_not);
  add_forth_prim("oddp", FALSE, F_oddp);
  add_forth_prim("evenp", FALSE, F_evenp);
  add_forth_prim("eq", FALSE, F_eq);
  add_forth_prim("+", FALSE, F_plus);
  add_forth_prim("-", FALSE, F_minus);
  add_forth_prim("/", FALSE, F_div);
  add_forth_prim("<", FALSE, F_lt);
  add_forth_prim(">", FALSE, F_gt);
  add_forth_prim("<=", FALSE, F_le);
  add_forth_prim(">=", FALSE, F_ge);
  add_forth_prim("max", FALSE, F_max);
  add_forth_prim("min", FALSE, F_min);
  add_forth_prim("and", FALSE, F_and);
  add_forth_prim("or", FALSE, F_or);
  /* equal = */

  add_forth_prim("branch-if", FALSE, F_branch_if);

  add_forth_prim("compile", FALSE, F_compile);
  add_forth_prim("here", FALSE, F_here); 
  
  add_forth_prim("print-thread", FALSE, F_print_thread);
  add_forth_prim("print-pstack", FALSE, F_print_pstack);
  add_forth_prim("print-rstack", FALSE, F_print_rstack);
  add_forth_prim("print-dict", FALSE, F_print_dict);

}

void forth_compile_in(forth_exp exp)
{
  dprintf(("compile(%4x)type=%d\n",
	   ((int)exp.num & ~TYPE_MASK), get_type(exp)));
  forth_thread_code[thread_index] = exp;
  thread_index++;
  set_end(forth_thread_code + thread_index);

  if(thread_index >= FORTH_THREAD_SIZE){
    fprintf(stderr, "threaded code over flower\n");
    exit(1);
  }
}

void forth_handle_found(forth_word* word)
{
  dprintf(("begin handle_found,compiling=%d\n",compiling));
  if(compiling && !(word->immediate)){
    forth_compile_in(*word->thread);
    dprintf(("after compile_in\n"));
  }else{
    pc = word->thread;
    dprintf(("before inner_interpreter pc=(%p)*pc=(%p)\n", pc, pc->c));
    forth_inner_interpreter();
    dprintf(("after inner_interpreter\n"));
  }
  dprintf(("end handle_found\n"));
}

void forth_handle_not_found(char* v)
{
  if(v[0] == '\''){
    dprintf(("get quote(%s)\n", v + 1));
    if(compiling){
      forth_compile_in(str_to_exp(v + 1));
    }else{
      push(str_to_exp(v + 1), &pstack);
    }
  }else if(v[0] == ','){
    forth_word* word;
    dprintf(("get postpone(%s)\n", v + 1));
    word = forth_lookup(v + 1);
    if(word == NULL){
      fprintf(stderr, "postpone failed %s\n", v + 1);
    }else{
       forth_compile_in(*word->thread);
    }
  }else if(compiling){
    dprintf(("compile word\n"));
    forth_compile_in(str_to_exp(v));
  }else{
    int tmp = atoi(v);
    if((tmp != 0) || v[0] == '0'){
      forth_exp e;
      set_num(&e, tmp);
      push(e, &pstack);
    }else{
      fprintf(stderr, "not found word(%s)\n", v);
      exit(1);
    }
  }
}

int main(void)
{
  char v[STDIN_SIZE], sym[FORTH_WORD_NAME_SIZE];
  int ret, symlen;

  init_stack(&rstack);
  init_stack(&pstack);

  forth_install_prims();
  forth_install_stdlib();

  while(1){
    fgets(v, STDIN_SIZE, stdin);
    //    dprintf(("fgets:(%s)\n",v));
    symlen = 0;
    do{
      ret = sscanf(v + symlen, "%s", sym);
      //      dprintf(("sscanf=(%d),parce:(%s)\n", ret, sym));
      if(ret == 0 || ret == EOF){ break;}
      symlen += strlen(sym) + 1;
      //      dprintf(("strlen(%s)=(%d)\n", sym, symlen)); 
      my_forth(sym);
    }while(1);
  }
  return 0;
}

void forth_install_stdlib(void)
{
  FILE* fp = fopen(FORTH_LIBRARY_NAME, "r");
  int ret;
  char sym[FORTH_WORD_NAME_SIZE], tmp[65535];

  if(fp == NULL){
    fprintf(stderr, "can't find " FORTH_LIBRARY_NAME "\n");
    return;
  }

  do{
    ret = fscanf(fp, "%s", sym);
    if(ret == 0 || ret == EOF){ break;}
    if(sym[0] == '#'){ fgets(tmp,65535,fp); continue;}
    dprintf(("lib load(%s)\n", sym));
    my_forth(sym);
  }while(1);
  fclose(fp);
}

void my_forth(char* v)
{
  forth_word* word;
  if(!strcmp(v, "quit")){ exit(0);}
  //  dprintf(("my_forth:(%s)\n", v));
  word = forth_lookup(v);
  if(word != NULL){
    //    dprintf(("forth_handle_found:(%s)\n", v));
    forth_handle_found(word);
  }else{
    //    dprintf(("forth_handle_not_found:(%s)\n", v));
    forth_handle_not_found(v);
  }
}

void print_stack(forth_stack* st)
{
  int i;
  printf("print_stack:%s\n", (st == &pstack) ? "pstack" : "rstack");
  for(i = 0; i< st->index; i++){
    printf("%2x", i);
    print_forth_exp(st->stack + i);
  }
}

void print_dict(void)
{
  int i;
  printf("print_dict\n");
  for(i = 0; i < dict_index; i++){
    printf("(%16s)imm=%d,thread=%p\n", dict[i].name,
	   dict[i].immediate, dict[i].thread);
  }
}

void print_thread(void)
{
  int i;
  printf("print_thread\n");
  for(i = 0; i <= thread_index; i++){
    print_forth_exp(forth_thread_code + i);
  }
}

void print_forth_exp(forth_exp* e)
{
  printf("%p(%04x):", e, get_type(*e));
  switch(get_type(*e)){
  case TYPE_FIXNUM:
    printf("FIXNUM:%8d\n", get_num(*e));
    break;
  case TYPE_PRIM:
    printf("PRIM  :%p\t%s\n", get_prim(*e),forth_dict_name(e));
    break;
  case TYPE_THREAD:
    printf("THREAD:%p\t%s\n", get_prim(*e),forth_dict_name(e));
    break;
  case TYPE_STRING:
    printf("STRING:%s\n", get_sym(*e));
    break;
  }
}
