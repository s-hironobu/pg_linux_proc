# pg_linux_proc

`pg_linux_proc` is a set of PostgreSQL functions that provides information under `/proc` directory.


It was developed to simplify OS statistics monitoring without requiring the use of heavyweight solutions like Zabbix and Prometheus.


## Installation

You can install it to do the usual way shown below.

```
$ tar xvfj postgresql-16.0.tar.bz2
$ cd postgresql-16.0/contrib
$ git clone https://github.com/s-hironobu/pg_linux_proc.git
$ cd pg_linux_proc
$ make && make install
```

You must add the line shown below in your postgresql.conf.

```
shared_preload_libraries = 'pg_linux_proc'
```

After starting your server, you must issue `CREATE EXTENSION` statement shown below.

```
postgres=# CREATE EXTENSION pg_linux_proc;
```

## How to use

### pg_proc('subdir')

`pg_proc(subdir)` displays information for specified sub-directory under `/proc`.

Here are examples display `/proc/version_signature` and `/proc/misc`:

```
testdb=# select * from pg_proc('version_signature');
                pg_proc
---------------------------------------
 Ubuntu 5.15.0-94.104-generic 5.15.136+

(1 row)

testdb=# select * from pg_proc('misc');
       pg_proc
---------------------
 122 vboxuser       +
 123 vboxguest      +
 235 autofs         +
 234 btrfs-control  +
 124 cpu_dma_latency+
... snip ...
 231 snapshot       +
 242 rfkill         +
 127 vga_arbiter    +

(1 row)
```

### pg_proc_pid()

`pg_proc_pid()` shows running process IDs (PIDs) and their names.

```
testdb=# select * from pg_proc_pid();
  pid   |                         cmdline
--------+---------------------------------------------------------
      1 | /sbin/init
      2 |
... snip ...
 311206 |
 311288 |
 311330 | /home/vagrant/pgsql/bin/postgres
 311331 | postgres: logger
 311332 | postgres: checkpointer
 311333 | postgres: background writer
 311335 | postgres: walwriter
 311336 | postgres: autovacuum launcher
 311337 | postgres: logical replication launcher
 311338 | ./bin/psql
 311339 | postgres: vagrant testdb [local] SELECT
(113 rows)
```

### pg_proc() and pg_proc_pid()

Using these functions, we can access all of information from the `/proc` directory in principle.

```
testdb=# select * from pg_proc('311336/cmdline');
            pg_proc
--------------------------------
 postgres: autovacuum launcher
(1 row)
testdb=# select * from pg_proc('311336/net/arp');
                                    pg_proc
--------------------------------------------------------------------------------
 IP address       HW type     Flags       HW address            Mask     Device+
 192.168.128.175  0x1         0x2         bc:d0:74:28:77:c7     *        enp0s9+
 10.0.2.2         0x1         0x2         52:54:00:12:35:02     *        enp0s3+
 10.0.2.3         0x1         0x2         52:54:00:12:35:03     *        enp0s3+
 192.168.128.1    0x1         0x2         4e:3f:c7:9c:8b:7d     *        enp0s9+

(1 row)
```
### Other functions

Several functions are available to easily access specific files.

#### pg_os_version()

```
testdb=# select * from pg_os_version();
 type  |      version
-------+-------------------
 Linux | 5.15.0-94-generic
(1 row)
```

#### pg_proc_loadavg()

```
testdb=# select * from pg_proc_loadavg();
 loadavg1 | loadavg5 | loadavg15 | current_processes | total_processes
----------+----------+-----------+-------------------+-----------------
        0 |        0 |         0 |                 2 |             140
(1 row)
```
#### pg_proc_diskstats()

```
testdb=# \x
Expanded display is on.
testdb=# select * from pg_proc_diskstats();
-[ RECORD 1 ]---------
major      | 7
minor      | 0
dev_name   | loop0
rd         | 61
rd_merged  | 0
rd_sec     | 794
rd_tm      | 28
wr         | 0
wr_merged  | 0
wr_sec     | 0
wr_tm      | 0
io         | 0
tm         | 52
wtm        | 28
dis        | 0
dis_merged | 0
dis_sec    | 0
dis_tm     | 0
fl         | 0
tm_fl      | 0
-[ RECORD 2 ]---------
major      | 7
minor      | 1
dev_name   | loop1
rd         | 1714
rd_merged  | 0
rd_sec     | 99394
rd_tm      | 946
wr         | 0
wr_merged  | 0
... snip ...
```

#### pg_proc_meminfo()

```
testdb=# select * from pg_proc_meminfo();
-[ RECORD 1 ]-----+---------------
memtotal          | 980320
memfree           | 100340
memavailable      | 514156
buffers           | 32892
cached            | 537552
swapcached        | 0
active            | 310352
inactive          | 387512
active_anon       | 5620
inactive_anon     | 178724
active_file       | 304732
inactive_file     | 208788
unevictable       | 27784
mlocked           | 27784
swaptotal         | 0
swapfree          | 0
dirty             | 24
writeback         | 0
anonpages         | 155212
mapped            | 131576
shmem             | 47852
kreclaimable      | 55584
slab              | 95732
sreclaimable      | 55584
sunreclaim        | 40148
kernelstack       | 2240
pagetables        | 3688
nfs_unstable      | 0
bounce            | 0
writebacktmp      | 0
commitlimit       | 490160
committed_as      | 705980
vmalloctotal      | 34359738367
vmallocused       | 17028
vmallocchunk      | 0
percpu            | 1288
hardwarecorrupted | 0
anonhugepages     | 0
shmemhugepages    | 0
shmempmdmapped    | 0
filehugepages     | 0
filepmdmapped     | 0
cmatotal          | 94765210203776
cmafree           | 94765191834997
hugepages_total   | 0
hugepages_free    | 0
hugepages_rsvd    | 0
hugepages_surp    | 0
hugepagesize      | 2048
hugetlb           | 0

```

#### pg_proc_stat()

This shows only cpu items in `/proc/stat`.

```
testdb=# select * from pg_proc_stat();
 cpu  |  usr   | nice  | system |   idle    | iowait | irq | softirq | steal
------+--------+-------+--------+-----------+--------+-----+---------+-------
 cpu0 | 502595 | 13988 | 351453 | 313147621 |  20962 |   0 |   28700 |     0
 cpu1 | 420270 | 17765 | 284332 | 304090660 |  14132 |   0 |   78664 |     0
(2 rows)
```


## Change Log
 - 28 Mar, 2024: Version 1.0 Released.
