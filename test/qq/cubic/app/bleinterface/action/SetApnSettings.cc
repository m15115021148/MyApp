/**
 * @file SetApnSettings.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _SET_APN_SETTINGS_CC_
#define _SET_APN_SETTINGS_CC_ 1

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
#include <regex.h>

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SetApnSettings"

using namespace std;
using namespace rapidjson;

#define DEF_METHOD_KEY                      "method"
#define DEF_APNPROFILE_KEY                  "ApnProfile"
#define DEF_PROFILENAME_KEY                 "profile_name"
#define DEF_APN_KEY                         "apn"
#define DEF_AUTH_TYPE_KEY                   "auth_type"
#define DEF_AUTHTYPE_KEY                    "authType"
#define DEF_USERNAME_KEY                    "user_name"
#define DEF_PASSWORD_KEY                    "password"
#define DEF_PROTOCOL_KEY                    "protocol"
#define DEF_IPV4PRIMARYDNS_KEY              "ipv4_primary_dns"
#define DEF_IPV4SECONDARYDNS_KEY            "ipv4_secondary_dns"
#define DEF_iPV6PRIMARYDNS_KEY              "ipv6_primary_dns"
#define DEF_IPV6SECONDARYDNS_KEY            "ipv6_secondary_dns"

class SetApnSettings: public CDataObject
{
private:
    typedef struct t_apnProfile
    {
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

    typedef enum SetApnRegularity
    {
        SETAPN_IPV4,
        SETAPN_IPV6,
    } CubicRegularity;

    string method;
    ApnProfile_t apnprofile;
    int JsonOperation( const char *json_req, string &str_ret )
    {
        Document doc;

        if( !JsonInterface::parseJson( json_req, doc ) )
        {
            str_ret = "Abnormal operation";
            return 400;
        }

        if( ( JsonInterface::checkMember( doc, DEF_METHOD_KEY, str_ret ) != 200 ) || ( JsonInterface::checkMember( doc, DEF_APNPROFILE_KEY, str_ret ) != 200 ) )
        {
            return 400;
        }

        Value &val = doc[DEF_APNPROFILE_KEY];

        if( !val.IsObject() )
        {
            str_ret = "Current '";
            str_ret += DEF_APNPROFILE_KEY;
            str_ret += "' not a object.";
            return 400;
        }

        if( ( JsonInterface::checkMember( val, DEF_PROFILENAME_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_APN_KEY, str_ret ) != 200 ) ||
                ( ( JsonInterface::checkMember( val, DEF_AUTH_TYPE_KEY, str_ret ) != 200 ) && ( JsonInterface::checkMember( val, DEF_AUTHTYPE_KEY, str_ret ) != 200 ) ) ||
                ( JsonInterface::checkMember( val, DEF_USERNAME_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_PASSWORD_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_IPV4PRIMARYDNS_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_IPV4SECONDARYDNS_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_iPV6PRIMARYDNS_KEY, str_ret ) != 200 ) ||
                ( JsonInterface::checkMember( val, DEF_IPV6SECONDARYDNS_KEY, str_ret ) != 200 ) )
        {
            return 400;
        }

        if( val[DEF_PROFILENAME_KEY].IsString() &&
                val[DEF_APN_KEY].IsString() &&
                ( ( val.HasMember( DEF_AUTH_TYPE_KEY ) && val[DEF_AUTH_TYPE_KEY].IsString() ) || ( val.HasMember( DEF_AUTHTYPE_KEY ) && val[DEF_AUTHTYPE_KEY].IsString() ) ) &&
                val[DEF_USERNAME_KEY].IsString() &&
                val[DEF_PASSWORD_KEY].IsString() &&
                val[DEF_PROTOCOL_KEY].IsString() &&
                val[DEF_IPV4PRIMARYDNS_KEY].IsString() &&
                val[DEF_IPV4SECONDARYDNS_KEY].IsString() &&
                val[DEF_iPV6PRIMARYDNS_KEY].IsString() &&
                val[DEF_IPV6SECONDARYDNS_KEY].IsString() )
        {
            string auth_type;
            string protocol;
            apnprofile.m_profile_name = val[DEF_PROFILENAME_KEY].GetString();
            apnprofile.m_apn = val[DEF_APN_KEY].GetString();
            apnprofile.m_user_name = val[DEF_USERNAME_KEY].GetString();
            apnprofile.m_password = val[DEF_PASSWORD_KEY].GetString();
            apnprofile.m_ipv4_primary_dns = val[DEF_IPV4PRIMARYDNS_KEY].GetString();
            apnprofile.m_ipv4_secondary_dns = val[DEF_IPV4SECONDARYDNS_KEY].GetString();
            apnprofile.m_ipv6_primary_dns = val[DEF_iPV6PRIMARYDNS_KEY].GetString();
            apnprofile.m_ipv6_secondary_dns = val[DEF_IPV6SECONDARYDNS_KEY].GetString();

            if( val.HasMember( DEF_AUTH_TYPE_KEY ) )
            {
                auth_type = val[DEF_AUTH_TYPE_KEY].GetString();
            }
            else if( val.HasMember( DEF_AUTHTYPE_KEY ) )
            {
                auth_type = val[DEF_AUTHTYPE_KEY].GetString();
            }

            if( auth_type == "PAP" )
            {
                apnprofile.m_auth_type = 0x01;
            }
            else if( auth_type == "CHAP" )
            {
                apnprofile.m_auth_type = 0x02;
            }
            else if( auth_type == "PAP_CHAP" )
            {
                apnprofile.m_auth_type = 0x03;
            }
            else
            {
                apnprofile.m_auth_type = 0;
            }

            protocol = val[DEF_PROTOCOL_KEY].GetString();

            if( auth_type == "IPV4" )
            {
                apnprofile.m_protocol = 0x00;
            }
            else if( auth_type == "IPV6" )
            {
                apnprofile.m_protocol = 0x02;
            }
            else if( auth_type == "IPV4_IPV6" )
            {
                apnprofile.m_protocol = 0x03;
            }
            else
            {
                apnprofile.m_protocol = 0;
            }

            str_ret = "OK";
            return 200;
        }
        else
        {
            str_ret = "Member type has error.";
            return 400;
        }
    };

    int checkRegularity( const char *str, const char *rule )
    {
        int n_ret = 0;
        regex_t reg;
        char buf[256 + 1] = {0};
        n_ret = regcomp( &reg, rule, REG_EXTENDED | REG_NEWLINE );

        if( n_ret != 0 )
        {
            regerror( n_ret, &reg, buf, 256 );
            LOGE( "Check Regularity error message:<%s>", buf );
            regfree( &reg );
            return 0;
        }

        n_ret = regexec( &reg, str, 0, NULL, 0 );

        if( n_ret == REG_NOMATCH )
        {
            regerror( n_ret, &reg, buf, 256 );
            LOGE( "Check Regularity error message:<%s>", buf );
            regfree( &reg );
            return 0;
        }

        regfree( &reg );
        return 1;
    }

    int checkRule( const char *str, int type )
    {
        const char *ipv4 = "^([0-9]{1,2}|1[0-9][0-9]|2[0-4][0-9]|25[0-5])((\\.([0-9]{1,2}|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){3}|(\\.([0-9]{1,2}|1[0-9][0-9]|2[0-4][0-9]|25[0-5])){5})$";
        const char *ipv6 = "^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$";
        const char *domain = "(https?|ftp|file)://[-A-Za-z0-9+&@#/%?=~_|!:,.;]+[-A-Za-z0-9+&@#/%=~_|]";

        switch( type )
        {
        case SETAPN_IPV4:
            if( ( checkRegularity( str, ipv4 ) == 1 ) || ( checkRegularity( str, domain ) == 1 ) )
            {
                return 200;
            }

            break;

        case SETAPN_IPV6:
            if( ( checkRegularity( str, ipv6 ) == 1 ) || ( checkRegularity( str, domain ) == 1 ) )
            {
                return 200;
            }

            break;

        default:
            break;
        }

        return 400;
    }

public:
    virtual ~SetApnSettings()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        int ret = 0;
        string retStatus;
        method = "SetApnSettings";
        LOGD( "json_req:<%s>\tsize_req:<%d>", json_req, size_req );
        ret = JsonOperation( json_req, retStatus );

        if( ret != 200 )
        {
            return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
        }

        if( apnprofile.m_ipv4_primary_dns.length() > 0 )
        {
            if( checkRule( apnprofile.m_ipv4_primary_dns.c_str(), SETAPN_IPV4 ) != 200 )
            {
                retStatus += "ipv4 primary dns format is error.";
                ret = 400;
                return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
            }
        }

        if( apnprofile.m_ipv4_secondary_dns.length() > 0 )
        {
            if( checkRule( apnprofile.m_ipv4_secondary_dns.c_str(), SETAPN_IPV4 ) != 200 )
            {
                retStatus += "ipv4 secondary dns format is error.";
                ret = 400;
                return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
            }
        }

        if( apnprofile.m_ipv6_primary_dns.length() > 0 )
        {
            if( checkRule( apnprofile.m_ipv6_primary_dns.c_str(), SETAPN_IPV6 ) != 200 )
            {
                retStatus += "ipv6 primary dns format is error.";
                ret = 400;
                return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
            }
        }

        if( apnprofile.m_ipv6_secondary_dns.length() > 0 )
        {
            if( checkRule( apnprofile.m_ipv6_secondary_dns.c_str(), SETAPN_IPV6 ) != 200 )
            {
                retStatus += "ipv6 secondary dns format is error.";
                ret = 400;
                return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
            }
        }

        CubicCfgSetV( CUBIC_CFG_net_apn_profile_name,       apnprofile.m_profile_name,  CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_apn_name,           apnprofile.m_apn,           CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_auth_type,          apnprofile.m_auth_type,     CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_user_name,          apnprofile.m_user_name,     CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_password,           apnprofile.m_password,      CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_protocol,           apnprofile.m_protocol,      CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_ipv4_primary_dns,   apnprofile.m_ipv4_primary_dns,      CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_ipv4_secondary_dns, apnprofile.m_ipv4_secondary_dns,    CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_ipv6_primary_dns,   apnprofile.m_ipv6_primary_dns,      CUBIC_APN_CUSTMOIZE );
        CubicCfgSetV( CUBIC_CFG_net_apn_ipv6_secondary_dns, apnprofile.m_ipv6_secondary_dns,    CUBIC_APN_CUSTMOIZE );

        if( ( ret = CubicPost( CUBIC_APP_NAME_NET_SERVICE, CUBIC_MSG_NET_SET_APN ) ) == 0 )
        {
            LOGD( "SETAPN ok ret:<%d>", ret );
            ret = 200;
            retStatus = "OK";
        }
        else
        {
            ret = 400;
            retStatus = "Bad request";
        }

        return JsonInterface::createResultJson( method, ret, retStatus.c_str(), json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( SetApnSettings, CDataObject, CDATA_OBJECT_SETAPNSETTINGS, DATA_OBJECT_NAME_SETAPNSETTINGS )

#endif //_SET_APN_SETTINGS_CC_
