AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cubic-netd
AM_CPPFLAGS = -g -Wunused-variable -Wreturn-type -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util 
AM_CFLAGS = -g -Wunused-variable -Wreturn-type -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util  
AM_LDFLAGS = -pthread -lrt -rdynamic -lqmi_cci -lqmiservices -lqcmap_cm -lqcmap_client

INCLUDES = -I$(WORKSPACE)/qmi-framework/inc \
		   -I$(WORKSPACE)/qmi-framework/qcci/inc \
		   -I$(WORKSPACE)/qmi/services \
		   -I$(WORKSPACE)/data/mobileap/ipc \
		   -I$(WORKSPACE)/data/mobileap/client/inc \
		   -I$(WORKSPACE)/data/dsutils/inc

cubic_netd_SOURCES = CNetworkService.cc
