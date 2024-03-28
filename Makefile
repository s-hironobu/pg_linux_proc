# pg_linux_proc/Makefile

MODULE_big = pg_linux_proc
OBJS = pg_linux_proc.o diskstats.o meminfo.o loadavg.o stat.o pid.o

EXTENSION = pg_linux_proc
DATA = pg_linux_proc--1.0.sql

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/pg_linux_proc
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif

