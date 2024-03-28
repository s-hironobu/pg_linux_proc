/*-------------------------------------------------------------------------
 *
 * loadavg.h
 *		Show /proc/loadavg info on Linux
 *
 * Copyright (c) 2008-2024, PostgreSQL Global Development Group
 * Copyright (c) 2024, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#ifndef __LOADAVG_H__
#define __LOADAVG_H__

#define FILE_LOADAVG			"/proc/loadavg"
#define NUM_LOADAVG_FIELDS_MIN	6

typedef struct LoadAvg
{
	float4		loadavg1;
	float4		loadavg5;
	float4		loadavg15;
	int32		current_processes;
	int32		total_processes;
	int32		last_pid;
}			LoadAvg;


extern bool get_proc_loadavg(struct LoadAvg *loadavg);


#endif
