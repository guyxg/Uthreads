#include "manager.h"

void print_lib_err(const std::string &msg)
{
  std::cerr << "thread library error: " << msg << "\n";
}

void print_sys_err(const std::string &msg)
{
  std::cerr << "system error: " << msg << "\n";
}

manager::manager()
{
  thread *main_thread = new thread(0, (char *) 1, (thread_entry_point) 1, RUNNING);
  current_thread = main_thread;
  ready_queue = {};
  sleeping_threads = {};
  threads[0] = main_thread;

  // fill minheap for available thread IDs
  for (int i = 1; i < MAX_THREAD_NUM; i++)
  {
    tid_pq.push(i);
  }
}

manager::~manager()
{
  current_thread = nullptr;

  for (auto it = threads.begin(); it != threads.end(); ++it)
  {
    delete it->second;
  }
}

int manager::get_id() const
{
  return current_thread->tid;
}

int manager::spawn(thread_entry_point entry_point, char *stack)
{
  if (threads.size() == MAX_THREAD_NUM)
  {
    print_lib_err("max number of threads reached.");
    return -1;
  }

  if (entry_point == nullptr)
  {
    print_lib_err("thread's entry point null error.");
    return -1;
  }

  int tid = tid_pq.top();
  tid_pq.pop();

  auto new_thread = new thread(tid, stack, entry_point, READY);
  // add new thread to the threads map and ready queue
  threads[tid] = new_thread;
  ready_queue.push_back(new_thread);

  return tid;
}

thread *manager::pop_next_ready()
{
  thread *next = ready_queue.front();
  ready_queue.pop_front();

  return next;
}

bool manager::is_ready_queue_empty()
{
  return ready_queue.empty();
}

void manager::set_current_to_ready()
{
  current_thread->state = READY;
  ready_queue.push_back(current_thread);
}

void manager::save_and_jump()
{
  int ret_val = sigsetjmp(current_thread->env, 1);
  if (ret_val == 0) // if we saved it just now, else return
  {
    jump_to_next();
  }
}

int manager::terminate(int tid)
{
  // the case tid==0 should be handled by the library
  if (threads.find(tid) == threads.end())
  {
    print_lib_err("attempted to terminate an invalid thread.");
    return -1;
  }

  thread *thread_to_terminate = threads[tid];

  // remove the treads from all containers and delete it
  ready_queue.remove(thread_to_terminate);
  sleeping_threads.erase(thread_to_terminate);
  threads.erase(tid);
  delete thread_to_terminate;

  // return thread id to list of available IDs
  tid_pq.push(tid);

  return 0;
}

int manager::get_quantums(int tid)
{
  auto iter = threads.find(tid);
  if (iter == threads.end())
  {
    print_lib_err("invalid thread id.");
    return -1;
  }

  return iter->second->qunatums_running;
}

int manager::block_thread(int tid)
{
  if (tid == 0)
  {
    print_lib_err("the main thread can't be blocked.");
    return -1;
  }

  auto iter = threads.find(tid);
  if (iter == threads.end())
  {
    print_lib_err("blocked invalid thread.");
    return -1;
  }

  threads[tid]->state = BLOCKED;
  ready_queue.remove(threads[tid]);

  return 0;
}

int manager::resume_thread(int tid)
{
  auto iter = threads.find(tid);
  if (iter == threads.end())
  {
    print_lib_err("resumed invalid thread.");
    return -1;
  }

  thread *const thread_to_resume = threads[tid];

  if (thread_to_resume->state != BLOCKED)
  {
    return 0;
  }

  if (!thread_to_resume->is_sleeping())
  {
    thread_to_resume->state = READY;
    ready_queue.push_back(thread_to_resume);
  }
  else
  {
    thread_to_resume->state = SLEEPING;
  }

  return 0;
}

int manager::sleep(int quantums_to_sleep)
{
  if (get_id() == 0)
  {
    print_lib_err("the main thread cannot be put to sleep.");
    return -1;
  }

  current_thread->state = SLEEPING;
  sleeping_threads.insert(current_thread);
  current_thread->sleeping_countdown = quantums_to_sleep;

  return 0;
}

void manager::jump_to_next()
{
  current_thread = pop_next_ready();
  current_thread->qunatums_running++;
  current_thread->state = RUNNING;
  siglongjmp(current_thread->env, 1);
}

void manager::update_sleeping_threads()
{
  for (auto iter = sleeping_threads.begin(); iter != sleeping_threads.end();)
  {
    thread *ptr = *iter;
    if (!(ptr->is_sleeping()))
    {
      awake_thread(ptr);
      iter = sleeping_threads.erase(iter);
    }
    else
    {
      ptr->sleeping_countdown--;
      iter++;
    }
  }
}

void manager::awake_thread(thread *thread_to_awake)
{
  if (thread_to_awake->state != BLOCKED)
  {
    thread_to_awake->state = READY;
    ready_queue.push_back(thread_to_awake);
  }
}