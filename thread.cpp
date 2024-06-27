//
// Created by User on 07/04/2023.
//

#include <iostream>
#include "thread.h"

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr) // (copied form the demos)
{
  address_t ret;
  asm volatile("xor    %%fs:0x30,%0\n"
               "rol    $0x11,%0\n"
      : "=g" (ret)
      : "0" (addr));
  return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address (address_t addr)
{
  address_t ret;
  asm volatile("xor    %%gs:0x18,%0\n"
               "rol    $0x9,%0\n"
      : "=g" (ret)
      : "0" (addr));
  return ret;
}

#endif

thread::thread(int tid, char *_stack, thread_entry_point entry_point,
               thread_state state) :
    tid(tid), stack(_stack), state(state), qunatums_running(0), sleeping_countdown(-1)
{
  address_t sp = (address_t) stack + STACK_SIZE - sizeof(address_t);
  address_t pc = (address_t) entry_point;
  sigsetjmp(env, 1);
  (env->__jmpbuf)[JB_SP] = translate_address(sp);
  (env->__jmpbuf)[JB_PC] = translate_address(pc);
  sigemptyset(&(env->__saved_mask));
}

thread::~thread() {
    if (tid != 0 && stack) {
        free(stack);
        stack = nullptr;
    }
}

bool thread::is_sleeping()
{
    return (sleeping_countdown > -1);
}