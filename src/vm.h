#ifndef venom_vm_h
#define venom_vm_h

#define STACK_MAX 255

typedef struct {
    int stack[STACK_MAX];
    int tos;  /* top of stack */
    int *cp;  /* constant pool */
    int cpp;  /* constant pool pointer */
} VM;

VM vm;

void init_vm();
void run();

#endif