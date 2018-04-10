/**
 * @file GetGeoFence.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GETGEOFENCE_CC_
#define _GETGEOFENCE_CC_ 1

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
#define CUBIC_LOG_TAG "GetGeoFence"

using namespace std;
using namespace rapidjson;

#define DEF_GEOFENCE_NUM 16

class GetGeoFence: public CDataObject
{
private:
    string method;
    typedef struct t_geoFence
    {
        int index;
        string name;
        float latitude;
        float longitude;
        int radius;
    } GeoFence_t;
    GeoFence_t geofence[DEF_GEOFENCE_NUM];
    unsigned int geofenceNum;

    int createResultJson( string &method, int result, const char *reason, char *json_rsp, unsigned int size_rsp )
    {
        RETNIF_LOGE( reason == NULL, -1, "reason  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        string json;
        Document doc;
        int ret;
        memset( json_rsp, 0, size_rsp );
        doc.SetObject();
        Document::AllocatorType &allocator = doc.GetAllocator();
        doc.AddMember( "method", StringRef( method.c_str(), method.length() ), allocator );
        doc.AddMember( "result", result, allocator );
        doc.AddMember( "reason", StringRef( reason, strlen( reason ) ), allocator );

        if( result == 200 )
        {
            Value array( kArrayType );

            for( unsigned int i = 0; i < geofenceNum; i++ )
            {
                Value obj( kObjectType );
                obj.SetObject();
                obj.AddMember( "index", geofence[i].index, allocator );
                obj.AddMember( "name", StringRef( geofence[i].name.c_str(), geofence[i].name.length() ), allocator );
                obj.AddMember( "latitude", geofence[i].latitude, allocator );
                obj.AddMember( "longitude", geofence[i].longitude, allocator );
                obj.AddMember( "radius", geofence[i].radius, allocator );
                array.PushBack( obj, allocator );
            }

            doc.AddMember( "geo_fences", array, allocator );
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer( buffer );
        doc.Accept( writer );
        json = buffer.GetString();
        ret = MIN( json.length(), size_rsp - 1 );
        strncpy( json_rsp, json.c_str(), ret );
        return ret;
    };
public:
    virtual ~GetGeoFence()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        vector<string> geofence_configs = CubicCfgEnum( CUBIC_CFG_loc_fence );
        geofenceNum = geofence_configs.size();
        geofenceNum = MIN( DEF_GEOFENCE_NUM, geofenceNum );

        for( unsigned int i = 0; i < geofenceNum; i++ )
        {
            geofence[i].index = i;
            geofence[i].name        = CubicCfgGetVStr( CUBIC_CFG_loc_fence_name, geofence[i].index );
            geofence[i].latitude    = CubicCfgGetVF( CUBIC_CFG_loc_fence_lat,    geofence[i].index );
            geofence[i].longitude   = CubicCfgGetVF( CUBIC_CFG_loc_fence_long,   geofence[i].index );
            geofence[i].radius      = CubicCfgGetVI( CUBIC_CFG_loc_fence_rad,    geofence[i].index );
        }

        method = "GetGeoFence";
        return createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GetGeoFence, CDataObject, CDATA_OBJECT_GETGEOFENCE, DATA_OBJECT_NAME_GETGEOFENCE )

#endif //_GETGEOFENCE_CC_
