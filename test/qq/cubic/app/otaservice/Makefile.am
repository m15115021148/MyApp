AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cubic-otad
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/libs/rapidjson/include -I$(top_srcdir)/libs/rapidjson/include/rapidjson -I$(top_srcdir)/app/bleinterface/action
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/libs/rapidjson/include -I$(top_srcdir)/libs/rapidjson/include/rapidjson -I$(top_srcdir)/app/bleinterface/action
AM_LDFLAGS = -pthread -lrt -rdynamic -lqmi_cci -lcrypto $(CURL_LIBS)
cubic_otad_SOURCES = OTAService.cc
