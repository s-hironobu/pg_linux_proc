/*-------------------------------------------------------------------------
 *
 * stat.c
 *		Get /proc/stat on Linux
 *
 * Copyright (c) 2008-2025, PostgreSQL Global Development Group
 * Copyright (c) 2024-2025, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/pg_list.h"

#include "stat.h"

List *
get_proc_stat(List *stat)
{
	FILE	   *fp;
	char		line[256];

	if ((fp = fopen(FILE_STAT, "r")) == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", FILE_STAT)));

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		ProcStat   *ps;

		if (strncmp(line, "cpu", 3) == 0 && line[3] >= '0' && line[3] <= '9')
		{

			ps = palloc0(sizeof(ProcStat));

			if (sscanf(line, "%s %ld %ld %ld %ld %ld %ld %ld %ld",
					   ps->cpu, &(ps->user), &(ps->nice), &(ps->system), &(ps->idle),
					   &(ps->iowait), &(ps->irq), &(ps->softirq), &(ps->steal)) < NUM_STAT_FIELDS_MIN)
				ereport(ERROR,
						(errcode(ERRCODE_DATA_EXCEPTION),
						 errmsg("unexpected file format: \"%s\"", FILE_STAT),
						 errdetail("number of fields is not corresponding")));

			stat = lappend(stat, ps);
		}
	}


	if (ferror(fp) && errno != EINTR)
	{
		fclose(fp);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": ", FILE_STAT)));
	}

	fclose(fp);

	return stat;
}
