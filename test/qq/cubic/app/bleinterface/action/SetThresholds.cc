/**
 * @file SetThresholds.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _SETTHRESHOLDS_CC_
#define _SETTHRESHOLDS_CC_ 1

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
#define CUBIC_LOG_TAG "SetThresholds"

using namespace std;
using namespace rapidjson;


#define DEF_THRESHOLDS_KEY              "Thresholds"
#define DEF_SHOCK_X_KEY                 "shock_x"
#define DEF_SHOCK_Y_KEY                 "shock_y"
#define DEF_SHOCK_Z_KEY                 "shock_z"
#define DEF_TEMPERATURE_UPPER_KEY       "temperature_upper"
#define DEF_TEMPERATURE_LOWER_KEY       "temperature_lower"
#define DEF_HUMIDITY_UPPER_KEY          "humidity_upper"
#define DEF_HUMIDITY_LOWER_KEY          "humidity_lower"
#define DEF_SHOCK_PRESSURE_UPPER_KEY    "pressure_upper"
#define DEF_SHOCK_PRESSURE_LOWER_KEY    "pressure_lower"

class SetThresholds: public CDataObject
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

    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, DEF_THRESHOLDS_KEY, str_ret ) != 200 )
        {
            return 400;
        }

        Value &val = doc[DEF_THRESHOLDS_KEY];

        if( ( JsonInterface::checkMember( val, DEF_SHOCK_X_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_SHOCK_Y_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_SHOCK_Z_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_TEMPERATURE_UPPER_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_TEMPERATURE_LOWER_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_HUMIDITY_UPPER_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_HUMIDITY_LOWER_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_SHOCK_PRESSURE_UPPER_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_SHOCK_PRESSURE_LOWER_KEY, str_ret ) != 200 ) )
        {
            return 400;
        }

        if( val[DEF_SHOCK_X_KEY].IsNumber() &&
                val[DEF_SHOCK_Y_KEY].IsNumber() &&
                val[DEF_SHOCK_Z_KEY].IsNumber() &&
                val[DEF_TEMPERATURE_UPPER_KEY].IsNumber() &&
                val[DEF_TEMPERATURE_LOWER_KEY].IsNumber() &&
                val[DEF_HUMIDITY_UPPER_KEY].IsNumber() &&
                val[DEF_HUMIDITY_LOWER_KEY].IsNumber() &&
                val[DEF_SHOCK_PRESSURE_UPPER_KEY].IsNumber() &&
                val[DEF_SHOCK_PRESSURE_LOWER_KEY].IsNumber() )
        {
            shock_x = val[DEF_SHOCK_X_KEY].GetFloat();
            shock_y = val[DEF_SHOCK_Y_KEY].GetFloat();
            shock_z = val[DEF_SHOCK_Z_KEY].GetFloat();
            temperature_upper = val[DEF_TEMPERATURE_UPPER_KEY].GetFloat();
            temperature_lower = val[DEF_TEMPERATURE_LOWER_KEY].GetFloat();
            humidity_upper = val[DEF_HUMIDITY_UPPER_KEY].GetFloat();
            humidity_lower = val[DEF_HUMIDITY_LOWER_KEY].GetFloat();
            pressure_upper = val[DEF_SHOCK_PRESSURE_UPPER_KEY].GetFloat();
            pressure_lower = val[DEF_SHOCK_PRESSURE_LOWER_KEY].GetFloat();
            str_ret = "OK";
            return 200;
        }
        else
        {
            str_ret = "Member type has error.";
            return 400;
        }
    };

public:
    virtual ~SetThresholds()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int n_ret = 0;
        string str_ret;

        if( ( n_ret = JsonOperation( json_req, str_ret ) ) == 200 )
        {
            CubicCfgSet( CUBIC_CFG_env_threshold_shock_x, shock_x );
            CubicCfgSet( CUBIC_CFG_env_threshold_shock_y, shock_y );
            CubicCfgSet( CUBIC_CFG_env_threshold_shock_z, shock_z );
            CubicCfgSet( CUBIC_CFG_env_threshold_temperature_high, temperature_upper );
            CubicCfgSet( CUBIC_CFG_env_threshold_temperature_low, temperature_lower );
            CubicCfgSet( CUBIC_CFG_env_threshold_humidity_high, humidity_upper );
            CubicCfgSet( CUBIC_CFG_env_threshold_humidity_low, humidity_lower );
            CubicCfgSet( CUBIC_CFG_env_threshold_pressure_high, pressure_upper );
            CubicCfgSet( CUBIC_CFG_env_threshold_pressure_low, pressure_lower );
        }

        method = "SetThresholds";

        if( ( n_ret = CubicPost( CUBIC_APP_NAME_EVT_SERVICE, CUBIC_MSG_SET_THRESHOLDS ) ) == 0 )
        {
            LOGD( "SetThresholds ok" );
            n_ret = 200;
            str_ret = "OK";
        }
        else
        {
            n_ret = 400;
            str_ret = "Bad request";
        }

        return JsonInterface::createResultJson( method, n_ret, str_ret.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetThresholds, CDataObject, CDATA_OBJECT_GETTHRESHOLDS, DATA_OBJECT_NAME_SETTHRESHOLDS )

#endif //_SETTHRESHOLDS_CC_
