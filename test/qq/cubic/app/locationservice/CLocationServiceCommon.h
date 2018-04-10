#ifndef __CLOCATIONSERVICECOMMON_H__
#define __CLOCATIONSERVICECOMMON_H__ 1

#include <iostream>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#include "gps_ctl_api.h"
#include "gps_ctl_uart.h"
}
#endif
#define SIGNAL_STRENGTH_NONE_OR_UNKNOWN 0
#define SIGNAL_STRENGTH_POOR 1
#define SIGNAL_STRENGTH_GOOD 2
#define SIGNAL_STRENGTH_GREAT 3
#define LOC_INFO_VALID TRUE
#define LOC_INFO_INVALID FALSE
#define GEOFENCE_INDEX_MAX 16
#define CUBIC_MSG_GPS_GEOFENCE_MEASURE 200
#define LATITUDE_M 111000


#endif
