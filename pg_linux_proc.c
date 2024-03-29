/*-------------------------------------------------------------------------
 *
 * pg_linux_proc.c
 *		Show /proc info on Linux
 *
 * Copyright (c) 2008-2024, PostgreSQL Global Development Group
 * Copyright (c) 2024, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "nodes/pg_list.h"
#include "utils/builtins.h"
#include "funcapi.h"
#include "tcop/utility.h"
#include "pgstat.h"

#include "loadavg.h"
#include "diskstats.h"
#include "meminfo.h"
#include "stat.h"
#include "pid.h"



PG_MODULE_MAGIC;

/* Function declarations */
void		_PG_init(void);
void		_PG_fini(void);

Datum		pg_proc(PG_FUNCTION_ARGS);
Datum		pg_proc_pid(PG_FUNCTION_ARGS);
Datum		pg_os_version(PG_FUNCTION_ARGS);
Datum		pg_proc_loadavg(PG_FUNCTION_ARGS);
Datum		pg_proc_diskstats(PG_FUNCTION_ARGS);
Datum		pg_proc_meminfo(PG_FUNCTION_ARGS);
Datum		pg_proc_stat(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pg_proc);
PG_FUNCTION_INFO_V1(pg_proc_pid);
PG_FUNCTION_INFO_V1(pg_os_version);
PG_FUNCTION_INFO_V1(pg_proc_loadavg);
PG_FUNCTION_INFO_V1(pg_proc_diskstats);
PG_FUNCTION_INFO_V1(pg_proc_meminfo);
PG_FUNCTION_INFO_V1(pg_proc_stat);


/* Module callback */
void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
		return;

	EmitWarningsOnPlaceholders("pg_linux_proc");
}

void
_PG_fini(void)
{
	;
}

/*
 * Display information for specified file under /proc.
 */

Datum
pg_proc(PG_FUNCTION_ARGS)
{
	text	   *t = PG_GETARG_TEXT_PP(0);

	int			i;
	char		file[64 + 7];
	FILE	   *fp = NULL;
	char		buffer[1024];
	List	   *procinfo = NIL;
	ListCell   *lc;
	int			length = 0;
	char	   *ret;
	char	   *input = VARDATA_ANY(t);
	int			input_len = VARSIZE_ANY_EXHDR(t);

	/* Check for "." to prevent setting relative paths. */
	for (i = 0; i < input_len; i++)
		if (input[i] == '.')
			elog(ERROR, "Input has \'.\', but relative path cannot be set.");

	if (input_len > 64)
		elog(ERROR, "Input sentence \'%s\' is too long. Less than 64.", input);

	sprintf(file, "/proc/%s", input);
	file[input_len + 6] = '\0';

	/* Check file open */
	if ((fp = fopen(file, "r")) == NULL)
		elog(ERROR, "Can't open %s", file);

	/* Copy all outputs to the List procinfo. */
	while (fgets(buffer, sizeof(buffer) - 1, fp) != NULL)
	{
		char	   *l = palloc0(sizeof(buffer));

		memcpy(l, buffer, sizeof(buffer));
		procinfo = lappend(procinfo, l);
	}

	fclose(fp);

	/* Count output length. */
	foreach(lc, procinfo)
		length += strlen((char *) lfirst(lc));

	/* Copy from each element of the list to the ret data area sequentially. */
	ret = (char *) palloc0(length);
	length = 0;
	foreach(lc, procinfo)
	{
		char	   *l = (char *) lfirst(lc);
		int			len = strlen(l);

		memcpy(ret + length, l, len);
		length += len;

	}
	ret[length] = '\0';

	PG_RETURN_TEXT_P(cstring_to_text(ret));

}

/*
 * Show running process IDs (PIDs) and their names.
 */

#define NUM_PID_COLS 2

Datum
pg_proc_pid(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	Datum		values[NUM_PID_COLS];
	bool		nulls[NUM_PID_COLS];
	List	   *pids = NIL;
	ListCell   *lc;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");
	Assert(tupdesc->natts == NUM_PID_COLS);

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	pids = get_proc_pid(pids);

	foreach(lc, pids)
	{
		ProcPid    *ps = (ProcPid *) lfirst(lc);
		int			i;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		i = 0;
		values[i++] = Int64GetDatum(ps->pid);
		values[i++] = CStringGetTextDatum(ps->cmdline);
		Assert(i == NUM_PID_COLS);
		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		pfree(ps);
	}

	return (Datum) 0;
}


/*
 * Display OS type and verion.
 */
#define FILE_OS_TYPE		"/proc/sys/kernel/ostype"
#define FILE_OS_VERSION		"/proc/sys/kernel/osrelease"

Datum
pg_os_version(PG_FUNCTION_ARGS)
{
	TupleDesc	tupdesc;
	HeapTuple	tuple;
	int			fd;
	char		buffer[64];
	int			nbytes;
	Datum		values[2];
	bool		nulls[2];

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	memset(nulls, 0, sizeof(nulls));
	memset(values, 0, sizeof(values));

	/*
	 * Get os type
	 */
	if ((fd = open(FILE_OS_TYPE, O_RDONLY)) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", FILE_OS_TYPE)));

	if ((nbytes = read(fd, buffer, sizeof(buffer) - 1)) < 0)
	{
		close(fd);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": ", FILE_OS_TYPE)));
	}

	close(fd);
	buffer[nbytes] = '\0';

	if (sscanf(buffer, "%s", buffer) < 1)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("unexpected file format: \"%s\"", FILE_OS_TYPE),
				 errdetail("number of fields is not corresponding")));

	values[0] = CStringGetTextDatum(buffer);

	/*
	 * Get os version
	 */
	if ((fd = open(FILE_OS_VERSION, O_RDONLY)) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", FILE_OS_VERSION)));

	if ((nbytes = read(fd, buffer, sizeof(buffer) - 1)) < 0)
	{
		close(fd);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": ", FILE_OS_VERSION)));
	}

	close(fd);
	buffer[nbytes] = '\0';

	if (sscanf(buffer, "%s", buffer) < 1)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("unexpected file format: \"%s\"", FILE_OS_VERSION),
				 errdetail("number of fields is not corresponding")));


	values[1] = CStringGetTextDatum(buffer);


	tuple = heap_form_tuple(tupdesc, values, nulls);

	return HeapTupleGetDatum(tuple);
}

/*
 * Display /proc/loadavg
 */

#define NUM_LOADAVG_COLS		6

Datum
pg_proc_loadavg(PG_FUNCTION_ARGS)
{
	TupleDesc	tupdesc;
	HeapTuple	tuple;
	Datum		values[NUM_LOADAVG_COLS];
	bool		nulls[NUM_LOADAVG_COLS];
	LoadAvg		loadavg;
	int			i = 0;

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == lengthof(values));

	get_proc_loadavg(&loadavg);

	memset(nulls, 0, sizeof(nulls));
	memset(values, 0, sizeof(values));

	values[i++] = Float4GetDatum(loadavg.loadavg1);
	values[i++] = Float4GetDatum(loadavg.loadavg5);
	values[i++] = Float4GetDatum(loadavg.loadavg15);
	values[i++] = Int32GetDatum(loadavg.current_processes);
	values[i++] = Int32GetDatum(loadavg.total_processes);

	tuple = heap_form_tuple(tupdesc, values, nulls);

	return HeapTupleGetDatum(tuple);
}

/*
 * Display /proc/diskstats
 */

#define NUM_DISKSTATS_COLS 20

Datum
pg_proc_diskstats(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	Datum		values[NUM_DISKSTATS_COLS];
	bool		nulls[NUM_DISKSTATS_COLS];
	List	   *diskstats = NIL;
	ListCell   *lc;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));


	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");
	Assert(tupdesc->natts == NUM_DISKSTATS_COLS);

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	diskstats = get_proc_diskstats(diskstats);

	foreach(lc, diskstats)
	{
		DiskStat   *ds = (DiskStat *) lfirst(lc);
		int			i;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		i = 0;
		values[i++] = Int32GetDatum(ds->major);
		values[i++] = Int32GetDatum(ds->minor);
		values[i++] = CStringGetTextDatum(ds->name);
		values[i++] = Int64GetDatum(ds->rd);
		values[i++] = Int64GetDatum(ds->rd_merged);
		values[i++] = Int64GetDatum(ds->rd_sec);
		values[i++] = Int64GetDatum(ds->rd_tm);
		values[i++] = Int64GetDatum(ds->wr);
		values[i++] = Int64GetDatum(ds->wr_merged);
		values[i++] = Int64GetDatum(ds->wr_sec);

		values[i++] = Int64GetDatum(ds->wr_tm);
		values[i++] = Int64GetDatum(ds->io);
		values[i++] = Int64GetDatum(ds->tm);
		values[i++] = Int64GetDatum(ds->wtm);
		values[i++] = Int64GetDatum(ds->dis);
		values[i++] = Int64GetDatum(ds->dis_merged);
		values[i++] = Int64GetDatum(ds->dis_sec);
		values[i++] = Int64GetDatum(ds->dis_tm);
		values[i++] = Int64GetDatum(ds->fl);
		values[i++] = Int64GetDatum(ds->tm_fl);

		Assert(i == NUM_DISKSTATS_COLS);
		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		pfree(ds);
	}

	return (Datum) 0;
}

/*
 * Display /proc/meminfo
 */


#define NUM_MEMORY_COLS		50


Datum
pg_proc_meminfo(PG_FUNCTION_ARGS)
{
	TupleDesc	tupdesc;
	HeapTuple	tuple;
	Datum		values[NUM_MEMORY_COLS];
	bool		nulls[NUM_MEMORY_COLS];
	MemInfo		meminfo;
	int			i;

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	Assert(tupdesc->natts == lengthof(values));

	get_proc_meminfo(&meminfo);

	memset(nulls, 0, sizeof(nulls));
	memset(values, 0, sizeof(values));

	i = 0;
	values[i++] = Int64GetDatum(meminfo.MemTotal);
	values[i++] = Int64GetDatum(meminfo.MemFree);
	values[i++] = Int64GetDatum(meminfo.MemAvailable);
	values[i++] = Int64GetDatum(meminfo.Buffers);
	values[i++] = Int64GetDatum(meminfo.Cached);
	values[i++] = Int64GetDatum(meminfo.SwapCached);
	values[i++] = Int64GetDatum(meminfo.Active);
	values[i++] = Int64GetDatum(meminfo.Inactive);
	values[i++] = Int64GetDatum(meminfo.Active_anon);
	values[i++] = Int64GetDatum(meminfo.Inactive_anon);

	values[i++] = Int64GetDatum(meminfo.Active_file);
	values[i++] = Int64GetDatum(meminfo.Inactive_file);
	values[i++] = Int64GetDatum(meminfo.Unevictable);
	values[i++] = Int64GetDatum(meminfo.Mlocked);
	values[i++] = Int64GetDatum(meminfo.SwapTotal);
	values[i++] = Int64GetDatum(meminfo.SwapFree);
	values[i++] = Int64GetDatum(meminfo.Dirty);
	values[i++] = Int64GetDatum(meminfo.Writeback);
	values[i++] = Int64GetDatum(meminfo.AnonPages);
	values[i++] = Int64GetDatum(meminfo.Mapped);

	values[i++] = Int64GetDatum(meminfo.Shmem);
	values[i++] = Int64GetDatum(meminfo.KReclaimable);
	values[i++] = Int64GetDatum(meminfo.Slab);
	values[i++] = Int64GetDatum(meminfo.SReclaimable);
	values[i++] = Int64GetDatum(meminfo.SUnreclaim);
	values[i++] = Int64GetDatum(meminfo.KernelStack);
	values[i++] = Int64GetDatum(meminfo.PageTables);
	values[i++] = Int64GetDatum(meminfo.NFS_Unstable);
	values[i++] = Int64GetDatum(meminfo.Bounce);
	values[i++] = Int64GetDatum(meminfo.WritebackTmp);

	values[i++] = Int64GetDatum(meminfo.CommitLimit);
	values[i++] = Int64GetDatum(meminfo.Committed_AS);
	values[i++] = Int64GetDatum(meminfo.VmallocTotal);
	values[i++] = Int64GetDatum(meminfo.VmallocUsed);
	values[i++] = Int64GetDatum(meminfo.VmallocChunk);
	values[i++] = Int64GetDatum(meminfo.Percpu);
	values[i++] = Int64GetDatum(meminfo.HardwareCorrupted);
	values[i++] = Int64GetDatum(meminfo.AnonHugePages);
	values[i++] = Int64GetDatum(meminfo.ShmemHugePages);
	values[i++] = Int64GetDatum(meminfo.ShmemPmdMapped);

	values[i++] = Int64GetDatum(meminfo.FileHugePages);
	values[i++] = Int64GetDatum(meminfo.FilePmdMapped);
	values[i++] = Int64GetDatum(meminfo.CmaTotal);
	values[i++] = Int64GetDatum(meminfo.CmaFree);
	values[i++] = Int64GetDatum(meminfo.HugePages_Total);
	values[i++] = Int64GetDatum(meminfo.HugePages_Free);
	values[i++] = Int64GetDatum(meminfo.HugePages_Rsvd);
	values[i++] = Int64GetDatum(meminfo.HugePages_Surp);
	values[i++] = Int64GetDatum(meminfo.Hugepagesize);
	values[i++] = Int64GetDatum(meminfo.Hugetlb);

	tuple = heap_form_tuple(tupdesc, values, nulls);

	return HeapTupleGetDatum(tuple);
}


/*
 * Display only cpu items in /proc/stat
 */

#define NUM_STAT_COLS 9

Datum
pg_proc_stat(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	Datum		values[NUM_STAT_COLS];
	bool		nulls[NUM_STAT_COLS];
	List	   *stats = NIL;
	ListCell   *lc;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));


	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");
	Assert(tupdesc->natts == NUM_STAT_COLS);

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	stats = get_proc_stat(stats);

	foreach(lc, stats)
	{
		ProcStat   *ps = (ProcStat *) lfirst(lc);
		int			i;

		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		i = 0;
		values[i++] = CStringGetTextDatum(ps->cpu);
		values[i++] = Int64GetDatum(ps->user);
		values[i++] = Int64GetDatum(ps->nice);
		values[i++] = Int64GetDatum(ps->system);
		values[i++] = Int64GetDatum(ps->idle);
		values[i++] = Int64GetDatum(ps->iowait);
		values[i++] = Int64GetDatum(ps->irq);
		values[i++] = Int64GetDatum(ps->softirq);
		values[i++] = Int64GetDatum(ps->steal);

		Assert(i == NUM_STAT_COLS);
		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		pfree(ps);
	}

	return (Datum) 0;
}
