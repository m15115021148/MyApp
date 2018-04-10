/**
 * @file SetGeoFence.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _SETGEOFENCE_CC_
#define _SETGEOFENCE_CC_ 1

#include "cubic_inc.h"
#include <iostream>
#include <termios.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "document.h"
#include "allocators.h"
#include "stringbuffer.h"
#include "writer.h"
#include "JsonInterface.cc"
#include "CStringTool.cc"
#include <assert.h>
#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SetGeoFence"


using namespace std;
using namespace rapidjson;

#define DEF_GEO_FENCES_KEY      "geo_fences"
#define DEF_INDEX_KEY           "index"
#define DEF_NAME_KEY            "name"
#define DEF_LATITUDE_KEY        "latitude"
#define DEF_LONGITUDE_KEY       "longitude"
#define DEF_RADIUS_KEY          "radius"

#define DEF_SET_GEOFENCE_NUMBER 10
#define DEF_GEOFENCE_CONFIG_PATH "/etc/cfg/rw/location/fence"

class SetGeoFence: public CDataObject
{
private:
    string method;
    unsigned int geofenceNum;
    typedef struct t_geoFence
    {
        int      index;
        string   name;
        float    latitude;
        float    longitude;
        int      radius;
    } GeoFence_t;
    GeoFence_t geofence[DEF_SET_GEOFENCE_NUMBER];

    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;
        unsigned int i = 0;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, DEF_GEO_FENCES_KEY, str_ret ) != 200 )
        {
            return 400;
        }

        Value &array = doc[DEF_GEO_FENCES_KEY];

        if( !array.IsArray() )
        {
            str_ret = "Current '";
            str_ret += DEF_GEO_FENCES_KEY;
            str_ret += "' not a array.";
            return 400;
        }

        geofenceNum = MIN( array.Capacity(), DEF_SET_GEOFENCE_NUMBER );

        for( i = 0; i < geofenceNum; i++ )
        {
            Value &obj = array[i];

            if( ( JsonInterface::checkMember( obj , DEF_INDEX_KEY, str_ret ) != 200 ) ||
                    ( JsonInterface::checkMember( obj , DEF_NAME_KEY, str_ret ) != 200 ) ||
                    ( JsonInterface::checkMember( obj , DEF_LATITUDE_KEY, str_ret ) != 200 ) ||
                    ( JsonInterface::checkMember( obj , DEF_LONGITUDE_KEY, str_ret ) != 200 ) ||
                    ( JsonInterface::checkMember( obj , DEF_RADIUS_KEY, str_ret ) != 200 ) )
            {
                return 400;
            }

            if( !obj[DEF_INDEX_KEY].IsNumber() ||
                    !obj[DEF_NAME_KEY].IsString() ||
                    !obj[DEF_LATITUDE_KEY].IsNumber() ||
                    !obj[DEF_LONGITUDE_KEY].IsNumber() ||
                    !obj[DEF_RADIUS_KEY].IsNumber() )
            {
                str_ret = "Member type has error.";
                return 400;
            }

            geofence[i].index = obj[DEF_INDEX_KEY].GetInt();
            geofence[i].name = obj[DEF_NAME_KEY].GetString();
            geofence[i].latitude = obj[DEF_LATITUDE_KEY].GetFloat();
            geofence[i].longitude = obj[DEF_LONGITUDE_KEY].GetFloat();
            geofence[i].radius = obj[DEF_RADIUS_KEY].GetInt();

            if( geofence[i].name.length() == 0 )
            {
                str_ret = "in the Array, name can not be empty.";
                return 400;
            }

            if( ( ( geofence[i].latitude < -90 ) || ( geofence[i].latitude > 90 ) )
                    || (  ( geofence[i].longitude < -180 ) || ( geofence[i].longitude > 180 ) ) )
            {
                str_ret = "latitude longitude not available";
                return 400;
            }
        }

        if( i == geofenceNum )
        {
            str_ret = "OK";
            return 200;
        }
        else
        {
            str_ret = "Abnormal operation";
            return 400;
        }
    };

public:

    virtual ~SetGeoFence()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        string str_ret;
        int n_ret;
        method = "SetGeoFence";

        if( ( n_ret = JsonOperation( json_req, str_ret ) ) == 200 )
        {
            //clear gps config
            CUtil::removeDir( DEF_GEOFENCE_CONFIG_PATH );

            //update gps config
            for( unsigned int i = 0; i < geofenceNum; i++ )
            {
                geofence[i].index = i;
                //LOGD("geofence[i].index %d",geofence[i].index);
                //LOGD("geofence[i].name = %s",geofence[i].name.c_str());
                //LOGD("geofence[i].radius = %f",geofence[i].radius);
                CubicCfgSetV( CUBIC_CFG_loc_fence_name, geofence[i].name, geofence[i].index );
                CubicCfgSetV( CUBIC_CFG_loc_fence_lat,  geofence[i].latitude,  geofence[i].index );
                CubicCfgSetV( CUBIC_CFG_loc_fence_long, geofence[i].longitude, geofence[i].index );
                CubicCfgSetV( CUBIC_CFG_loc_fence_rad,  geofence[i].radius,    geofence[i].index );
            }

            if( ( n_ret = CubicPost( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_SET_GEOFENCE ) ) == 0 )
            {
                n_ret = 200;
                str_ret = "OK";
            }
            else
            {
                n_ret = 501;
                str_ret = "Internal Error";
            }
        }

        return JsonInterface::createResultJson( method, n_ret, str_ret.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetGeoFence, CDataObject, CDATA_OBJECT_SETGEOFENCE, DATA_OBJECT_NAME_SETGEOFENCE )

#endif //_SETGEOFENCE_CC_
