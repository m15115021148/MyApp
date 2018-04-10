/**
 * @file GetApnSettings.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GETAPNSETTINGS_CC_
#define _GETAPNSETTINGS_CC_ 1

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
#define CUBIC_LOG_TAG "GetApnSettings"

using namespace std;
using namespace rapidjson;

class GetApnSettings: public CDataObject
{
private:
    typedef struct t_apnProfile
    {
        int m_index;
        string m_profile_name;
        string m_apn;
        int m_auth_type;
        string m_user_name;
        string m_password;
        int m_protocol;
        string m_ipv4_primary_dns;
        string m_ipv4_secondary_dns;
        string m_ipv6_primary_dns;
        string m_ipv6_secondary_dns;
    } ApnProfile_t;
    string method;
    ApnProfile_t apnprofile;

    int createResultJson( string &method, int result, const char *reason, char *json_rsp, unsigned int size_rsp )
    {
        RETNIF_LOGE( reason == NULL, -1, "reason  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        memset( json_rsp, 0, size_rsp );
        Document doc;
        string json;
        string auth_type;
        string protocol;
        int ret;
        doc.SetObject();
        Document::AllocatorType &allocator = doc.GetAllocator();
        doc.AddMember( "method", StringRef( method.c_str(), method.length() ), allocator );
        doc.AddMember( "result", result, allocator );
        doc.AddMember( "reason", StringRef( reason, strlen( reason ) ), allocator );

        switch( apnprofile.m_auth_type )
        {
        case 0x01:
            auth_type = "PAP";
            break;

        case 0x02:
            auth_type = "CHAP";
            break;

        case 0x03:
            auth_type = "PAP_CHAP";
            break;

        default:
            auth_type = "NONE";
            break;
        }

        switch( apnprofile.m_protocol )
        {
        case 0x00:
            protocol = "IPV4";
            break;

        case 0x02:
            protocol = "IPV6";
            break;

        case 0x03:
        default:
            protocol = "IPV4_IPV6";
            break;
        }

        Value obj( kObjectType );
        obj.AddMember( "index", apnprofile.m_index, allocator );
        obj.AddMember( "profile_name", StringRef( apnprofile.m_profile_name.c_str(), apnprofile.m_profile_name.length() ), allocator );
        obj.AddMember( "apn", StringRef( apnprofile.m_apn.c_str(), apnprofile.m_apn.length() ), allocator );
        obj.AddMember( "auth_type", StringRef( auth_type.c_str(), auth_type.length() ), allocator );
        obj.AddMember( "user_name", StringRef( apnprofile.m_user_name.c_str(), apnprofile.m_user_name.length() ), allocator );
        obj.AddMember( "password", StringRef( apnprofile.m_password.c_str(), apnprofile.m_password.length() ), allocator );
        obj.AddMember( "protocol", StringRef( protocol.c_str(), protocol.length() ), allocator );
        obj.AddMember( "ipv4_primary_dns", StringRef( apnprofile.m_ipv4_primary_dns.c_str(), apnprofile.m_ipv4_primary_dns.length() ), allocator );
        obj.AddMember( "ipv4_secondary_dns", StringRef( apnprofile.m_ipv4_secondary_dns.c_str(), apnprofile.m_ipv4_secondary_dns.length() ), allocator );
        obj.AddMember( "ipv6_primary_dns", StringRef( apnprofile.m_ipv6_primary_dns.c_str(), apnprofile.m_ipv6_primary_dns.length() ), allocator );
        obj.AddMember( "ipv6_secondary_dns", StringRef( apnprofile.m_ipv6_secondary_dns.c_str(), apnprofile.m_ipv6_secondary_dns.length() ), allocator );
        doc.AddMember( "ApnProfile", obj, allocator );
        StringBuffer buffer;
        Writer<StringBuffer> writer( buffer );
        doc.Accept( writer );
        json = buffer.GetString();
        ret = MIN( json.length(), size_rsp - 1 );
        strncpy( json_rsp, json.c_str(), ret );
        return ret;
    };
public:
    virtual ~GetApnSettings()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        apnprofile.m_protocol       = CubicCfgGetVI( CUBIC_CFG_net_apn_protocol,        CUBIC_APN_CUSTMOIZE );
        apnprofile.m_profile_name   = CubicCfgGetVStr( CUBIC_CFG_net_apn_profile_name,  CUBIC_APN_CUSTMOIZE );
        apnprofile.m_apn            = CubicCfgGetVStr( CUBIC_CFG_net_apn_apn_name,      CUBIC_APN_CUSTMOIZE );
        apnprofile.m_auth_type      = CubicCfgGetVI( CUBIC_CFG_net_apn_auth_type,       CUBIC_APN_CUSTMOIZE );
        apnprofile.m_user_name      = CubicCfgGetVStr( CUBIC_CFG_net_apn_user_name,         CUBIC_APN_CUSTMOIZE );
        apnprofile.m_password       = CubicCfgGetVStr( CUBIC_CFG_net_apn_password,      CUBIC_APN_CUSTMOIZE );
        apnprofile.m_ipv4_primary_dns   = CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv4_primary_dns,  CUBIC_APN_CUSTMOIZE );
        apnprofile.m_ipv4_secondary_dns = CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv4_secondary_dns,    CUBIC_APN_CUSTMOIZE );
        apnprofile.m_ipv6_primary_dns   = CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv6_primary_dns,  CUBIC_APN_CUSTMOIZE );
        apnprofile.m_ipv6_secondary_dns = CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv6_secondary_dns,    CUBIC_APN_CUSTMOIZE );
        method = "GetApnSettings";
        return createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GetApnSettings, CDataObject, CDATA_OBJECT_GETAPNSETTINGS, DATA_OBJECT_NAME_GETAPNSETTINGS )

#endif //_GETAPNSETTINGS_CC_
