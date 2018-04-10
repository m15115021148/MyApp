/**
 * @file GetSip.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GETSIP_CC_
#define _GETSIP_CC_ 1

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
#define CUBIC_LOG_TAG "GetSip"

using namespace std;
using namespace rapidjson;

class GetSip: public CDataObject
{
private:
    string method;
    int sport;
    int mport_start;
    int mport_end;

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
            obj.AddMember( "sport", sport, allocator );
            obj.AddMember( "mport_start", mport_start, allocator );
            obj.AddMember( "mport_end", mport_end, allocator );
            doc.AddMember( "Sip", obj, allocator );
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
    virtual ~GetSip()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        sport = CubicCfgGetI( CUBIC_CFG_sip_sport );
        mport_start = CubicCfgGetI( CUBIC_CFG_sip_mport_start );
        mport_end = CubicCfgGetI( CUBIC_CFG_sip_mport_end );
        method = "GetSip";
        return createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GetSip, CDataObject, CDATA_OBJECT_GETSIP, DATA_OBJECT_NAME_GETSIP )

#endif //_GETSIP_CC_
