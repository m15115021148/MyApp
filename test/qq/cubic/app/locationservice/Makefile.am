AUTOMAKE_OPTIONS = foreign
bin_PROGRAMS = cubic-locd
AM_CPPFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util 
AM_CFLAGS = -g -Wall -Werror -I$(top_srcdir) -I$(top_srcdir)/framework -I$(top_srcdir)/util  
AM_LDFLAGS = -pthread -lrt -rdynamic -lqmi_cci -lgps_default_so -lgps_api_so 

INCLUDES = -I$(WORKSPACE)/hardware/qcom/gps/gps_api/libgps_api \
	   -I$(WORKSPACE)/hardware/qcom/gps/gps_api/gps_ctl_srv \
	   -I$(WORKSPACE)/hardware/qcom/gps/core \
	   -I$(WORKSPACE)/hardware/qcom/gps/loc_api/loc_api_v02 \
	   -I$(WORKSPACE)/hardware/qcom/gps

cubic_locd_SOURCES = CLocationService.cc 
