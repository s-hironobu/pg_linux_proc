/*-------------------------------------------------------------------------
 *
 * pid.h
 *		Get pid list from /proc on Linux
 *
 * Copyright (c) 2008-2025, PostgreSQL Global Development Group
 * Copyright (c) 2024-2025, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#ifndef __PROC_PID_H__
#define __PROC_PID_H__

#define DIR_PID			"/proc"
#define MAX_CMDLINE 512

typedef struct ProcPid
{
	int64		pid;
	char		cmdline[MAX_CMDLINE];
}			ProcPid;


extern List *get_proc_pid(struct List *pid);

#endif
