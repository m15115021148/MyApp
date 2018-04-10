/**
 * @file BleInterface.cc
 * @author thirchina
 * @version 1.0
 * @brief BLE interface
 * @detail BLE interface
 */

#include "CFramework.cc"
#include "cubic_inc.h"
#include "BleListener.cc"
#include "CDataObject.cc"
#include <iostream>
#include "JsonInterface.cc"

#include "GroupInvitation.cc"
#include "SetPassword.cc"
#include "GetDeviceStatus.cc"
#include "SetServerUrl.cc"
#include "SetSip.cc"
#include "SetInterval.cc"
#include "GetSip.cc"
#include "GetLog.cc"
#include "GetApnSettings.cc"
#include "GetThresholds.cc"
#include "GetGeoFence.cc"
#include "GetIntervals.cc"
#include "GetSimLockStatus.cc"
#include "UpgradeFirmware.cc"
#include "SetApnSettings.cc"
#include "SetThresholds.cc"
#include "SetGeoFence.cc"
#include "Test.cc"


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "BleInterface"


#define GROUPINVITATION_METHOD "method"
#define GROUPINVITATION_SERIAL "serial"
#define GROUPINVITATION_PASSWORD "password"

#define CUBIC_PARAM_MAX 2048

#define CUBIC_BRATE_BLE    B115200
#define CUBIC_START_TIMER_INTERVAL 1000*10
#define CUBIC_LOOP_CHECK_TIMER_INTERVAL  1000*30
#define CUBIC_BLE_RETRY_MAX 3


class BleInterface : public ICubicApp, public IBleMsgHandler, public ITimer
{
private:
    BleListener m_ble_listener;
    int         m_loop_check_timer;
    int         m_loop_check_fail_count;


    void cancelLoopCheckTimer()
    {
        RETIF( m_loop_check_timer < 0 );
        LOGI( "cancelLoopCheckTimer" );
        CubicKillTimer( m_loop_check_timer );
        m_loop_check_timer = -1;
    };

    void setLoopCheckTimer()
    {
        cancelLoopCheckTimer();
        m_loop_check_timer = CubicSetTimerInterval( CUBIC_LOOP_CHECK_TIMER_INTERVAL, this );
        LOGI( "setLoopCheckTimer id=%d", m_loop_check_timer );
    };

    virtual void onTimer( int n_timer_id )
    {
        LOGI( "onTimer: %d\n", n_timer_id );

        if( n_timer_id == m_loop_check_timer )
        {
            //check ble
            CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_CHECK );
        }
    };

    int enbleBT()
    {
        LOGD( "enbleBT" );
        int ret = 0;

        for( int i = 0; i < CUBIC_BLE_RETRY_MAX &&  ( ret = m_ble_listener.start( this ) ) != 0; i++ )
        {
            LOGE( "CAN NOT START BLE LISTENER: %s, ret=%d", CUBIC_DEV_PATH_BLE, ret );
            m_ble_listener.stop();
            sleep( 1 );
        }

        if( ret != 0 )
        {
            LOGE( "enbleBT failed" );
            CubicStatSet( CUBIC_STAT_bt_status, "err" );
            return ret;
        }

        CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_CHECK );
        setLoopCheckTimer();
        CubicStatSet( CUBIC_STAT_bt_status, "on" );
        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_BT_ON );
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_BT);
        return 0;
    };

    int disableBT()
    {
        LOGD( "disableBT" );
        cancelLoopCheckTimer();
        m_ble_listener.stop();
        CubicStatSet( CUBIC_STAT_bt_status, "off" );
        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_BT_OFF );
        CubicWakeupLockClear(CUBIC_WAKELOCK_ID_BT);
        return 0;
    };

public:
    BleInterface()
        : m_ble_listener()
        , m_loop_check_timer( -1 )
        , m_loop_check_fail_count( 0 )
    {};

    ~BleInterface()
    {};

    // interface of ICubicApp
    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        CubicStatSet( CUBIC_STAT_bt_status, "off" );
        return true;
    };

    // interface of ICubicApp
    void onDeInit()
    {
        LOGD( "onDeInit" );
        disableBT();
        return;
    };

    // interface of IBleMsgHandler
    virtual int process( const char *json_req, int size_req, char *json_rsp, int size_rsp )
    {
        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_FLASH_BT_LIGHT );
#if 0
        static const int BUF_SZ = 128;
        char method[BUF_SZ];
        printf( "size_req=%d\n", size_req );
        printf( "============= json_req ===========\n%s\n", json_req );
        const char *p = strstr( json_req, "\"method\":" );

        if( p == NULL )
        {
            printf( "!!!!!!! no method found in req !!!!!!!!\n" );
            return 0;
        }

        memset( method, 0, BUF_SZ );
        p += strlen( "\"method\":" );

        for( uint32_t i = 0; i < BUF_SZ && *p != 0 && *p != ','; i++, p++ )
        {
            method[i] = *p;;
        }

        return snprintf(
                   json_rsp, size_rsp,
                   "{\"method\":%s,\"result\":200,\"reason\":\"OK\"}",
                   method );
#else
        Document doc;
        string str_ret;
        string method = "Unknown";
        string serial_config = CubicCfgGetStr( CUBIC_CFG_serial_num );
        int ret = -1;
        char buf[size_req + 1] = {0};
        memset( buf, 0, size_req + 1 );
        memcpy( buf, json_req, size_req );
        LOGD( "start json_req:<%s>\tsize_req: <%d>", json_req, size_req );
        LOGD( "start buf:<%s>", buf );

        if ( !doc.ParseInsitu<0>( buf ).HasParseError() )
        {
            method = doc[GROUPINVITATION_METHOD].GetString();

            if( method == "Test" )
            {
                CDataObject *data_obj = CFactory<CDataObject>::getInstance().createByName( method.c_str() );

                if( data_obj )
                {
                    ret = data_obj->handler( json_req, size_req, json_rsp, size_rsp );
                }
                else
                {
                    ret = JsonInterface::createResultJson( method, 404, "Not Found object", json_rsp, size_rsp );
                }

                return ret;
            }

            if( JsonInterface::checkMember( doc, GROUPINVITATION_SERIAL, str_ret ) != 200 )
            {
                ret = JsonInterface::createResultJson( method, 400, str_ret.c_str(), json_rsp, size_rsp );
                LOGD( "serial not match return json_rsp:<%s>\tsize_rsp:<%d>", json_rsp, size_rsp );
                return ret;
            }

            string serial = doc[GROUPINVITATION_SERIAL].GetString();

            if( serial == serial_config )
            {
                string pswd_local = CubicCfgGetStr( CUBIC_CFG_ble_password );

                if( pswd_local.length() > 0  )
                {
                    if( JsonInterface::checkMember( doc, GROUPINVITATION_PASSWORD, str_ret ) == 200 )
                    {
                        string pswd = doc[GROUPINVITATION_PASSWORD].GetString();

                        if(  pswd == pswd_local )
                        {
                            CDataObject *data_obj = CFactory<CDataObject>::getInstance().createByName( method.c_str() );

                            if( data_obj )
                            {
                                ret = data_obj->handler( json_req, size_req, json_rsp, size_rsp );
                            }
                            else
                            {
                                ret = JsonInterface::createResultJson( method, 404, "Not Found object", json_rsp, size_rsp );
                            }
                        }
                        else
                        {
                            ret = JsonInterface::createResultJson( method, 401, "Unauthorized", json_rsp, size_rsp );
                        }
                    }
                    else
                    {
                        ret = JsonInterface::createResultJson( method, 400, str_ret.c_str(), json_rsp, size_rsp );
                    }
                }
                else
                {
                    if( method == "SetPassword" )
                    {
                        CDataObject *data_obj = CFactory<CDataObject>::getInstance().createByName( method.c_str() );

                        if( data_obj )
                        {
                            ret = data_obj->handler( json_req, size_req, json_rsp, size_rsp );
                        }
                        else
                        {
                            ret = JsonInterface::createResultJson( method, 404, "Not Found object", json_rsp, size_rsp );
                        }
                    }
                    else
                    {
                        ret = JsonInterface::createResultJson( method, 400, "Current settings are not allowed ,please set password", json_rsp, size_rsp );
                    }
                }
            }
            else
            {
                string str;
                str = "Serial number not match , I'm '";
                str += serial_config;
                str += "' ,but you asked for '";
                str += serial;
                str_ret += "'.";
                LOGE( "403 error %s", str.c_str() );
                ret = JsonInterface::createResultJson( method, 403, "Foridden", json_rsp, size_rsp );
            }
        }
        else
        {
            LOGD( "\nError(offset %u): %u\n", ( unsigned )doc.GetErrorOffset(), doc.GetParseError() );
            ret = JsonInterface::createResultJson( method, 400, "Not a json formatted substring", json_rsp, size_rsp );
        }

        LOGD( "return json_rsp:<%s>\tsize_rsp:<%d>\tret:<%d>", json_rsp, size_rsp, ret );
        return ret;
#endif
    };

    // interface of ICubicApp
    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGE( "n_msg_id:<%d>", n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_BLE_ENABLE:
            LOGD( "onMessage: CUBIC_MSG_BLE_ENABLE" );
            RETNIF_LOGE( enbleBT(),
                         -1,
                         "CAN NOT ENABLE BLE LISTENER: %s", CUBIC_DEV_PATH_BLE );
            break;

        case CUBIC_MSG_BLE_DISABLE:
            LOGD( "onMessage: CUBIC_MSG_BLE_DISABLE" );
            disableBT();
            break;

        case CUBIC_MSG_BLE_TEST:
        {
            LOGD( "onMessage: CUBIC_MSG_BLE_TEST" );
            cubic_msg_test_ble *str_src = ( cubic_msg_test_ble * )p_data;
            char buf[CUBIC_PARAM_MAX + 4] = {0};
            process( str_src->param, strlen( str_src->param ), buf, CUBIC_PARAM_MAX );
        }
        break;

        case CUBIC_MSG_BLE_CHECK:
            LOGD( "onMessage: CUBIC_MSG_BLE_CHECK" );
            m_ble_listener.doCheck();
            sleep( 1 );

            if( m_ble_listener.getCheckStat() )
            {
                m_loop_check_fail_count = 0;
                break;
            }

            m_loop_check_fail_count++;

            if( m_loop_check_fail_count > CUBIC_BLE_RETRY_MAX )
            {
                LOGE( "ble not work, check failed and out of retry limit" );
                disableBT();
                CubicStatSet( CUBIC_STAT_bt_status, "err" );
                break;
            }

            LOGD( "ble not work, check failed retry: %d", m_loop_check_fail_count );
            CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_DISABLE );
            CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_ENABLE );
            break;

        default:
            break;
        };

        return 0;
    };
};

IMPLEMENT_CUBIC_APP( BleInterface )

