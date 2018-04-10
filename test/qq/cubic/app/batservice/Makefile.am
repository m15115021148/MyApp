AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cubic-batd
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util 
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util  
AM_LDFLAGS = -pthread -lrt -rdynamic -lqmi_cci 
cubic_batd_SOURCES = CBatService.cc
