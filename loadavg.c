/*-------------------------------------------------------------------------
 *
 * loadavg.c
 *		Show /proc/loadavg info on Linux
 *
 * Copyright (c) 2008-2024, PostgreSQL Global Development Group
 * Copyright (c) 2024, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "loadavg.h"

bool
get_proc_loadavg(struct LoadAvg *loadavg)
{
	int			fd;
	char		buffer[64];
	int			nbytes;

	/* extract loadavg information */
	if ((fd = open(FILE_LOADAVG, O_RDONLY)) < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", FILE_LOADAVG)));

	if ((nbytes = read(fd, buffer, sizeof(buffer) - 1)) < 0)
	{
		close(fd);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": ", FILE_LOADAVG)));
	}

	close(fd);
	buffer[nbytes] = '\0';

	if (sscanf(buffer, "%f %f %f %d/%d %d",
			   &(loadavg->loadavg1), &(loadavg->loadavg5), &(loadavg->loadavg15),
			   &(loadavg->current_processes), &(loadavg->total_processes),
			   &(loadavg->last_pid)) < NUM_LOADAVG_FIELDS_MIN)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_EXCEPTION),
				 errmsg("unexpected file format: \"%s\"", FILE_LOADAVG),
				 errdetail("number of fields is not corresponding")));



	return true;
}
