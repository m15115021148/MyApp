AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util 
AM_LDFLAGS = -pthread -lrt -rdynamic

bin_PROGRAMS = cmmd cevt read_comm cshut

cmmd_SOURCES = cmmd.cc

cevt_SOURCES = getevent.cc

read_comm_SOURCES = read_comm.cc

cshut_SOURCES = cshutdown.cc

