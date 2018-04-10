/**
 * @file UnlockSimWithPin.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _UNLOCKSIMWITHPIN_CC_
#define _UNLOCKSIMWITHPIN_CC_ 1

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
#define CUBIC_LOG_TAG "UnlockSimWithPin"

using namespace std;
using namespace rapidjson;

#define DEF_PIN_KEY "pin"
class UnlockSimWithPin: public CDataObject
{
private:
    string method;
    string pin;
    string lock;
    int remaining_retries;
    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( JsonInterface::parseJson( json_req, doc ) )
        {
            if( JsonInterface::checkMember( doc, DEF_PIN_KEY, str_ret ) == 200 )
            {
                Value &val = doc[DEF_PIN_KEY];
                pin = val.GetString();
                str_ret = "OK";
                return 200;
            }
            else
            {
                return 400;
            }
        }

        str_ret = "Abnormal operation";
        return 400;
    };

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
            obj.AddMember( "lock", StringRef( lock.c_str(), lock.length() ), allocator );
            obj.AddMember( "remaining_retries", remaining_retries, allocator );
            doc.AddMember( "SimLockStatus", obj, allocator );
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
    virtual ~UnlockSimWithPin()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        cubic_msg_net_sim_unlock unlockPin;
        int n_len = sizeof( cubic_msg_net_enable_pin );
        int n_ret = 400;
        string str_ret = "Bad Request";
        n_ret = JsonOperation( json_req, str_ret );

        if ( n_ret == 200 )
        {
            memset( &unlockPin, 0, n_len );
            n_len = ( CUBIC_PIN_PUK_LEN_MAX > pin.length() ) ? pin.length() : CUBIC_PIN_PUK_LEN_MAX - 1;
            memcpy( unlockPin.pin_puk_code, pin.c_str(), n_len );

            if( CubicPostReq( CUBIC_APP_NAME_NET_SERVICE, CUBIC_MSG_NET_SIM_UNLOCK, unlockPin ) == 0 )
            {
                str_ret = "OK";
                n_ret = 200;
                lock = CubicStatGetStr( CUBIC_STAT_net_pin_status );
                remaining_retries = CubicStatGetI( CUBIC_STAT_net_verify_retries_left );
            }
            else
            {
                n_ret = 400;
                str_ret = "event 'CUBIC_MSG_NET_SIM_UNLOCK' send error";
            }
        }

        method = "UnlockSimWithPin";
        return createResultJson( method, n_ret, str_ret.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( UnlockSimWithPin, CDataObject, CDATA_OBJECT_UNLOCKSIMWITHPIN, DATA_OBJECT_NAME_UNLOCKSIMWITHPIN )

#endif //_UNLOCKSIMWITHPIN_CC_
