/*-------------------------------------------------------------------------
 *
 * meminfo.h
 *		Show /proc/meminfo info on Linux
 *
 * Copyright (c) 2008-2025, PostgreSQL Global Development Group
 * Copyright (c) 2024-2025, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#ifndef __MEMINFO_H__
#define __MEMINFO_H__

#define FILE_MEMINFO		"/proc/meminfo"

/*
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/deployment_guide/s2-proc-meminfo

*/

typedef struct MemInfo
{
	int64		MemTotal;
	int64		MemFree;
	int64		MemAvailable;
	int64		Buffers;
	int64		Cached;
	int64		SwapCached;
	int64		Active;
	int64		Inactive;
	int64		Active_anon;
	int64		Inactive_anon;

	int64		Active_file;
	int64		Inactive_file;
	int64		Unevictable;
	int64		Mlocked;
	int64		SwapTotal;
	int64		SwapFree;
	int64		Dirty;
	int64		Writeback;
	int64		AnonPages;
	int64		Mapped;

	int64		Shmem;
	int64		KReclaimable;
	int64		Slab;
	int64		SReclaimable;
	int64		SUnreclaim;
	int64		KernelStack;
	int64		PageTables;
	int64		NFS_Unstable;
	int64		Bounce;
	int64		WritebackTmp;

	int64		CommitLimit;
	int64		Committed_AS;
	int64		VmallocTotal;
	int64		VmallocUsed;
	int64		VmallocChunk;
	int64		Percpu;
	int64		HardwareCorrupted;
	int64		AnonHugePages;
	int64		ShmemHugePages;
	int64		ShmemPmdMapped;

	int64		FileHugePages;
	int64		FilePmdMapped;
	int64		CmaTotal;
	int64		CmaFree;
	int64		HugePages_Total;
	int64		HugePages_Free;
	int64		HugePages_Rsvd;
	int64		HugePages_Surp;
	int64		Hugepagesize;
	int64		Hugetlb;
}			MemInfo;


typedef struct meminfo_store
{
	const char *key;
	int64	   *value;
}			meminfo_store;


extern bool get_proc_meminfo(MemInfo * meminfo);

#endif
