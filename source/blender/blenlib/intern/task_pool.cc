/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup bli
 *
 * A generic task system which can be used for any task based subsystem.
 */

#include <stdlib.h>

#include "MEM_guardedalloc.h"

#include "DNA_listBase.h"

#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_mempool.h"
#include "BLI_task.h"
#include "BLI_threads.h"

#include "atomic_ops.h"

/* Define this to enable some detailed statistic print. */
#undef DEBUG_STATS

/* Types */

/* Number of per-thread pre-allocated tasks.
 *
 * For more details see description of TaskMemPool.
 */
#define MEMPOOL_SIZE 256

#ifndef NDEBUG
#  define ASSERT_THREAD_ID(scheduler, thread_id) \
    do { \
      if (!BLI_thread_is_main()) { \
        TaskThread *thread = (TaskThread *)pthread_getspecific(scheduler->tls_id_key); \
        if (thread == NULL) { \
          BLI_assert(thread_id == 0); \
        } \
        else { \
          BLI_assert(thread_id == thread->id); \
        } \
      } \
      else { \
        BLI_assert(thread_id == 0); \
      } \
    } while (false)
#else
#  define ASSERT_THREAD_ID(scheduler, thread_id)
#endif

typedef struct Task {
  struct Task *next, *prev;

  TaskRunFunction run;
  void *taskdata;
  bool free_taskdata;
  TaskFreeFunction freedata;
  TaskPool *pool;
} Task;

/* This is a per-thread storage of pre-allocated tasks.
 *
 * The idea behind this is simple: reduce amount of malloc() calls when pushing
 * new task to the pool. This is done by keeping memory from the tasks which
 * were finished already, so instead of freeing that memory we put it to the
 * pool for the later re-use.
 *
 * The tricky part here is to avoid any inter-thread synchronization, hence no
 * lock must exist around this pool. The pool will become an owner of the pointer
 * from freed task, and only corresponding thread will be able to use this pool
 * (no memory stealing and such).
 *
 * This leads to the following use of the pool:
 *
 * - task_push() should provide proper thread ID from which the task is being
 *   pushed from.
 *
 * - Task allocation function which check corresponding memory pool and if there
 *   is any memory in there it'll mark memory as re-used, remove it from the pool
 *   and use that memory for the new task.
 *
 *   At this moment task queue owns the memory.
 *
 * - When task is done and task_free() is called the memory will be put to the
 *   pool which corresponds to a thread which handled the task.
 */
typedef struct TaskMemPool {
  /* Number of pre-allocated tasks in the pool. */
  int num_tasks;
  /* Pre-allocated task memory pointers. */
  Task *tasks[MEMPOOL_SIZE];
} TaskMemPool;

#ifdef DEBUG_STATS
typedef struct TaskMemPoolStats {
  /* Number of allocations. */
  int num_alloc;
  /* Number of avoided allocations (pointer was re-used from the pool). */
  int num_reuse;
  /* Number of discarded memory due to pool saturation, */
  int num_discard;
} TaskMemPoolStats;
#endif

struct TaskPool {
  TaskScheduler *scheduler;

  volatile size_t num;
  ThreadMutex num_mutex;
  ThreadCondition num_cond;

  void *userdata;
  ThreadMutex user_mutex;

  volatile bool do_cancel;
  volatile bool do_work;

  volatile bool is_suspended;
  bool start_suspended;
  ListBase suspended_queue;
  size_t num_suspended;

  TaskPriority priority;

  /* If set, this pool may never be work_and_wait'ed, which means TaskScheduler
   * has to use its special background fallback thread in case we are in
   * single-threaded situation.
   */
  bool run_in_background;

  /* This is a task scheduler's ID of a thread at which pool was constructed.
   * It will be used to access task TLS.
   */
  int thread_id;

#ifndef NDEBUG
  pthread_t creator_thread_id;
#endif
};

struct TaskScheduler {
  pthread_t *threads;
  struct TaskThread *task_threads;
  int num_threads;
  bool background_thread_only;

  ListBase queue;
  ThreadMutex queue_mutex;
  ThreadCondition queue_cond;

  ThreadMutex startup_mutex;
  ThreadCondition startup_cond;
  volatile int num_thread_started;

  volatile bool do_exit;

  /* NOTE: In pthread's TLS we store the whole TaskThread structure. */
  pthread_key_t tls_id_key;
};

typedef struct TaskThread {
  TaskScheduler *scheduler;
  int id;
} TaskThread;

/* Helper */
BLI_INLINE void task_data_free(Task *task, const int thread_id)
{
  if (task->free_taskdata) {
    if (task->freedata) {
      task->freedata(task->pool, task->taskdata, thread_id);
    }
    else {
      MEM_freeN(task->taskdata);
    }
  }
}

static Task *task_alloc(TaskPool *pool)
{
  return (Task *)MEM_mallocN(sizeof(Task), "New task");
}

static void task_free(TaskPool *pool, Task *task, const int thread_id)
{
  task_data_free(task, thread_id);
  MEM_freeN(task);
}

/* Task Scheduler */

static void task_pool_num_decrease(TaskPool *pool, size_t done)
{
  BLI_mutex_lock(&pool->num_mutex);

  BLI_assert(pool->num >= done);

  pool->num -= done;

  if (pool->num == 0) {
    BLI_condition_notify_all(&pool->num_cond);
  }

  BLI_mutex_unlock(&pool->num_mutex);
}

static void task_pool_num_increase(TaskPool *pool, size_t new_num)
{
  BLI_mutex_lock(&pool->num_mutex);

  pool->num += new_num;
  BLI_condition_notify_all(&pool->num_cond);

  BLI_mutex_unlock(&pool->num_mutex);
}

static bool task_scheduler_thread_wait_pop(TaskScheduler *scheduler, Task **task)
{
  bool found_task = false;
  BLI_mutex_lock(&scheduler->queue_mutex);

  while (!scheduler->queue.first && !scheduler->do_exit) {
    BLI_condition_wait(&scheduler->queue_cond, &scheduler->queue_mutex);
  }

  do {
    Task *current_task;

    /* Assuming we can only have a void queue in 'exit' case here seems logical
     * (we should only be here after our worker thread has been woken up from a
     * condition_wait(), which only happens after a new task was added to the queue),
     * but it is wrong.
     * Waiting on condition may wake up the thread even if condition is not signaled
     * (spurious wake-ups), and some race condition may also empty the queue **after**
     * condition has been signaled, but **before** awoken thread reaches this point...
     * See http://stackoverflow.com/questions/8594591
     *
     * So we only abort here if do_exit is set.
     */
    if (scheduler->do_exit) {
      BLI_mutex_unlock(&scheduler->queue_mutex);
      return false;
    }

    for (current_task = (Task *)scheduler->queue.first; current_task != NULL;
         current_task = current_task->next) {
      TaskPool *pool = current_task->pool;

      if (scheduler->background_thread_only && !pool->run_in_background) {
        continue;
      }

      *task = current_task;
      found_task = true;
      BLI_remlink(&scheduler->queue, *task);
      break;
    }
    if (!found_task) {
      BLI_condition_wait(&scheduler->queue_cond, &scheduler->queue_mutex);
    }
  } while (!found_task);

  BLI_mutex_unlock(&scheduler->queue_mutex);

  return true;
}

static void *task_scheduler_thread_run(void *thread_p)
{
  TaskThread *thread = (TaskThread *)thread_p;
  TaskScheduler *scheduler = thread->scheduler;
  int thread_id = thread->id;
  Task *task;

  pthread_setspecific(scheduler->tls_id_key, thread);

  /* signal the main thread when all threads have started */
  BLI_mutex_lock(&scheduler->startup_mutex);
  scheduler->num_thread_started++;
  if (scheduler->num_thread_started == scheduler->num_threads) {
    BLI_condition_notify_one(&scheduler->startup_cond);
  }
  BLI_mutex_unlock(&scheduler->startup_mutex);

  /* keep popping off tasks */
  while (task_scheduler_thread_wait_pop(scheduler, &task)) {
    TaskPool *pool = task->pool;

    /* run task */
    task->run(pool, task->taskdata, thread_id);

    /* delete task */
    task_free(pool, task, thread_id);

    /* notify pool task was done */
    task_pool_num_decrease(pool, 1);
  }

  return NULL;
}

TaskScheduler *BLI_task_scheduler_create(int num_threads)
{
  TaskScheduler *scheduler = (TaskScheduler *)MEM_callocN(sizeof(TaskScheduler), "TaskScheduler");

  /* multiple places can use this task scheduler, sharing the same
   * threads, so we keep track of the number of users. */
  scheduler->do_exit = false;

  BLI_listbase_clear(&scheduler->queue);
  BLI_mutex_init(&scheduler->queue_mutex);
  BLI_condition_init(&scheduler->queue_cond);

  BLI_mutex_init(&scheduler->startup_mutex);
  BLI_condition_init(&scheduler->startup_cond);
  scheduler->num_thread_started = 0;

  if (num_threads == 0) {
    /* automatic number of threads will be main thread + num cores */
    num_threads = BLI_system_thread_count();
  }

  /* main thread will also work, so we count it too */
  num_threads -= 1;

  /* Add background-only thread if needed. */
  if (num_threads == 0) {
    scheduler->background_thread_only = true;
    num_threads = 1;
  }

  scheduler->task_threads = (TaskThread *)MEM_mallocN(sizeof(TaskThread) * (num_threads + 1),
                                                      "TaskScheduler task threads");

  /* Initialize TLS for main thread. */
  pthread_key_create(&scheduler->tls_id_key, NULL);

  /* launch threads that will be waiting for work */
  if (num_threads > 0) {
    int i;

    scheduler->num_threads = num_threads;
    scheduler->threads = (pthread_t *)MEM_callocN(sizeof(pthread_t) * num_threads,
                                                  "TaskScheduler threads");

    for (i = 0; i < num_threads; i++) {
      TaskThread *thread = &scheduler->task_threads[i + 1];
      thread->scheduler = scheduler;
      thread->id = i + 1;

      if (pthread_create(&scheduler->threads[i], NULL, task_scheduler_thread_run, thread) != 0) {
        fprintf(stderr, "TaskScheduler failed to launch thread %d/%d\n", i, num_threads);
      }
    }
  }

  /* Wait for all worker threads to start before returning to caller to prevent the case where
   * threads are still starting and pthread_join is called, which causes a deadlock on pthreads4w.
   */
  BLI_mutex_lock(&scheduler->startup_mutex);
  /* NOTE: Use loop here to avoid false-positive everything-is-ready caused by spontaneous thread
   * wake up. */
  while (scheduler->num_thread_started != num_threads) {
    BLI_condition_wait(&scheduler->startup_cond, &scheduler->startup_mutex);
  }
  BLI_mutex_unlock(&scheduler->startup_mutex);

  return scheduler;
}

void BLI_task_scheduler_free(TaskScheduler *scheduler)
{
  Task *task;

  /* stop all waiting threads */
  BLI_mutex_lock(&scheduler->queue_mutex);
  scheduler->do_exit = true;
  BLI_condition_notify_all(&scheduler->queue_cond);
  BLI_mutex_unlock(&scheduler->queue_mutex);

  pthread_key_delete(scheduler->tls_id_key);

  /* delete threads */
  if (scheduler->threads) {
    int i;

    for (i = 0; i < scheduler->num_threads; i++) {
      if (pthread_join(scheduler->threads[i], NULL) != 0) {
        fprintf(stderr, "TaskScheduler failed to join thread %d/%d\n", i, scheduler->num_threads);
      }
    }

    MEM_freeN(scheduler->threads);
  }

  /* Delete task thread data */
  if (scheduler->task_threads) {
    MEM_freeN(scheduler->task_threads);
  }

  /* delete leftover tasks */
  for (task = (Task *)scheduler->queue.first; task; task = task->next) {
    task_data_free(task, 0);
  }
  BLI_freelistN(&scheduler->queue);

  /* delete mutex/condition */
  BLI_mutex_end(&scheduler->queue_mutex);
  BLI_condition_end(&scheduler->queue_cond);
  BLI_mutex_end(&scheduler->startup_mutex);
  BLI_condition_end(&scheduler->startup_cond);

  MEM_freeN(scheduler);
}

int BLI_task_scheduler_num_threads(TaskScheduler *scheduler)
{
  return scheduler->num_threads + 1;
}

static void task_scheduler_push(TaskScheduler *scheduler, Task *task, TaskPriority priority)
{
  task_pool_num_increase(task->pool, 1);

  /* add task to queue */
  BLI_mutex_lock(&scheduler->queue_mutex);

  if (priority == TASK_PRIORITY_HIGH) {
    BLI_addhead(&scheduler->queue, task);
  }
  else {
    BLI_addtail(&scheduler->queue, task);
  }

  BLI_condition_notify_one(&scheduler->queue_cond);
  BLI_mutex_unlock(&scheduler->queue_mutex);
}

static void task_scheduler_push_all(TaskScheduler *scheduler,
                                    TaskPool *pool,
                                    Task **tasks,
                                    int num_tasks)
{
  if (num_tasks == 0) {
    return;
  }

  task_pool_num_increase(pool, num_tasks);

  BLI_mutex_lock(&scheduler->queue_mutex);

  for (int i = 0; i < num_tasks; i++) {
    BLI_addhead(&scheduler->queue, tasks[i]);
  }

  BLI_condition_notify_all(&scheduler->queue_cond);
  BLI_mutex_unlock(&scheduler->queue_mutex);
}

static void task_scheduler_clear(TaskScheduler *scheduler, TaskPool *pool)
{
  Task *task, *nexttask;
  size_t done = 0;

  BLI_mutex_lock(&scheduler->queue_mutex);

  /* free all tasks from this pool from the queue */
  for (task = (Task *)scheduler->queue.first; task; task = nexttask) {
    nexttask = task->next;

    if (task->pool == pool) {
      task_data_free(task, pool->thread_id);
      BLI_freelinkN(&scheduler->queue, task);

      done++;
    }
  }

  BLI_mutex_unlock(&scheduler->queue_mutex);

  /* notify done */
  task_pool_num_decrease(pool, done);
}

/* Task Pool */

static TaskPool *task_pool_create_ex(TaskScheduler *scheduler,
                                     void *userdata,
                                     const bool is_background,
                                     const bool is_suspended,
                                     const TaskPriority priority)
{
  TaskPool *pool = (TaskPool *)MEM_mallocN(sizeof(TaskPool), "TaskPool");

#ifndef NDEBUG
  /* Assert we do not try to create a background pool from some parent task -
   * those only work OK from main thread. */
  if (is_background) {
    const pthread_t thread_id = pthread_self();
    int i = scheduler->num_threads;

    while (i--) {
      BLI_assert(!pthread_equal(scheduler->threads[i], thread_id));
    }
  }
#endif

  pool->scheduler = scheduler;
  pool->num = 0;
  pool->do_cancel = false;
  pool->do_work = false;
  pool->is_suspended = is_suspended;
  pool->start_suspended = is_suspended;
  pool->num_suspended = 0;
  pool->suspended_queue.first = pool->suspended_queue.last = NULL;
  pool->priority = priority;
  pool->run_in_background = is_background;

  BLI_mutex_init(&pool->num_mutex);
  BLI_condition_init(&pool->num_cond);

  pool->userdata = userdata;
  BLI_mutex_init(&pool->user_mutex);

  if (BLI_thread_is_main()) {
    pool->thread_id = 0;
  }
  else {
    TaskThread *thread = (TaskThread *)pthread_getspecific(scheduler->tls_id_key);
    if (thread == NULL) {
      /* NOTE: Task pool is created from non-main thread which is not
       * managed by the task scheduler. We identify ourselves as thread ID
       * 0 but we do not use scheduler's TLS storage and use our own
       * instead to avoid any possible threading conflicts.
       */
      pool->thread_id = 0;
#ifndef NDEBUG
      pool->creator_thread_id = pthread_self();
#endif
    }
    else {
      pool->thread_id = thread->id;
    }
  }

  /* Ensure malloc will go fine from threads,
   *
   * This is needed because we could be in main thread here
   * and malloc could be non-thread safe at this point because
   * no other jobs are running.
   */
  BLI_threaded_malloc_begin();

  return pool;
}

/**
 * Create a normal task pool. Tasks will be executed as soon as they are added.
 */
TaskPool *BLI_task_pool_create(TaskScheduler *scheduler, void *userdata, TaskPriority priority)
{
  return task_pool_create_ex(scheduler, userdata, false, false, priority);
}

/**
 * Create a background task pool.
 * In multi-threaded context, there is no differences with #BLI_task_pool_create(),
 * but in single-threaded case it is ensured to have at least one worker thread to run on
 * (i.e. you don't have to call #BLI_task_pool_work_and_wait
 * on it to be sure it will be processed).
 *
 * \note Background pools are non-recursive
 * (that is, you should not create other background pools in tasks assigned to a background pool,
 * they could end never being executed, since the 'fallback' background thread is already
 * busy with parent task in single-threaded context).
 */
TaskPool *BLI_task_pool_create_background(TaskScheduler *scheduler,
                                          void *userdata,
                                          TaskPriority priority)
{
  return task_pool_create_ex(scheduler, userdata, true, false, priority);
}

/**
 * Similar to BLI_task_pool_create() but does not schedule any tasks for execution
 * for until BLI_task_pool_work_and_wait() is called. This helps reducing threading
 * overhead when pushing huge amount of small initial tasks from the main thread.
 */
TaskPool *BLI_task_pool_create_suspended(TaskScheduler *scheduler,
                                         void *userdata,
                                         TaskPriority priority)
{
  return task_pool_create_ex(scheduler, userdata, false, true, priority);
}

void BLI_task_pool_free(TaskPool *pool)
{
  BLI_task_pool_cancel(pool);

  BLI_mutex_end(&pool->num_mutex);
  BLI_condition_end(&pool->num_cond);

  BLI_mutex_end(&pool->user_mutex);

  MEM_freeN(pool);

  BLI_threaded_malloc_end();
}

void BLI_task_pool_push(TaskPool *pool,
                        TaskRunFunction run,
                        void *taskdata,
                        bool free_taskdata,
                        TaskFreeFunction freedata)
{
  /* Allocate task and fill it's properties. */
  Task *task = task_alloc(pool);
  task->run = run;
  task->taskdata = taskdata;
  task->free_taskdata = free_taskdata;
  task->freedata = freedata;
  task->pool = pool;
  /* For suspended pools we put everything yo a global queue first
   * and exit as soon as possible.
   *
   * This tasks will be moved to actual execution when pool is
   * activated by work_and_wait().
   */
  if (pool->is_suspended) {
    BLI_addhead(&pool->suspended_queue, task);
    atomic_fetch_and_add_z(&pool->num_suspended, 1);
    return;
  }
  /* Do push to a global execution pool, slowest possible method,
   * causes quite reasonable amount of threading overhead.
   */
  task_scheduler_push(pool->scheduler, task, pool->priority);
}

void BLI_task_pool_work_and_wait(TaskPool *pool)
{
  TaskScheduler *scheduler = pool->scheduler;

  if (atomic_fetch_and_and_uint8((uint8_t *)&pool->is_suspended, 0)) {
    if (pool->num_suspended) {
      task_pool_num_increase(pool, pool->num_suspended);
      BLI_mutex_lock(&scheduler->queue_mutex);

      BLI_movelisttolist(&scheduler->queue, &pool->suspended_queue);

      BLI_condition_notify_all(&scheduler->queue_cond);
      BLI_mutex_unlock(&scheduler->queue_mutex);

      pool->num_suspended = 0;
    }
  }

  pool->do_work = true;

  ASSERT_THREAD_ID(pool->scheduler, pool->thread_id);

  BLI_mutex_lock(&pool->num_mutex);

  while (pool->num != 0) {
    Task *task, *work_task = NULL;
    bool found_task = false;

    BLI_mutex_unlock(&pool->num_mutex);

    BLI_mutex_lock(&scheduler->queue_mutex);

    /* find task from this pool. if we get a task from another pool,
     * we can get into deadlock */

    for (task = (Task *)scheduler->queue.first; task; task = task->next) {
      if (task->pool == pool) {
        work_task = task;
        found_task = true;
        BLI_remlink(&scheduler->queue, task);
        break;
      }
    }

    BLI_mutex_unlock(&scheduler->queue_mutex);

    /* if found task, do it, otherwise wait until other tasks are done */
    if (found_task) {
      /* run task */
      work_task->run(pool, work_task->taskdata, pool->thread_id);

      /* delete task */
      task_free(pool, task, pool->thread_id);

      /* notify pool task was done */
      task_pool_num_decrease(pool, 1);
    }

    BLI_mutex_lock(&pool->num_mutex);
    if (pool->num == 0) {
      break;
    }

    if (!found_task) {
      BLI_condition_wait(&pool->num_cond, &pool->num_mutex);
    }
  }

  BLI_mutex_unlock(&pool->num_mutex);
}

void BLI_task_pool_work_wait_and_reset(TaskPool *pool)
{
  BLI_task_pool_work_and_wait(pool);

  pool->do_work = false;
  pool->is_suspended = pool->start_suspended;
}

void BLI_task_pool_cancel(TaskPool *pool)
{
  pool->do_cancel = true;

  task_scheduler_clear(pool->scheduler, pool);

  /* wait until all entries are cleared */
  BLI_mutex_lock(&pool->num_mutex);
  while (pool->num) {
    BLI_condition_wait(&pool->num_cond, &pool->num_mutex);
  }
  BLI_mutex_unlock(&pool->num_mutex);

  pool->do_cancel = false;
}

bool BLI_task_pool_canceled(TaskPool *pool)
{
  return pool->do_cancel;
}

void *BLI_task_pool_userdata(TaskPool *pool)
{
  return pool->userdata;
}

ThreadMutex *BLI_task_pool_user_mutex(TaskPool *pool)
{
  return &pool->user_mutex;
}
