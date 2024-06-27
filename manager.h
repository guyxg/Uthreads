//
// Created by User on 09/04/2023.
//

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <iostream>
#include <queue>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <signal.h>
#include <sys/time.h>
#include <memory>
#include "thread.h"
#include "uthreads.h"

typedef std::unordered_map<int, thread *> threads_map;
typedef std::unordered_set<thread *> threads_set;
typedef std::priority_queue<int, std::vector<int>, std::greater<int>> int_minheap;

/* outputs library error message to cerr */
void print_lib_err(const std::string &msg);

/* outputs system error message to cerr */
void print_sys_err(const std::string &msg);

class manager
{
 private:
  threads_map threads; /* hashtable for all threads, keyed by ID */
  int_minheap tid_pq; /* min-heap for available thread IDs */
  std::list<thread *> ready_queue; /* dequeue for ready threads */
  threads_set sleeping_threads; /* set of all threads currently seeping */

 public:
  thread *current_thread;

  /**
   * constructor
   * @param quantum_usecs length of a single quantum_time in micro-seconds
   */
  manager();

  ~manager();

  int spawn(thread_entry_point entry_point, char *stack);

  thread *pop_next_ready();

  int terminate(int tid);

  int block_thread(int tid);

  int sleep(int quantums_to_sleep);

  int resume_thread(int tid);

  bool is_ready_queue_empty();

  void set_current_to_ready();

  void save_and_jump();

  /**
   * @return current thread's id.
   */
  int get_id() const;

  /**
   * @param tid thread's ID
   * @return number of quantums the thread has been in a RUNNING state, -1 in case of invalid tid
   */
  int get_quantums(int tid);

  /**
   * jumps to the next ready thread.
   */
  void jump_to_next();

  /**
   * iterates over the sleeping threads container, decrements their sleeping countdown
   * or wakes them up if their countdown is up.
   */
  void update_sleeping_threads();

  /**
   * removes thread from sleeping threads set. If not in BLOCKED state, adds it to the ready queue.
   */
  void awake_thread(thread *thread_to_awake);
};

#endif //_MANAGER_H_
