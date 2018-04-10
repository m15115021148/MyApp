/**
 * @file GetDeviceStatus.cc
 * @author Lele.Zhou
 * @version 1.0
 */
#ifndef _GET_DEVICE_STATUS_CC_
#define _GET_DEVICE_STATUS_CC_ 1

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
//#include "cubic_log.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "GetDeviceStatus"

using namespace std;
using namespace rapidjson;

#define DEF_METHOD_KEY "method"

class GetDeviceStatus: public CDataObject
{
private:
    int ap_version;
    string ble_mac;
    string server_url;
    int    sip_stat;
    string reg_status;
    string call_status;
    string method;
    string sim_card_status;
    int signal_level;
    string ipv4_status;
    string ipv6_status;
    string mcc;
    string mnc;
    string gpsStatus;
    int gpsSignalLevel;
    float current_latitude;
    float current_longitude;
    float voltage;
    int level;
    string charger_status;
    string groupUuid;
    string groupStatus;

    enum
    {
        STAT_NOT_READY = 1,
        STAT_READY,
        STAT_REGISTERING,
        STAT_IDLE,
        STAT_RINGING,
        STAT_DIALING,
        STAT_CONVERSATION
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
            Value val_general( kObjectType );
            Value val_calls( kObjectType );
            Value val_Network( kObjectType );
            Value val_GPS( kObjectType );
            Value val_Battery( kObjectType );
            Value val_Group( kObjectType );
            val_general.AddMember( "firm", ap_version, allocator );
            val_general.AddMember( "mac", StringRef( ble_mac.c_str(), ble_mac.length() ), allocator );
            val_general.AddMember( "server_url", StringRef( server_url.c_str(), server_url.length() ), allocator );
            val_calls.AddMember( "reg_status", StringRef( reg_status.c_str(), reg_status.length() ), allocator );
            val_calls.AddMember( "call_status", StringRef( call_status.c_str(), call_status.length() ), allocator );
            val_Network.AddMember( "sim_card_status", StringRef( sim_card_status.c_str(), sim_card_status.length() ), allocator );
            val_Network.AddMember( "signal_level", signal_level, allocator );
            val_Network.AddMember( "ipv4_status", StringRef( ipv4_status.c_str(), ipv4_status.length() ), allocator );
            val_Network.AddMember( "ipv6_status", StringRef( ipv6_status.c_str(), ipv6_status.length() ), allocator );
            val_Network.AddMember( "mcc", StringRef( mcc.c_str(), mcc.length() ), allocator );
            val_Network.AddMember( "mnc", StringRef( mnc.c_str(), mnc.length() ), allocator );
            val_GPS.AddMember( "status", StringRef( gpsStatus.c_str(), gpsStatus.length() ), allocator );
            val_GPS.AddMember( "signal_level", gpsSignalLevel, allocator );
            val_GPS.AddMember( "current_latitude", current_latitude, allocator );
            val_GPS.AddMember( "current_longitude", current_longitude, allocator );
            val_Battery.AddMember( "voltage", voltage, allocator );
            val_Battery.AddMember( "level", level, allocator );
            val_Battery.AddMember( "charger_status", StringRef( charger_status.c_str(), charger_status.length() ), allocator );
            val_Group.AddMember( "status", StringRef( groupStatus.c_str(), groupStatus.length() ), allocator );
            val_Group.AddMember( "uuid", StringRef( groupUuid.c_str(), groupUuid.length() ), allocator );
            obj.AddMember( "General", val_general, allocator );
            obj.AddMember( "Calls", val_calls, allocator );
            obj.AddMember( "Network", val_Network, allocator );
            obj.AddMember( "GPS", val_GPS, allocator );
            obj.AddMember( "Battery", val_Battery, allocator );
            obj.AddMember( "Group", val_Group, allocator );
            doc.AddMember( "Status", obj, allocator );
        }

        StringBuffer buffer;
        Writer<StringBuffer> writer( buffer );
        doc.Accept( writer );
        json = buffer.GetString();
        ret = MIN( json.length(), size_rsp - 1 );
        strncpy( json_rsp, json.c_str(), ret );
        return ret;
    };

    void getCallStatus()
    {
        sip_stat = CubicStatGetI( CUBIC_STAT_sip_stat );

        switch( sip_stat )
        {
        case STAT_REGISTERING:
            reg_status = "registered";
            break;

        case STAT_IDLE:
        case STAT_RINGING:
        case STAT_DIALING:
        case STAT_CONVERSATION:
            reg_status = "registering";
            break;

        default:
            reg_status = "not register";
            break;
        };

        switch( sip_stat )
        {
        case STAT_IDLE:
            call_status = "idle";
            break;

        case STAT_RINGING:
            call_status = "ringing";
            break;

        case STAT_DIALING:
            call_status = "dailing";
            break;

        case STAT_CONVERSATION:
            call_status = "connected";
            break;

        default:
            call_status = "idle";
            break;
        };
    };

    void getSimCardStatus()
    {
        int simStatus = CubicStatGet( CUBIC_STAT_net_uim_state, ( int ) - 1 );
        int pinStatus = CubicStatGet( CUBIC_STAT_net_pin_status, ( int ) - 1 );

        switch( simStatus )
        {
		case -1:
			sim_card_status = "UNKNOWN";
			break;

        case 0:
            sim_card_status = "READY";
            break;

        case 1:
        {
            if( pinStatus == 4 )
            {
                sim_card_status = "PUK_LOCKED";
                return;
            }

            if( pinStatus == 5 )
            {
                sim_card_status = "LOCKED_OUT";
                return;
            }

            sim_card_status = "PIN_LOCKED";
        }
        break;

        case 2:
            sim_card_status = "NO_SIM";
            break;

        default:
            LOGE( "Current SIM status has problem." );
            break;
        };
    };

    void getWanStatus()
    {
        int ipv4Status = CubicStatGetI( CUBIC_STAT_net_wanstat_v4 );
        int ipv6Status = CubicStatGetI( CUBIC_STAT_net_wanstat_v6 );

        switch( ipv4Status )
        {
        case 1:
            ipv4_status = "CONNECTING";
            break;

        case 3:
            ipv4_status = "CONNECTED";
            break;

        case 4:
            ipv4_status = "DISCONNECTING";
            break;

        case 6:
            ipv4_status = "DISCONNECTED";
            break;

		default:
			ipv4_status = "DISCONNECTED";
            break;
        };

        switch( ipv6Status )
        {
        case 7:
            ipv6_status = "CONNECTING";
            break;

        case 9:
            ipv6_status = "CONNECTED";
            break;

        case 10:
            ipv6_status = "DISCONNECTING";
            break;

        case 12:
            ipv6_status = "DISCONNECTED";
            break;

		default:
            ipv6_status = "DISCONNECTED";
            break;
        };
    };

    void getNetworkStatus()
    {
        getSimCardStatus();
        getWanStatus();
        signal_level = CubicStatGetI( CUBIC_STAT_net_signal );
        mcc = CubicStatGetStr( CUBIC_STAT_net_mcc );
        mnc = CubicStatGetStr( CUBIC_STAT_net_mnc );
    };
    void getGroupStatus()
    {
        int retryCnt = CubicCfgGetI( CUBIC_CFG_push_group_invite_retry_cnt );
        string tmpUuid = CubicCfgGetStr( CUBIC_CFG_push_group_invite_gid );
        int joinStatus = CubicStatGetI( CUBIC_STAT_core_group_join_stat );
        groupUuid = CubicCfgGetStr( CUBIC_CFG_push_group );

        if( groupUuid.length() != 0 )
        {
            groupStatus = "JOINED";
            return;
        }

        if( ( tmpUuid.length() > 0 ) && ( retryCnt >= 0 ) )
        {
			groupUuid = tmpUuid;
            if( joinStatus == 1 )
            {
                groupStatus = "JOINING";
                return;
            }

            groupStatus = "JOIN_ERROR_RETRY";
            return;
        }

        groupStatus = "JOIN_ERROR_FINAL";
    };
public:
    virtual ~GetDeviceStatus()
    {
    };
    virtual int handler( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        RETNIF_LOGE( json_req == NULL, -1, "json_req  is null" );
        RETNIF_LOGE( json_rsp == NULL, -1, "json_rsp  is null" );
        RETNIF_LOGE( size_rsp <= 0, -1, "the value of size_rsp  is error" );
        string retStatus;
        method = "GetDeviceStatus";
        string apVer = CubicCfgGetStr( CUBIC_CFG_version_num );
        size_t size  = apVer.rfind( '.' );
        string substr = apVer.substr( size + 1 );
        ap_version = CStringTool::fromString<int>( substr );
        ble_mac = CubicCfgGetStr( CUBIC_CFG_bt_addr );
        server_url = CubicCfgGetStr( CUBIC_CFG_push_server );
        getCallStatus();
        getNetworkStatus();

        if( CubicStatGetI( CUBIC_STAT_location_valid ) == 1 )
        {
            gpsStatus = "TRACKING";
        }
        else
        {
            gpsStatus = "SEARCHING";
        }

        gpsSignalLevel = CubicStatGetI( CUBIC_STAT_location_signal );
        current_latitude = CubicStatGetF( CUBIC_STAT_location_lat );
        current_longitude = CubicStatGetF( CUBIC_STAT_location_long );
        voltage = CubicStatGetF( CUBIC_STAT_bat_vol );
        level = CubicStatGetF( CUBIC_STAT_bat_level );
        charger_status = CubicStatGetStr( CUBIC_STAT_charger_status );
        getGroupStatus();
        return createResultJson( method, 200, "OK", json_rsp, size_rsp );
    };
};

REGISTER_PRODUCTOR_NAME( GetDeviceStatus, CDataObject, CDATA_OBJECT_GETDEVICESTATUS, DATA_OBJECT_NAME_GETDEVICESTATUS )

#endif //_GET_DEVICE_STATUS_CC_
