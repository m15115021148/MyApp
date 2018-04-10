/**
 * @file SetSip.cc
 * @author Lele.zhou
 * @version 1.0
 */
#ifndef _TEST_CC_
#define _TEST_CC_ 1

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
#define CUBIC_LOG_TAG "Test"

using namespace std;
using namespace rapidjson;

class Test: public CDataObject
{
private:
    string method;

public:
    virtual ~Test()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        method = "Test";
        return JsonInterface::createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( Test, CDataObject, CDATA_OBJECT_TEST, DATA_OBJECT_NAME_TEST )

#endif //_TEST_CC_