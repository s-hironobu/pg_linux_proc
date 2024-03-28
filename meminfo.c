/*-------------------------------------------------------------------------
 *
 * meminfo.c
 *		Show /proc/meminfo info on Linux
 *
 * Copyright (c) 2008-2024, PostgreSQL Global Development Group
 * Copyright (c) 2024, Hironobu Suzuki @ interdb.jp
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "meminfo.h"


bool
get_proc_meminfo(MemInfo * meminfo)
{
	FILE	   *fp;
	char		line[64];

	meminfo_store meminfo_stores[] =
	{
		{"MemTotal:", &(meminfo->MemTotal)},
		{"MemFree:", &(meminfo->MemFree)},
		{"MemAvailable:", &(meminfo->MemAvailable)},
		{"Buffers:", &(meminfo->Buffers)},
		{"Cached:", &(meminfo->Cached)},
		{"SwapCached:", &(meminfo->SwapCached)},
		{"Active:", &(meminfo->Active)},
		{"Inactive:", &(meminfo->Inactive)},
		{"Active(anon):", &(meminfo->Active_anon)},
		{"Inactive(anon):", &(meminfo->Inactive_anon)},
		{"Active(file):", &(meminfo->Active_file)},
		{"Inactive(file):", &(meminfo->Inactive_file)},
		{"Unevictable:", &(meminfo->Unevictable)},
		{"Mlocked:", &(meminfo->Mlocked)},
		{"SwapTotal:", &(meminfo->SwapTotal)},
		{"SwapFree:", &(meminfo->SwapFree)},
		{"Dirty:", &(meminfo->Dirty)},
		{"Writeback:", &(meminfo->Writeback)},
		{"AnonPages:", &(meminfo->AnonPages)},
		{"Mapped:", &(meminfo->Mapped)},
		{"Shmem:", &(meminfo->Shmem)},
		{"KReclaimable:", &(meminfo->KReclaimable)},
		{"Slab:", &(meminfo->Slab)},
		{"SReclaimable:", &(meminfo->SReclaimable)},
		{"SUnreclaim:", &(meminfo->SUnreclaim)},
		{"KernelStack:", &(meminfo->KernelStack)},
		{"PageTables:", &(meminfo->PageTables)},
		{"NFS_Unstable:", &(meminfo->NFS_Unstable)},
		{"Bounce:", &(meminfo->Bounce)},
		{"WritebackTmp:", &(meminfo->WritebackTmp)},
		{"CommitLimit:", &(meminfo->CommitLimit)},
		{"Committed_AS:", &(meminfo->Committed_AS)},
		{"VmallocTotal:", &(meminfo->VmallocTotal)},
		{"VmallocUsed:", &(meminfo->VmallocUsed)},
		{"VmallocChunk:", &(meminfo->VmallocChunk)},
		{"Percpu:", &(meminfo->Percpu)},
		{"HardwareCorrupted:", &(meminfo->HardwareCorrupted)},
		{"AnonHugePages:", &(meminfo->AnonHugePages)},
		{"ShmemHugePages:", &(meminfo->ShmemHugePages)},
		{"ShmemPmdMapped:", &(meminfo->ShmemPmdMapped)},
		{"FileHugePages:", &(meminfo->FileHugePages)},
		{"FilePmdMapped:", &(meminfo->FilePmdMapped)},
		{"CmaTotal:", &(meminfo->CmaTotal)},
		{"CmaFree:", &(meminfo->CmaFree)},
		{"HugePages_Total:", &(meminfo->HugePages_Total)},
		{"HugePages_Free:", &(meminfo->HugePages_Free)},
		{"HugePages_Rsvd:", &(meminfo->HugePages_Rsvd)},
		{"HugePages_Surp:", &(meminfo->HugePages_Surp)},
		{"Hugepagesize:", &(meminfo->Hugepagesize)},
		{"Hugetlb:", &(meminfo->Hugetlb)}
	};

	if ((fp = fopen(FILE_MEMINFO, "r")) == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": ", FILE_MEMINFO)));

	while (fgets(line, sizeof(line), fp) != NULL)
	{
		int			i;
		int			store_size;

		store_size = sizeof(meminfo_stores) / sizeof(meminfo_store);

		for (i = 0; i < store_size; i++)
		{
			meminfo_store *m = &(meminfo_stores[i]);
			int			len = strlen(m->key);

			if (strncmp(m->key, line, len) == 0)
			{
				if (sscanf(line, "%*s %ld %*s", m->value) < 1)
					ereport(ERROR,
							(errcode(ERRCODE_DATA_EXCEPTION),
							 errmsg("unexpected file format: \"%s\"", FILE_MEMINFO),
							 errdetail("number of fields is not corresponding")));

				break;
			}
		}
	}


	if (ferror(fp) && errno != EINTR)
	{
		fclose(fp);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": ", FILE_MEMINFO)));
	}

	fclose(fp);


	return true;
}
