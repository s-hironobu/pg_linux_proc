/* pg_linux_proc--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pg_linux_proc" to load this file. \quit

-- Register functions.

CREATE OR REPLACE FUNCTION pg_proc(
       IN  t text
)
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION pg_proc_pid(
       OUT pid int,
       OUT cmdline text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION pg_os_version(
       OUT type text,
       OUT version text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION pg_proc_loadavg(
       OUT loadavg1 real,
       OUT loadavg5 real,
       OUT loadavg15 real,
       OUT current_processes int,
       OUT total_processes int
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION pg_proc_diskstats(
       OUT major int,
       OUT minor int,
       OUT dev_name varchar,
       OUT rd int,
       OUT rd_merged int,
       OUT rd_sec int,
       OUT rd_tm int,
       OUT wr int,
       OUT wr_merged int,
       OUT wr_sec int,
       OUT wr_tm int,
       OUT io int,
       OUT tm int,
       OUT wtm int,
       OUT dis int,
       OUT dis_merged int,
       OUT dis_sec int,
       OUT dis_tm int,
       OUT fl int,
       OUT tm_fl int
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION pg_proc_meminfo(
	OUT MemTotal bigint,
	OUT MemFree bigint,
	OUT MemAvailable bigint,
	OUT Buffers bigint,
	OUT Cached bigint,
	OUT SwapCached bigint,
	OUT Active bigint,
	OUT Inactive bigint,
	OUT Active_anon bigint,
	OUT Inactive_anon bigint,

	OUT Active_file bigint,
	OUT Inactive_file bigint,
	OUT Unevictable bigint,
	OUT Mlocked bigint,
	OUT SwapTotal bigint,
	OUT SwapFree bigint,
	OUT Dirty bigint,
	OUT Writeback bigint,
	OUT AnonPages bigint,
	OUT Mapped bigint,

	OUT Shmem bigint,
	OUT KReclaimable bigint,
	OUT Slab bigint,
	OUT SReclaimable bigint,
	OUT SUnreclaim bigint,
	OUT KernelStack bigint,
	OUT PageTables bigint,
	OUT NFS_Unstable bigint,
	OUT Bounce bigint,
	OUT WritebackTmp bigint,

	OUT CommitLimit bigint,
	OUT Committed_AS bigint,
	OUT VmallocTotal bigint,
	OUT VmallocUsed bigint,
	OUT VmallocChunk bigint,
	OUT Percpu bigint,
	OUT HardwareCorrupted bigint,
	OUT AnonHugePages bigint,
	OUT ShmemHugePages bigint,
	OUT ShmemPmdMapped bigint,

	OUT FileHugePages bigint,
	OUT FilePmdMapped bigint,
	OUT CmaTotal bigint,
	OUT CmaFree bigint,
	OUT HugePages_Total bigint,
	OUT HugePages_Free bigint,
	OUT HugePages_Rsvd bigint,
	OUT HugePages_Surp bigint,
	OUT Hugepagesize bigint,
	OUT Hugetlb bigint
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION pg_proc_stat(
       OUT cpu text,
       OUT usr int,
       OUT nice int,
       OUT system int,
       OUT idle int,
       OUT iowait int,
       OUT irq int,
       OUT softirq int,
       OUT steal int
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


-- GRANT SELECT ON pg_show_vm TO PUBLIC;

-- Don't want this to be available to non-superusers.
--REVOKE ALL ON FUNCTION pg_show_vm() FROM PUBLIC;
