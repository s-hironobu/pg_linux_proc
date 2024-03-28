/*-------------------------------------------------------------------------
 *
 * diskstats.c
 *		Get /proc/diskstats on Linux
 *
 * Copyright (c) 2008-2024, PostgreSQL Global Development Group
 * Copyright (c) 2024, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/pg_list.h"

#include "diskstats.h"

/*
Date:		February 2008
Contact:	Jerome Marchand <jmarchan@redhat.com>
Description:
		The /proc/diskstats file displays the I/O statistics
		of block devices. Each line contains the following 14
		fields:

		==  ===================================
		 1  major number
		 2  minor number
		 3  device name
		 4  reads completed successfully
		 5  reads merged
		 6  sectors read
		 7  time spent reading (ms)
		 8  writes completed
		 9  writes merged
		10  sectors written
		11  time spent writing (ms)
		12  I/Os currently in progress
		13  time spent doing I/Os (ms)
		14  weighted time spent doing I/Os (ms)
		==  ===================================

		Kernel 4.18+ appends four more fields for discard
		tracking putting the total at 18:

		==  ===================================
		15  discards completed successfully
		16  discards merged
		17  sectors discarded
		18  time spent discarding
		==  ===================================

		Kernel 5.5+ appends two more fields for flush requests:

		==  =====================================
		19  flush requests completed successfully
		20  time spent flushing
		==  =====================================

 */

List *
get_proc_diskstats(List *diskstats)
{
	FILE	   *fp;
	char		line[256];

	if ((fp = fopen(FILE_DISKSTATS, "r")) == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", FILE_DISKSTATS)));

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		DiskStat   *ds;

		ds = palloc0(sizeof(DiskStat));

		if (sscanf(line, "%d %d %s %ld %ld  %ld %ld %ld %ld %ld  %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld",
				   &(ds->major), &(ds->minor), ds->name, &(ds->rd), &(ds->rd_merged), &(ds->rd_sec), &(ds->rd_tm),
				   &(ds->wr), &(ds->wr_merged), &(ds->wr_sec), &(ds->wr_tm), &(ds->io), &(ds->tm), &(ds->wtm),
				   &(ds->dis), &(ds->dis_merged), &(ds->dis_sec), &(ds->dis_tm), &(ds->fl), &(ds->tm_fl)) < 20)
			ereport(ERROR,
					(errcode(ERRCODE_DATA_EXCEPTION),
					 errmsg("unexpected file format: \"%s\"", FILE_DISKSTATS),
					 errdetail("number of fields is not corresponding")));

		diskstats = lappend(diskstats, ds);

	}


	if (ferror(fp) && errno != EINTR)
	{
		fclose(fp);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": ", FILE_DISKSTATS)));
	}

	fclose(fp);

	return diskstats;
}
