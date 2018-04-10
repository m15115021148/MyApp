AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/libs/rapidjson/include
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/libs/rapidjson/include
AM_LDFLAGS = -pthread -lrt -rdynamic $(CURL_LIBS)
bin_PROGRAMS = cubic-lightd
cubic_lightd_SOURCES = LightService.cc

