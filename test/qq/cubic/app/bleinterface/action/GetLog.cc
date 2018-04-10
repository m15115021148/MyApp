/**
 * @file GetLog.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GETLOG_CC_
#define _GETLOG_CC_ 1

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
#include "cubic_func.h"
#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "GetLog"

using namespace std;
using namespace rapidjson;

#define DEF_LOG_KEY "Log"
#define DEF_TYPE_KEY "type"
#define DEF_LINES_KEY "lines"
#define DEF_SYSCAL_GETLOG "cubic-read-log %d %s"
#define DEF_CMMD_LENGTH 128
#define DEF_CMMD_RESULT_MAX 4000
#define DEF_SYSCAL_GETLOG_UPLOAD "cubic-read-log > %s"

class GetLog: public CDataObject
{
private:
    string method;
    string type;
    int    lines;
    string data;

    int JsonOperation( const char *json_req, string &str_ret )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        Document doc;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, DEF_LOG_KEY, str_ret ) != 200 )
        {
            return 400;
        }

        Value &val = doc[DEF_LOG_KEY];

        if( !val[DEF_TYPE_KEY].IsString() || !val[DEF_LINES_KEY].IsNumber() )
        {
            str_ret = "Member type has error.";
            return 400;
        }

        type = val[DEF_TYPE_KEY].GetString();
        lines = val[DEF_LINES_KEY].GetInt();
        str_ret = "OK";
        return 200;
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
            obj.AddMember( "type", StringRef( type.c_str(), type.length() ), allocator );
            obj.AddMember( "data", StringRef( data.c_str(), data.length() ), allocator );
            doc.AddMember( "Log", obj, allocator );
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer( buffer );
        doc.Accept( writer );
        json = buffer.GetString();
        ret = MIN( json.length(), size_rsp - 1 );
        strncpy( json_rsp, json.c_str(), ret );
        writeLogFile();
        return ret;
    };

    int writeLogFile()
    {
        char cmdGetLog[DEF_CMMD_LENGTH + 4] = {0};
        char result[DEF_CMMD_RESULT_MAX + 4] = {0};
        snprintf( cmdGetLog, DEF_CMMD_LENGTH, DEF_SYSCAL_GETLOG_UPLOAD , CUBIC_BLE_UPLOAD_LOG_PATH );
        CUtil::syscall( cmdGetLog, result, DEF_CMMD_RESULT_MAX );
        CubicPost( CUBIC_APP_NAME_LOG_SERVICE, CUBIC_MSG_BLE_UPLOAD_LOG );
        return 0;
    }

public:
    virtual ~GetLog()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int n_ret;
        string str_ret;
        char cmdGetLog[DEF_CMMD_LENGTH + 4] = {0};
        char result[DEF_CMMD_RESULT_MAX + 4] = {0};
        n_ret = JsonOperation( json_req, str_ret );

        if(  n_ret == 200 )
        {
            //          data = "Wait for the implementation.";
            snprintf( cmdGetLog, DEF_CMMD_LENGTH, DEF_SYSCAL_GETLOG, lines, type.c_str() );
            CUtil::syscall( cmdGetLog, result, DEF_CMMD_RESULT_MAX );
            data = result;
        }

        method = "GetLog";
        return createResultJson( method, n_ret, str_ret.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GetLog, CDataObject, CDATA_OBJECT_GETLOG, DATA_OBJECT_NAME_GETLOG )

#endif //_GETLOG_CC_
