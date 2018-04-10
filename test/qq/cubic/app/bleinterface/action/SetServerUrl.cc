/**
 * @file SetServerUrl.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _SET_SERVER_URL_CC_
#define _SET_SERVER_URL_CC_ 1

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
#define CUBIC_LOG_TAG "SetServerUrl"

using namespace std;
using namespace rapidjson;

#define DEF_SERVERURL_KEY   "server_url"

class SetServerUrl: public CDataObject
{
private:
    string method;
    string serverurl;
    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, DEF_SERVERURL_KEY, str_ret ) != 200 )
        {
            return 400;
        }

        if( !doc[DEF_SERVERURL_KEY].IsString() )
        {
            str_ret = "Member type has error.";
            return 400;
        }

        method = doc[DEF_METHOD_KEY].GetString();
        serverurl = doc[DEF_SERVERURL_KEY].GetString();
        str_ret = "OK";
        return 200;
    };
public:
    virtual ~SetServerUrl()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        method = "SetServerUrl";
        int ret = 0;
        string retStatus;
        ret = JsonOperation( json_req, retStatus );

        if( ret == 200 )
        {
            CubicCfgSet( CUBIC_CFG_push_server, serverurl );
        }

        return JsonInterface::createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetServerUrl, CDataObject, CDATA_OBJECT_SETSERVERURL, DATA_OBJECT_NAME_SETSERVERURL )

#endif //_SET_SERVER_URL_CC_
