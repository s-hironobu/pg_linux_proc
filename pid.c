/*-------------------------------------------------------------------------
 *
 * pid.c
 *		Get pid list from /proc on Linux
 *
 * Copyright (c) 2008-2025, PostgreSQL Global Development Group
 * Copyright (c) 2024-2025, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/pg_list.h"

#include <dirent.h>
#include "pid.h"

/*
 * Read cmdline from /proc/pid
 */
static void
get_cmdline(int64 pid, char *cmdline)
{
	FILE	   *fp;
	char		line[32];

	sprintf(line, "/proc/%ld/cmdline", pid);

	if ((fp = fopen(line, "r")) == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", line)));

	/*
	 * We handle NULL because it doesn't indicate an error when the command
	 * line cannot be read.
	 */
	if (fgets(cmdline, sizeof(char) * MAX_CMDLINE, fp) == NULL)
		elog(DEBUG1, "could not read file \"%s\".", line);

	fclose(fp);
}



List *
get_proc_pid(List *stat)
{
	DIR		   *dir;

	struct dirent *dp;

	if ((dir = opendir(DIR_PID)) == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open dir \"%s\": ", DIR_PID)));

	while ((dp = readdir(dir)) != NULL)
	{
		ProcPid    *ps;

		if (dp->d_name[0] >= '0' && dp->d_name[0] <= '9')
		{

			ps = (ProcPid *) palloc0(sizeof(ProcPid));

			if (sscanf(dp->d_name, "%ld", &(ps->pid)) != 1)
				ereport(ERROR,
						(errcode(ERRCODE_DATA_EXCEPTION),
						 errmsg("unexpected file format: \"%s\"", DIR_PID),
						 errdetail("number of fields is not corresponding")));

			get_cmdline(ps->pid, ps->cmdline);
			if (ps->cmdline != NULL)
				stat = lappend(stat, ps);
		}
	}

	closedir(dir);

	return stat;
}
