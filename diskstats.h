/*-------------------------------------------------------------------------
 *
 * diskstats.h
 *		Get /proc/diskstats on Linux
 *
 * Copyright (c) 2008-2024, PostgreSQL Global Development Group
 * Copyright (c) 2024, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/pg_list.h"

#ifndef __DISKSTATS_H__
#define __DISKSTATS_H__

#define FILE_DISKSTATS		"/proc/diskstats"

typedef struct DiskStat
{
	int			major;			/* 1  major number */
	int			minor;			/* 2  minor number */
	char		name[32];		/* 3  device name */
	int64		rd;				/* 4  reads completed successfully */
	int64		rd_merged;		/* 5  reads merged */
	int64		rd_sec;			/* 6  sectors read */
	int64		rd_tm;			/* 7  time spent reading (ms) */
	int64		wr;				/* 8  writes completed */
	int64		wr_merged;		/* 9  writes merged */
	int64		wr_sec;			/* 10  sectors written */
	int64		wr_tm;			/* 11  time spent writing (ms) */
	int64		io;				/* 12  I/Os currently in progress */
	int64		tm;				/* 13  time spent doing I/Os (ms) */
	int64		wtm;			/* 14  weighted time spent doing I/Os (ms) */

	/*
	 * Kernel 4.18+ appends four more fields for discard
	 */
	int64		dis;			/* 15  discards completed successfully */
	int64		dis_merged;		/* 16  discards merged */
	int64		dis_sec;		/* 17  sectors discarded */
	int64		dis_tm;			/* 18  time spent discarding */

	/*
	 * Kernel 5.5+ appends two more fields for flush requests:
	 */
	int64		fl;				/* 19  flush requests completed successfully */
	int64		tm_fl;			/* 20  time spent flushing */

}			DiskStat;

extern List *get_proc_diskstats(List *diskstats);

#endif
