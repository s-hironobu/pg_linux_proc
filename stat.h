/*-------------------------------------------------------------------------
 *
 * stat.h
 *		Get /proc/stat on Linux
 *
 * Copyright (c) 2008-2025, PostgreSQL Global Development Group
 * Copyright (c) 2024-2025, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#ifndef __PROC_STAT_H__
#define __PROC_STAT_H__

#define FILE_STAT			"/proc/stat"
#define NUM_STAT_FIELDS_MIN	9

/*
 * https://man7.org/linux/man-pages/man5/proc.5.html

user   (1) Time spent in user mode.

nice   (2) Time spent in user mode with low
       priority (nice).

system (3) Time spent in system mode.

idle   (4) Time spent in the idle task.  This value
       should be USER_HZ times the second entry in
       the /proc/uptime pseudo-file.

iowait (since Linux 2.5.41)
       (5) Time waiting for I/O to complete.  This
       value is not reliable, for the following
       reasons:

       •  The CPU will not wait for I/O to
          complete; iowait is the time that a task
          is waiting for I/O to complete.  When a
          CPU goes into idle state for outstanding
          task I/O, another task will be scheduled
          on this CPU.

       •  On a multi-core CPU, the task waiting for
          I/O to complete is not running on any
          CPU, so the iowait of each CPU is
          difficult to calculate.

       •  The value in this field may decrease in
          certain conditions.

irq (since Linux 2.6.0)
       (6) Time servicing interrupts.

softirq (since Linux 2.6.0)
       (7) Time servicing softirqs.

steal (since Linux 2.6.11)
       (8) Stolen time, which is the time spent in
       other operating systems when running in a
       virtualized environment

guest (since Linux 2.6.24)
       (9) Time spent running a virtual CPU for
       guest operating systems under the control of
       the Linux kernel.

guest_nice (since Linux 2.6.33)
       (10) Time spent running a niced guest
       (virtual CPU for guest operating systems
       under the control of the Linux kernel).
 */

typedef struct ProcStat
{
	char		cpu[8];
	int64		user;
	int64		nice;
	int64		system;
	int64		idle;
	int64		iowait;
	int64		irq;
	int64		softirq;
	int64		steal;
}			ProcStat;


extern List *get_proc_stat(struct List *stat);

#endif
