#ifndef _CDATA_OBJECT_TYPES_H_
#define _CDATA_OBJECT_TYPES_H_  1

#define  DATA_OBJECT_NAME_SETPASSWORD       "SetPassword"
#define  DATA_OBJECT_NAME_GROUPINVITATION   "GroupInvitation"
#define  DATA_OBJECT_NAME_SETSIP            "SetSip"
#define  DATA_OBJECT_NAME_SETSERVERURL      "SetServerUrl"
#define  DATA_OBJECT_NAME_SETAPNSETTINGS    "SetApnSettings"
#define  DATA_OBJECT_NAME_SETTHRESHOLDS     "SetThresholds"
#define  DATA_OBJECT_NAME_SETGEOFENCE       "SetGeoFence"
#define  DATA_OBJECT_NAME_SETINTERVAL       "SetIntervals"
#define  DATA_OBJECT_NAME_LOCKSIMWITHPIN    "LockSimWithPin"
#define  DATA_OBJECT_NAME_UNLOCKSIMWITHPIN  "UnlockSimWithPin"
#define  DATA_OBJECT_NAME_UNLOCKSIMWITHPUK  "UnlockSimWithPuk"
#define  DATA_OBJECT_NAME_GETSIP            "GetSip"
#define  DATA_OBJECT_NAME_GETDEVICESTATUS   "GetDeviceStatus"
#define  DATA_OBJECT_NAME_GETLOG            "GetLog"
#define  DATA_OBJECT_NAME_GETAPNSETTINGS    "GetApnSettings"
#define  DATA_OBJECT_NAME_GETTHRESHOLDS     "GetThresholds"
#define  DATA_OBJECT_NAME_GETGEOFENCE       "GetGeoFence"
#define  DATA_OBJECT_NAME_GETINTERVALS      "GetIntervals"
#define  DATA_OBJECT_NAME_GETSIMLOCKSTATUS  "GetSimLockStatus"
#define  DATA_OBJECT_NAME_UPGRADEFIRMWARE   "UpgradeFirmware"


#define  DATA_OBJECT_NAME_TEST  "Test"


typedef enum DataObj_Type
{
    CDATA_OBJECT_BASE = 0,
    CDATA_OBJECT_SETPASSWORD = 1,
    CDATA_OBJECT_GROUPINVITATION = 2,
    CDATA_OBJECT_SETSIP = 3,
    CDATA_OBJECT_SETSERVERURL = 4,
    CDATA_OBJECT_SETAPNSETTINGS = 5,
    CDATA_OBJECT_SETTHRESHOLDS = 6,
    CDATA_OBJECT_SETGEOFENCE = 7,
    CDATA_OBJECT_SETINTERVAL = 8,
    CDATA_OBJECT_LOCKSIMWITHPIN = 9,
    CDATA_OBJECT_UNLOCKSIMWITHPIN = 10,
    CDATA_OBJECT_UNLOCKSIMWITHPUK = 11,
    CDATA_OBJECT_GETSIP = 12,
    CDATA_OBJECT_GETDEVICESTATUS = 13,
    CDATA_OBJECT_GETLOG = 14,
    CDATA_OBJECT_GETAPNSETTINGS = 15,
    CDATA_OBJECT_GETTHRESHOLDS = 16,
    CDATA_OBJECT_GETGEOFENCE = 17,
    CDATA_OBJECT_GETINTERVALS = 18,
    CDATA_OBJECT_GETSIMLOCKSTATUS = 19,
    CDATA_OBJECT_UPGRADEFIRMWARE = 20,
    CDATA_OBJECT_TEST = 100,

} EDataObj_Type;


#endif
