/**
 * @file UpgradeFirmware.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _UPGRADEFIRMWARE_CC_
#define _UPGRADEFIRMWARE_CC_ 1

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
#include <curl/curl.h>
#include <curl/easy.h>


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG                   "UpgradeFirmware"
#define CUBIC_DOWNLOAD_FILE_PATH        "/cache/"
#define CUBIC_JSON_FILE_PATH            "/cache/mamot.json"

#define CUBIC_BUF_SIZE                  256

using namespace std;
using namespace rapidjson;

class UpgradeFirmware: public CDataObject
{
private:
    string method;

public:
    virtual ~UpgradeFirmware()
    {};

    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        int n_ret = 400;
        string str_ret = "ERR";
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        method = "UpgradeFirmware";

        if( access( CUBIC_JSON_FILE_PATH, F_OK ) < 0 )
        {
            n_ret = 404;
            str_ret = "ERR";
            LOGE( "JSON file not exit, not need upgrade!" );
            return JsonInterface::createResultJson( method, n_ret, str_ret.c_str(), json_rsp, size_rsp );
        }

        CubicPost( CUBIC_APP_NAME_OTA_SERVICE, CUBIC_MSG_OTA_UPGRADEFIRMWARE );
        n_ret = 200;
        str_ret = "OK";
        return JsonInterface::createResultJson( method, n_ret, str_ret.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( UpgradeFirmware, CDataObject, CDATA_OBJECT_UPGRADEFIRMWARE, DATA_OBJECT_NAME_UPGRADEFIRMWARE )

#endif //_UPGRADEFIRMWARE_CC_
