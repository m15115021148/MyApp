/**
 * @file SetSip.cc
 * @author Lele.zhou
 * @version 1.0
 */
#ifndef _SET_SIP_CC_
#define _SET_SIP_CC_ 1

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
#define CUBIC_LOG_TAG "SetSip"

using namespace std;
using namespace rapidjson;

#define DEF_SIP_KEY             "Sip"
#define DEF_SPORT_KEY           "sport"
#define DEF_MPORT_START_KEY     "mport_start"
#define DEF_MPORT_END_KEY       "mport_end"


class SetSip: public CDataObject
{
private:
    string method;
    int sport;
    int mport_start;
    int mport_end;

    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( JsonInterface::parseJson( json_req, doc ) )
        {
            if( ( JsonInterface::checkMember( doc, DEF_METHOD_KEY, str_ret ) == 200 ) &&
                    ( JsonInterface::checkMember( doc, DEF_SIP_KEY, str_ret ) == 200 ) )
            {
                Value &val = doc[DEF_SIP_KEY];
                method = doc[DEF_METHOD_KEY].GetString();

                if( ( JsonInterface::checkMember( val, DEF_SPORT_KEY, str_ret ) == 200 ) &&
                        ( JsonInterface::checkMember( val, DEF_MPORT_START_KEY, str_ret ) == 200 ) &&
                        ( JsonInterface::checkMember( val, DEF_MPORT_END_KEY, str_ret ) == 200 ) )
                {
                    sport = val[DEF_SPORT_KEY].GetInt();
                    mport_start = val[DEF_MPORT_START_KEY].GetInt();
                    mport_end = val[DEF_MPORT_END_KEY].GetInt();
                    str_ret = "OK";
                    return 200;
                }
                else
                {
                    return 400;
                }
            }
            else
            {
                return 400;
            }
        }

        str_ret = "Abnormal operation";
        return 400;
    };

public:
    virtual ~SetSip()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int ret = 0;
        string retStatus;
        method = "SetSip";
        ret = JsonOperation( json_req, retStatus );

        if( ret == 200 )
        {
            CubicCfgSet( CUBIC_CFG_sip_sport,       sport );
            CubicCfgSet( CUBIC_CFG_sip_mport_start, mport_start );
            CubicCfgSet( CUBIC_CFG_sip_mport_end,   mport_end );
            CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_SET_CFG );
        }

        return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetSip, CDataObject, CDATA_OBJECT_SETSIP, DATA_OBJECT_NAME_SETSIP )

#endif //_SET_SIP_CC_
