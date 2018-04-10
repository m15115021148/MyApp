/**
 * @file SetPassword.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _SET_PASSWORD_CC_
#define _SET_PASSWORD_CC_ 1

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
#include <assert.h>
#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SetPassword"

using namespace std;
using namespace rapidjson;

#define SETPASSWORD_PASSWORD "password"
#define SETPASSWORD_NEW_PASSWORD "new_password"

class SetPassword: public CDataObject
{
private:
    string newpswd;

    bool parseJson( char *json_req, Document &doc )
    {
        //      doc.Parse(json_req);
        doc.ParseInsitu<0>( json_req );

        if ( doc.HasParseError() )
        {
            LOGD( "\n1 Error(offset %u): %u\n", ( unsigned )doc.GetErrorOffset(), doc.GetParseError() );
            return false;
        }

        return true;
    };

    int JsonOperation( char *json_req, string &str_ret )
    {
        RETNIF_LOGE( json_req == NULL, false, "json_req is null" );
        Document doc;

        if( !parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, SETPASSWORD_NEW_PASSWORD, str_ret ) != 200 )
        {
            return 400;
        }

        if( !doc[SETPASSWORD_NEW_PASSWORD].IsString() )
        {
            str_ret = "Member type has error.";
            return 400;
        }

        newpswd = doc[SETPASSWORD_NEW_PASSWORD].GetString();

        if( ( newpswd.length() >= 8 ) && ( newpswd.length() <= 16 )  )
        {
            str_ret = "OK";
            return 200;
        }
        else
        {
            str_ret = "the character length of the password need to be between eight and sixteen.";
            return 406;
        }
    };
public:
    virtual ~SetPassword()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int ret = 0;
        string retStatus;
        string method = "SetPassword";
        char buf[size_req + 1] = {0};
        memset( buf, 0, size_req + 1 );
        memcpy( buf, json_req, size_req );
        ret = JsonOperation( buf, retStatus );

        if( ret == 200 )
        {
            CubicCfgSet( CUBIC_CFG_ble_password, newpswd );
        }

        return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetPassword, CDataObject, CDATA_OBJECT_SETPASSWORD, DATA_OBJECT_NAME_SETPASSWORD )

#endif //_SET_PASSWORD_CC_
