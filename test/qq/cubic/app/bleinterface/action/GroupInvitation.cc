/**
 * @file GroupInvitation.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GROUP_INVITATION_CC_
#define _GROUP_INVITATION_CC_ 1

#include "cubic_inc.h"
#include "CFactory.cc"
#include "CProductor.cc"
#include "CDataObject.cc"
#include "CDataObjectTypes.h"
#include "JsonInterface.cc"
#include "document.h"
#include "allocators.h"
#include "stringbuffer.h"
#include "writer.h"
#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "GroupInvitation"

using namespace std;
using namespace rapidjson;

#define GROUPINVITATION_KEY "GroupInvitation"
#define GROUPINVITATION_TOKEN_KEY "token"
#define GROUPINVITATION_GROUP_UUID_KEY "group_uuid"



class GroupInvitation: public CDataObject
{
private:
    string group_token;
    string group_uuid;
    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( JsonInterface::checkMember( doc, GROUPINVITATION_KEY, str_ret ) != 200 )
        {
            return 400;
        }

        Value &val = doc[GROUPINVITATION_KEY];

        if( !val.IsObject() )
        {
            str_ret = "Current '";
            str_ret += GROUPINVITATION_KEY;
            str_ret += "' is not object";
            return 400;
        }

        if( ( JsonInterface::checkMember( val, GROUPINVITATION_TOKEN_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, GROUPINVITATION_GROUP_UUID_KEY, str_ret ) != 200 ) )
        {
            return 400;
        }

        if( !val[GROUPINVITATION_TOKEN_KEY].IsString() || !val[GROUPINVITATION_GROUP_UUID_KEY].IsString() )
        {
            str_ret = "Member type has error";
            return 400;
        }

        group_token = val[GROUPINVITATION_TOKEN_KEY].GetString();
        group_uuid = val[GROUPINVITATION_GROUP_UUID_KEY].GetString();
        str_ret = "OK";
        return 200;
    };

public:
    virtual ~GroupInvitation()
    {
    };

    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int ret = 0;
        string retVal;
        string method = "GroupInvitation";
        cubic_msg_join_group group;
        string groupId = CubicCfgGetStr( CUBIC_CFG_push_group );
        int retry = 0;
        static const int RETRY_COUNT = 10;
        memset( &group, 0, sizeof( cubic_msg_join_group ) );
        ret = JsonOperation( json_req, retVal );

        if( groupId.length() > 0 )
        {
            ret = 488;
            retVal = "Not Acceptable Here.";
            return JsonInterface::createResultJson( method, ret, retVal.c_str(), json_rsp, size_rsp );
        }

        if( ret != 200 )
        {
            return JsonInterface::createResultJson( method, ret, retVal.c_str(), json_rsp, size_rsp );
        }

        strncpy( group.token, group_token.c_str(), CUBIC_UUID_LEN_MAX );
        strncpy( group.group_uuid, group_uuid.c_str(), CUBIC_UUID_LEN_MAX );

        do
        {
            CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_JOIN_GROUP, group );
            usleep( 10000 ); //sleep 10 ms

            if( ( CubicCfgGetStr( CUBIC_CFG_push_group_invite_gid ).length() > 0 ) &&
                    ( CubicCfgGetStr( CUBIC_CFG_push_group_invite_token ).length() > 0 ) )
            {
                break;
            }

            retry++;
        }
        while( retry < RETRY_COUNT );

        LOGD( "join group retry count:%d ", retry );

        if( retry < RETRY_COUNT )
        {
            LOGD( "GroupInvitation ok" );
            ret = 200;
            retVal = "OK";
        }
        else
        {
            ret = 400;
            retVal = "Bad request";
            LOGE( "%s Internal error", method.c_str() );
        }

        //send msg
        return JsonInterface::createResultJson( method, ret, retVal.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GroupInvitation, CDataObject, CDATA_OBJECT_GROUPINVITATION, DATA_OBJECT_NAME_GROUPINVITATION )

#endif //_GROUP_INVITATION_CC_
