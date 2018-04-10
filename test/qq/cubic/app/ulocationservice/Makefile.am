AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cubic-ulocd
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/libs/nmealib/include
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/libs/nmealib/include
AM_LDFLAGS = -pthread -lrt -rdynamic -lnmea -lserialport


cubic_ulocd_SOURCES = ULocationService.cc
