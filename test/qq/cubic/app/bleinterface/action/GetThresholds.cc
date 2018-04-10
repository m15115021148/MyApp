/**
 * @file GetThresholds.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GETTHRESHOLDS_CC_
#define _GETTHRESHOLDS_CC_ 1

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
#define CUBIC_LOG_TAG "GetThresholds"

using namespace std;
using namespace rapidjson;

class GetThresholds: public CDataObject
{
private:
    string method;
    float shock_x;
    float shock_y;
    float shock_z;
    float temperature_upper;
    float temperature_lower;
    float humidity_upper;
    float humidity_lower;
    float pressure_upper;
    float pressure_lower;

    int createResultJson( string &method, int result, const char *reason, char *json_rsp, unsigned int size_rsp )
    {
        Document doc;
        string json;
        int ret;
        RETNIF_LOGE( reason == NULL, -1, "reason  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        memset( json_rsp, 0, size_rsp );
        doc.SetObject();
        Document::AllocatorType &allocator = doc.GetAllocator();
        doc.AddMember( "method", StringRef( method.c_str(), method.length() ), allocator );
        doc.AddMember( "result", result, allocator );
        doc.AddMember( "reason", StringRef( reason, strlen( reason ) ), allocator );

        if( result == 200 )
        {
            Value obj( kObjectType );
            obj.AddMember( "shock_x", shock_x, allocator );
            obj.AddMember( "shock_y", shock_y, allocator );
            obj.AddMember( "shock_z", shock_z, allocator );
            obj.AddMember( "temperature_upper", temperature_upper, allocator );
            obj.AddMember( "temperature_lower", temperature_lower, allocator );
            obj.AddMember( "humidity_upper", humidity_upper, allocator );
            obj.AddMember( "humidity_lower", humidity_lower, allocator );
            obj.AddMember( "pressure_upper", pressure_upper, allocator );
            obj.AddMember( "pressure_lower", pressure_lower, allocator );
            doc.AddMember( "Thresholds", obj, allocator );
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
    virtual ~GetThresholds()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        shock_x = CubicCfgGetF( CUBIC_CFG_env_threshold_shock_x );
        shock_y = CubicCfgGetF( CUBIC_CFG_env_threshold_shock_y );
        shock_z = CubicCfgGetF( CUBIC_CFG_env_threshold_shock_z );
        temperature_upper = CubicCfgGetF( CUBIC_CFG_env_threshold_temperature_high );
        temperature_lower = CubicCfgGetF( CUBIC_CFG_env_threshold_temperature_low );
        humidity_upper = CubicCfgGetF( CUBIC_CFG_env_threshold_humidity_high );
        humidity_lower = CubicCfgGetF( CUBIC_CFG_env_threshold_humidity_low );
        pressure_upper = CubicCfgGetF( CUBIC_CFG_env_threshold_pressure_high );
        pressure_lower = CubicCfgGetF( CUBIC_CFG_env_threshold_pressure_low );
        method = "GetThresholds";
        return createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GetThresholds, CDataObject, CDATA_OBJECT_GETTHRESHOLDS, DATA_OBJECT_NAME_GETTHRESHOLDS )

#endif //_GETTHRESHOLDS_CC_
