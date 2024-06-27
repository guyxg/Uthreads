//
// Created by User on 07/04/2023.
//

#ifndef _THREAD_H_
#define _THREAD_H_

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include "uthreads.h"

/*
 * possible thread's states. A thread could be considered BLOCKED and SLEEPING simultaneously - in that
 * case the thread's state should be set to BLOCKED until resumed.
 */
enum thread_state
{
  RUNNING, READY, BLOCKED, SLEEPING
};

struct thread
{
  const int tid;
  char* stack;
  thread_state state;
  sigjmp_buf env;
  int qunatums_running; /* number of quantums in RUNNING state */
  int sleeping_countdown; /* number of quantums left to sleep. should be -1 if the thread is awake */

  thread(int tid, char *_stack, thread_entry_point entry_point,
         thread_state state);
  ~thread();

  // returns true if the thread is sleeping (sleeping_countdown >= 0), false otherwise
  bool is_sleeping();
};

#endif //_THREAD_H_
