AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -g -Wall -Werror \
			  -I$(top_srcdir)/framework \
			  -I$(top_srcdir)/util \
			  -I$(top_srcdir)/app/core \
			  -I$(top_srcdir)/libs/rapidjson/include 
			  
AM_CFLAGS = -g -Wall -Werror \
			-I$(top_srcdir)/framework \
			-I$(top_srcdir)/util \
			-I$(top_srcdir)/app/core \
			-I$(top_srcdir)/libs/rapidjson/include 
			
			
AM_LDFLAGS = -pthread -lrt -rdynamic \
    $(CURL_LIBS) \
    -lavutil \
    -lavcodec \
    -lavformat \
    -lavfilter \
    -lswresample \
    -lalsa_intf \
    -logg \
    -lvorbis \
    -lvorbisenc


#AM_LDFLAGS = -pthread -lrt -rdynamic -llog

bin_PROGRAMS = cubic-logd cubic-read-log
cubic_logd_SOURCES = CLogService.cc
cubic_read_log_SOURCES = CLogReader.cc

EXTRA_DIST = CLogFile.cc \
             CLogNet.cc \
             CLogService.h
