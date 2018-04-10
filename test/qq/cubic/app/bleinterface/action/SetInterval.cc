/**
 * @file SetInterval.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _SETINTERVAL_CC_
#define _SETINTERVAL_CC_ 1

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
#include "CDataObject.cc"
#include "CDataObjectTypes.h"
#include <assert.h>
#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SetIntervals"

using namespace std;
using namespace rapidjson;

#define DEF_INTERVALS_KEY "Intervals"
#define DEF_ACTIVE_KEY "active"
#define DEF_PASSIVE_KEY "passive"

class SetIntervals: public CDataObject
{
private:
    string method;
    float active;
    float passive;
    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, DEF_INTERVALS_KEY, str_ret ) != 200 )
        {
            return 400;
        }

        Value &obj = doc[DEF_INTERVALS_KEY];

        if( ( JsonInterface::checkMember( obj, DEF_ACTIVE_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( obj, DEF_PASSIVE_KEY, str_ret ) != 200 ) )
        {
            return 400;
        }

        if( !obj[DEF_ACTIVE_KEY].IsNumber() || !obj[DEF_PASSIVE_KEY].IsNumber() )
        {
            str_ret = "Member type has error.";
            return 400;
        }

        active = obj[DEF_ACTIVE_KEY].GetFloat();
        passive = obj[DEF_PASSIVE_KEY].GetFloat();

        if( ( ( active < 1 ) || ( active > 300 ) ) || ( ( passive < 300 ) || ( passive > 3600 ) ) )
        {
            str_ret = "The set value is not correct.";
            return 400;
        }

        str_ret = "OK";
        return 200;
    };

public:
    virtual ~SetIntervals()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int ret = 0;
        string retStatus;
        ret = JsonOperation( json_req, retStatus );

        if( ret == 200 )
        {
            CubicCfgSet( CUBIC_CFG_report_interval, active * 1000 );
            CubicCfgSet( CUBIC_CFG_report_p_interval, passive * 1000 );
        }

        method = "SetInterval";
        return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetIntervals, CDataObject, CDATA_OBJECT_SETINTERVAL, DATA_OBJECT_NAME_SETINTERVAL )

#endif //_SETINTERVAL_CC_
