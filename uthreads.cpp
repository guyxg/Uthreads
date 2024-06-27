#include <iostream>
#include "uthreads.h"
#include "thread.h"
#include "manager.h"

#define USEC_IN_SEC 1000000;

manager *mgr = nullptr;
int total_quantums;

/* structs for timer */
struct itimerval timer;
struct sigaction sa;

void timer_reset();

/*
 * frees dynamically allocated memory before exiting
 */
void safe_exit(int exit_value)
{
  delete mgr;
  mgr = nullptr;
  exit(exit_value);
}

void block_timer()
{
  if (sigprocmask(SIG_BLOCK, &sa.sa_mask, NULL) < 0)
  {
    print_sys_err("sigprocmask error.");
    safe_exit(1);
  }
}

void unblock_timer()
{
  if (sigprocmask(SIG_UNBLOCK, &sa.sa_mask, NULL) < 0)
  {
    print_sys_err("sigprocmask error.");
    safe_exit(1);
  }
}

void timer_handler(int sig)
{
  block_timer();
  if (mgr)
  {
    if (mgr->is_ready_queue_empty())
    {
      mgr->current_thread->qunatums_running++;
      timer_reset();
      return;
    }
    else
    {
      mgr->set_current_to_ready();
      timer_reset();
      mgr->save_and_jump();
      return;
    }
  }
  unblock_timer();

}

/*
 * initializes the library timer to run quantum_time microseconds in virtual time ONCE.
 * once expired, the timer should be reset using timer_reset.
 */
void timer_init(int quantum_time)
{
  // Install timer_handler as the signal handler for SIGVTALRM.
  sa = {0};
  sa.sa_handler = &timer_handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGVTALRM, &sa, NULL) < 0)
  {
    print_sys_err("sigaction error.");
    safe_exit(1);
  }

  timer.it_value.tv_sec = quantum_time / USEC_IN_SEC;        // first time interval, seconds part
  timer.it_value.tv_usec = quantum_time % USEC_IN_SEC;        // first time interval, microseconds part

  // configure the timer to expire every quantum microseconds after that.
  timer.it_interval.tv_sec = 0;    // following time intervals, seconds part
  timer.it_interval.tv_usec = 0;    // following time intervals, microseconds part

  // Start a virtual timer. It counts down whenever this process is executing.
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
  {
    print_sys_err("setitimer error.");
    safe_exit(1);
  }
}

void timer_reset()
{
  block_timer();
  total_quantums++;
  mgr->update_sleeping_threads();
  if (setitimer(ITIMER_VIRTUAL, &timer, NULL))
  {
    print_sys_err("setitimer error.");
    safe_exit(1);
  }
  unblock_timer();
}

/* External interface implementation */

int uthread_init(int quantum_usecs)
{
  if (quantum_usecs <= 0)
  {
    print_lib_err("quantum_time usecs value must be a positive integer.");
    return -1;
  }

  mgr = new manager();
  timer_init(quantum_usecs);

  total_quantums = 1;
  mgr->current_thread->qunatums_running = 1;

  return 0;
}

int uthread_spawn(thread_entry_point entry_point)
{
  block_timer();
  // allocate memory for the thread's stack
  char *stack = (char *) malloc(STACK_SIZE);
  if (!stack)
  {
    print_sys_err("memory allocation failed.");
    safe_exit(1);
  }

  int ret_value = mgr->spawn(entry_point, stack);
  unblock_timer();
  return ret_value;
}

int uthread_terminate(int tid)
{
  block_timer();
  // if the main thread is terminated
  if (tid == 0)
  {
    safe_exit(0);
  }

  bool suicide = (uthread_get_tid() == tid); // if the thread terminates itself

  if (mgr->terminate(tid) < 0)
  {
    return -1;
  }

  // if terminating a running thread, reset the timer and jump to the next ready
  if (suicide)
  {
    timer_reset();
    mgr->jump_to_next();
  }

  unblock_timer();
  return 0;
}

int uthread_block(int tid)
{
  block_timer();
  bool is_switch = (uthread_get_tid() == tid); // if the thread blocks itself

  if (mgr->block_thread(tid) < 0)
  {
    return -1;
  }

  // if the thread blocks itself, reset timer and switch context
  if (is_switch)
  {
    timer_reset();
    mgr->save_and_jump();
  }

  unblock_timer();
  return 0;
}

int uthread_resume(int tid)
{
  block_timer();
  int ret_val = mgr->resume_thread(tid);
  unblock_timer();
  return ret_val;
}

int uthread_sleep(int num_quantums)
{
  block_timer();
  if (mgr->sleep(num_quantums) == -1)
  {
    return -1;
  }

  timer_reset();
  mgr->save_and_jump();
  unblock_timer();
  return 0;
}

int uthread_get_tid()
{
  return mgr->get_id();
}

int uthread_get_total_quantums()
{
  return total_quantums;
}

int uthread_get_quantums(int tid)
{
  return mgr->get_quantums(tid);
}