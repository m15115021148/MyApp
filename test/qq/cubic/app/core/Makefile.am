AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -g -Wall -Werror \
              -I$(top_srcdir) \
              -I$(top_srcdir)/framework \
              -I$(top_srcdir)/util \
              -I$(top_srcdir)/libs/rapidjson/include \
              $(CURL_CFLAGS)

AM_CFLAGS = -g -Wall -Werror \
            -I$(top_srcdir) \
            -I$(top_srcdir)/framework \
            -I$(top_srcdir)/util \
            -I$(top_srcdir)/libs/rapidjson/include \
            $(CURL_CFLAGS)

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


bin_PROGRAMS = cubic-core
cubic_core_SOURCES = CoreApp.cc

