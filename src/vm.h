#ifndef venom_vm_h
#define venom_vm_h

int stack[255];
int tos;

int pop();
void push(int value);
void run();

#endif