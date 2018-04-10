AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cubic-bled
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util -I$(top_srcdir)/app/bleinterface/action -I$(top_srcdir)/libs/rapidjson/include/rapidjson
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util  -I$(top_srcdir)/app/bleinterface/action -I$(top_srcdir)/libs/rapidjson/include/rapidjson
AM_LDFLAGS = -pthread -lrt -rdynamic -lserialport -lcrypto
cubic_bled_SOURCES = BleInterface.cc
sysconf_DATA = ble580.bin