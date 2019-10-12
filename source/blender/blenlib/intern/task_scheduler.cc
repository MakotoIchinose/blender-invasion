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
 * Task scheduler initialization.
 */

#include "BLI_task.h"
#include "BLI_threads.h"

#ifdef WITH_TBB
/* Quiet top level deprecation message, unrelated to API usage here. */
#  define TBB_SUPPRESS_DEPRECATED_MESSAGES 1
#  include <tbb/tbb.h>
#endif

/* Task Scheduler */

static int task_scheduler_num_threads = 1;

void BLI_task_scheduler_init()
{
#ifdef WITH_TBB
  task_scheduler_num_threads = BLI_system_thread_count();
  tbb::task_scheduler_init init(task_scheduler_num_threads);
#endif
}

int BLI_task_scheduler_num_threads(TaskScheduler *UNUSED(scheduler))
{
  return task_scheduler_num_threads;
}
