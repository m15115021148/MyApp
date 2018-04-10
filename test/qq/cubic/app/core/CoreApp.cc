/**
 * @file CoreApp.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief main control of mamotel
 * @detail main control of mamotel
 */

#include "cubic_inc.h"

#include "CFramework.cc"
#include "CThread.cc"
#include "CAsyncRun.cc"
#include "CRemoteReport.cc"
#include "CStatMachine.cc"
#include "CSafeQueue.cc"


#include <iostream>
#include <list>
#include <algorithm>
#include <syscall.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <linux/reboot.h>
#include <linux/input.h>



using namespace std;

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "CoreApp"


#define CUBIC_RING_PATH "/data/res/sound/ring.mp3"
#define CUBIC_BEEP_PATH "/data/res/sound/beep.mp3"
#define CUBIC_RECORD_PATH CUBIC_VOICE_MSG_CACHE"/record_%d.ogg"


#define SHUTDOWN_COMMAND "shutdown -hP now"
#define REBOOT_COMMAND "shutdown -r now"


#define CUBIC_ACTIVATE_TIMEOUT      10000 //ms
#define CUBIC_VM_RECORD_TIMEOUT     60000 //ms
#define CUBIC_RING_TIMEOUT          60000 //ms
#define CUBIC_DIAL_TIMEOUT          100000 //ms
#define CUBIC_HEARTBEAT_TIMEOUT     43200000 // 12*3600*1000 ms
#define CUBIC_JOIN_GROUP_TIMEOUT    60000 // ms
#define CUBIC_JOIN_GROUP_MAX_RETRY  1440  // 24h
#define CUBIC_EVENT_REPORT_RETRY    5
#define CUBIC_LM_TIMEOUT            90000 // ms, NOTE: should greater than CUBIC_VM_RECORD_TIMEOUT

#define CUBIC_BT_FLASH_TIMEOUT      3000  // ms
#define CUBIC_BT_ACTIVE_TIMEOUT     180000 // ms
#define CUBIC_BT_START_TIMEOUT      6000 // ms

#define CUBIC_OTA_ERROR_FLASH_TIMEOUT      2000  // ms

#define CUBIC_DEF_REPORT_INTERVAL    60






class CoreApp : public ICubicApp, public ITimer, public IStatListener, protected CStatMachine, public IAbsTimer
{
private:
    int                         m_record_vm_timer;
    int                         m_call_ring_timer;
    int                         m_call_dial_timer;
    int                         m_activate_timer;
    int                         m_report_tracking_timer;  // abs timer
    int                         m_report_heartbeat_timer; // abs timer
    int                         m_join_group_timer;
    int                         m_bt_start_timer;
    int                         m_bt_flash_timer;
    int                         m_bt_active_timer;
    int                         m_ota_err_flash_timer;
    int                         m_active_timer;
    int                         m_lm_timer;
    bool                        m_is_in_active_mode;
    int                         m_curr_play_lm_idx;


    typedef cubic_msg_gps_fence_evt  report_gps_fence_t;
    typedef cubic_msg_update         report_new_firmware_t;
    typedef cubic_msg_last_unread_vm evt_vm_play;
    typedef cubic_msg_lm             evt_lm_play;
    typedef cubic_msg_play_done      evt_vm_play_end;
    typedef cubic_msg_record_done    evt_vm_record_end;


    void shutdownSystem( bool reboot = false )
    {
        int ret;
        LOGE( "shutdownSystem reboot=%s", ( reboot ? "REBOOT" : "SHUTDOWN" ) );
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_POWER_ACT);
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_BOOTING_ON );
        ret = CRemoteReport::reportPowerOff();
        LOGE( "shutdownSystem reportPowerOff ret=%d", ret );

        if( reboot )
        {
            sync();
            ret = system( REBOOT_COMMAND );
            //ret = syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL);
        }
        else
        {
            sync();
            ret = system( SHUTDOWN_COMMAND );
            //ret = syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, NULL);
        }

        stopPlay();
        stopRecord();
        RETIF_LOGE( ret < 0, "shutdownSystem syscall failed, %d, %s", errno, strerror( errno ) );
        LOGE( "shutdownSystem accept" );
        // nerver clear for shuting down
        // CubicWakeupLockClear(CUBIC_WAKELOCK_ID_POWER_ACT);
    };

    void updateStatus()
    {
        LOGD( "updateStatus" );
        // battery status
        string charg_stat = CubicStatGetStr( CUBIC_STAT_charger_status );

        if( charg_stat == "full" )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_CHARGER_FULL );
        }
        else if( charg_stat == "on" )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_CHARGER_IN );
        }
        else if( charg_stat == "none" )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_CHARGER_OUT );

            switch( CubicStatGetI( CUBIC_STAT_bat_level ) )
            {
            case 0:
                CubicPost( CUBIC_THIS_APP, CUBIC_MSG_BATTERY_OUT );
                break;

            case 1:
                CubicPost( CUBIC_THIS_APP, CUBIC_MSG_BATTERY_LOW );
                break;

            default: // include unknown status
                CubicPost( CUBIC_THIS_APP, CUBIC_MSG_BATTERY_NORMAL );
                break;
            }
        }

        // network status
        if( CubicStatGetI( CUBIC_STAT_net_uim_state ) == 0 )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_SIM_READY );
        }
        else
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_SIM_ERROR );
        }

        int v4_stat = CubicCfgGetI( CUBIC_STAT_net_wanstat_v4 );
        int v6_stat = CubicCfgGetI( CUBIC_STAT_net_wanstat_v6 );

        if( CubicStatGetI( CUBIC_STAT_net_connected ) == 1 )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_NETWORK_CONNECTED );

            // sip status
            if( CubicStatGetI( CUBIC_STAT_sip_registered ) == 1 )
            {
                CubicPost( CUBIC_THIS_APP, CUBIC_MSG_REGISTER_SUCCESS );
            }
            else if( CubicCfgGetI( CUBIC_STAT_sip_stat ) != 0x03 )
            {
                CubicPost( CUBIC_THIS_APP, CUBIC_MSG_REGISTER_LOST );
            }
        }
        else if( v4_stat != 0 && v4_stat != 0x01 &&
                 v6_stat != 0 && v6_stat != 0x07 )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_NETWORK_LOST );
        }

        // GPS status
        if( CubicStatGetI( CUBIC_STAT_location_valid ) == 1 )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_GPS_RENEW );
        }
        else
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_GPS_LOST );
        }

        // BT status
        if( CubicStatGetStr( CUBIC_STAT_bt_status ) == "on" )
        {
            CubicPost( CUBIC_THIS_APP, CUBIC_MSG_BT_ON );
        }
    };

    bool playVM( const string &id )
    {
        if( id.length() <= 0 )
        {
            LOGE( "playVM, VM id not available !" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            return false;
        }

        char file[PATH_MAX];
        snprintf( file, ( size_t ) PATH_MAX, CUBIC_VOICE_MSG_CACHE"/%s", id.c_str() );

        if( access( file, 0 ) != 0 )
        {
            LOGE( "playVM, file not ready !" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            return false;
        }

        if( !playAudio( file ) )
        {
            LOGE( "playVM, play message failed %s", file );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            return false;
        }

        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_ON );
        return true;
    }

    void stopPlay()
    {
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_PLAY_STOP );
    };

    bool playAudio( const string &url, bool loop = false )
    {
        cubic_msg_sound_play_start arg;
        memset( &arg, 0, sizeof( cubic_msg_sound_play_start ) );
        strncpy( arg.path, url.c_str(), CUBIC_PATH_MAX );

        if( loop )
        {
            arg.loop = 1;
        }

        return ( 0 == CubicPostReq( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_PLAY_START, arg ) );
    };

    void stopRecord()
    {
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_RECORD_STOP );
    };

    bool recordAudio( const string &url )
    {
        cubic_msg_sound_record_start arg;
        memset( &arg, 0, sizeof( cubic_msg_sound_record_start ) );
        strncpy( arg.path, url.c_str(), CUBIC_PATH_MAX );
        return ( 0 == CubicPostReq( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_RECORD_START, arg ) );
    };

    inline string getVMPath( int id )
    {
        char path[PATH_MAX + 4];
        snprintf( path, PATH_MAX, CUBIC_RECORD_PATH, id );
        return string( path );
    };

    string stopVMRecord()
    {
        RETNIF( m_record_vm_timer < 0, "" );
        LOGD( "stopVMRecord" );
        stopRecord();
        CubicKillTimer( m_record_vm_timer );
        string ret = getVMPath( m_record_vm_timer );
        m_record_vm_timer = -1;
        return ret;
    };

    bool startVMRecord()
    {
        LOGD( "startVMRecord" );
        stopVMRecord();
        m_record_vm_timer = CubicSetTimer( CUBIC_VM_RECORD_TIMEOUT, this );
        RETNIF_LOGE( m_record_vm_timer < 0, false, "startVMRecord failed, can not setup timer !" );
        return recordAudio( getVMPath( m_record_vm_timer ) );
    };

    void stopRing()
    {
        RETIF( m_call_ring_timer < 0 );
        LOGD( "stopRing" );
        stopRecord();
        stopPlay();
        CubicKillTimer( m_call_ring_timer );
        m_call_ring_timer = -1;
    };

    bool startRing()
    {
        LOGD( "startRing" );
        stopRing();
        m_call_ring_timer = CubicSetTimer( CUBIC_RING_TIMEOUT, this );
        RETNIF_LOGE( m_call_ring_timer < 0, false, "startRing failed, can not setup timer !" );
        return playAudio( CUBIC_RING_PATH, true );
    };

    void stopDial()
    {
        RETIF( m_call_dial_timer < 0 );
        LOGD( "stopDial" );
        CubicKillTimer( m_call_dial_timer );
        m_call_dial_timer = -1;
    };

    bool startDial( const string &number )
    {
        LOGD( "startDial number:%s", number.c_str() );
        stopDial();
        m_call_dial_timer = CubicSetTimer( CUBIC_DIAL_TIMEOUT, this );
        RETNIF_LOGE( m_call_dial_timer < 0, false, "startDial failed, can not setup timer !" );
        cubic_msg_sip_call_dial arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.peer_uuid, number.c_str(), CUBIC_UUID_LEN_MAX );
        CubicPostReq( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_DIAL, arg );
        return true;
    };

    void cancelActivateTimer()
    {
        RETIF( m_activate_timer < 0 );
        LOGD( "cancelActivateTimer" );
        CubicKillTimer( m_activate_timer );
        m_activate_timer = -1;
    };

    void setActivateTimer()
    {
        cancelActivateTimer();
        m_activate_timer = CubicSetTimer( CUBIC_ACTIVATE_TIMEOUT, this );
        LOGD( "setActivateTimer id=%d", m_activate_timer );
    };


    void cancelActiveTimer()
    {
        RETIF( m_active_timer < 0 );
        LOGD( "cancelActiveTimer" );
        CubicKillTimer( m_active_timer );
        m_active_timer = -1;
    };

    void setActiveTimer()
    {
        cancelActiveTimer();
        int timeout = CubicCfgGetI( CUBIC_CFG_active_timeout );
        RETIF( timeout <= 0 );
        timeout *= 1000; // to ms
        m_active_timer = CubicSetTimer( timeout, this );
        LOGD( "setActiveTimer id=%d", m_active_timer );
    };

    void cancelReportTrackingTimer()
    {
        RETIF( m_report_tracking_timer < 0 );
        LOGD( "cancelReportTrackingTimer" );
        CubicKillAbsTimer( m_report_tracking_timer );
        m_report_tracking_timer = -1;
    };

    void setReportTrackingTimer()
    {
        cancelReportTrackingTimer();
        int interval = CubicCfgGet( m_is_in_active_mode ? CUBIC_CFG_report_interval : CUBIC_CFG_report_p_interval, ( int )CUBIC_DEF_REPORT_INTERVAL );
        m_report_tracking_timer = CubicSetAbsTimerInterval( interval, this );
        LOGD( "setReportTrackingTimer id=%d", m_report_tracking_timer );
    };

    void cancelReportHeartbeatTimer()
    {
        RETIF( m_report_heartbeat_timer < 0 );
        LOGD( "cancelReportHeartbeatTimer" );
        CubicKillAbsTimer( m_report_heartbeat_timer );
        m_report_heartbeat_timer = -1;
    };

    void setReportHeartbeatTimer()
    {
        cancelReportHeartbeatTimer();
        m_report_heartbeat_timer = CubicSetAbsTimerInterval( CUBIC_HEARTBEAT_TIMEOUT, this );
        LOGD( "setReportHeartbeatTimer id=%d", m_report_heartbeat_timer );
    };

    void cancelJoinGroupTimer()
    {
        RETIF( m_join_group_timer < 0 );
        LOGD( "cancelJoinGroupTimer" );
        CubicKillTimer( m_join_group_timer );
        m_join_group_timer = -1;
    };

    void setJoinGroupTimer()
    {
        cancelJoinGroupTimer();
        m_join_group_timer = CubicSetTimer( CUBIC_JOIN_GROUP_TIMEOUT, this );
        LOGD( "setJoinGroupTimer id=%d", m_join_group_timer );
    };

    void cancelLMTimer()
    {
        RETIF( m_lm_timer < 0 );
        LOGD( "cancelLMTimer" );
        CubicKillTimer( m_lm_timer );
        m_lm_timer = -1;
    };

    void setLMTimer()
    {
        cancelLMTimer();
        m_lm_timer = CubicSetTimer( CUBIC_LM_TIMEOUT, this );
        LOGD( "setLMTimer id=%d", m_lm_timer );
    };

    void joinGroup()
    {
        string gid = CubicCfgGetStr( CUBIC_CFG_push_group_invite_gid );
        string token = CubicCfgGetStr( CUBIC_CFG_push_group_invite_token );
        int retry = CubicCfgGetI( CUBIC_CFG_push_group_invite_retry_cnt );
        RETIF_LOGE( retry > CUBIC_JOIN_GROUP_MAX_RETRY, "joinGroup reach max retry limit !" );
        CubicStatSet( CUBIC_STAT_core_group_join_stat, TRUE );
        CubicWakeupLockSet( CUBIC_WAKELOCK_ID_JOINGROUP );

        if( 0 > CRemoteReport::joinGroup( gid, token ) )
        {
            // retry if not reach limit
            retry ++;
            CubicCfgSet( CUBIC_CFG_push_group_invite_retry_cnt, retry );
            setJoinGroupTimer();
        }
        else
        {
            LOGE( "--== joinGroup success ==--" );
            CubicCfgSet( CUBIC_CFG_push_group_invite_gid, ( string )"" );
            CubicCfgSet( CUBIC_CFG_push_group_invite_token, ( string )"" );
            CubicCfgSet( CUBIC_CFG_push_group_invite_retry_cnt, ( string )"0" );
            setBTActiveTimer();
            playAudio( CUBIC_BEEP_PATH );
        }

        CubicStatSet( CUBIC_STAT_core_group_join_stat, FALSE );
        CubicWakeupLockClear( CUBIC_WAKELOCK_ID_JOINGROUP );
    };

    void cancelBTStartTimer()
    {
        RETIF( m_bt_start_timer < 0 );
        LOGI( "cancelBTStartTimer" );
        CubicKillTimer( m_bt_start_timer );
        m_bt_start_timer = -1;
    };

    void setBTStartTimer()
    {
        cancelBTStartTimer();
        m_bt_start_timer = CubicSetTimer( CUBIC_BT_START_TIMEOUT, this );
        LOGI( "setStartTimer id=%d", m_bt_start_timer );
    };

    void cancelBTFlashTimer()
    {
        RETIF( m_bt_flash_timer < 0 );
        LOGD( "cancelBTFlashTimer" );
        CubicKillTimer( m_bt_flash_timer );
        m_bt_flash_timer = -1;
    };

    void setBTFlashTimer()
    {
        cancelBTFlashTimer();
        m_bt_flash_timer = CubicSetTimer( CUBIC_BT_FLASH_TIMEOUT, this );
        LOGD( "setBTFlashTimer id=%d", m_bt_flash_timer );
    };

    void cancelBTActiveTimer()
    {
        RETIF( m_bt_active_timer < 0 );
        LOGD( "cancelBTActiveTimer" );
        CubicKillTimer( m_bt_active_timer );
        m_bt_active_timer = -1;
    };

    void setBTActiveTimer()
    {
        cancelBTActiveTimer();
        int bt_active = CubicCfgGetI( CUBIC_CFG_ble_active_timeout );
        RETIF_LOGD( bt_active == 0, "not need set BT active Timer, BT will always be running." );
        m_bt_active_timer = CubicSetTimer( bt_active, this );
        LOGD( "setBTActiveTimer id=%d", m_bt_active_timer );
    };

    void cancelOTAErrFlashTimer()
    {
        RETIF( m_ota_err_flash_timer < 0 );
        LOGD( "cancelOTAErrFlashTimer" );
        CubicKillTimer( m_ota_err_flash_timer );
        m_ota_err_flash_timer = -1;
    };

    void setOTAErrFlashTimer()
    {
        cancelOTAErrFlashTimer();
        m_ota_err_flash_timer = CubicSetTimer( CUBIC_OTA_ERROR_FLASH_TIMEOUT, this );
        LOGD( "setOTAErrFlashTimer id=%d", m_ota_err_flash_timer );
    };

    static bool isActivated()
    {
        string client_id = CubicCfgGetStr( CUBIC_CFG_push_uname );
        LOGD( "client_id=%s, length=%d", client_id.c_str(), client_id.length() );
        RETNIF( client_id.length() == 0 || client_id == "null", false );
        return true;
    }

    static void report_tracking( void *arg = NULL )
    {
        UNUSED_ARG( arg );
        RETIF( !isActivated() );
        RETIF_LOGD(
            CubicStatGetF( CUBIC_STAT_location_lat ) == 0.0 && CubicStatGetF( CUBIC_STAT_location_long ) == 0.0,
            "report_tracking, no fix location to report" );

        for( int i = 0; CRemoteReport::reportLocation() != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };

    static void report_heartbeat( void *arg = NULL )
    {
        UNUSED_ARG( arg );
        RETIF( !isActivated() );

        for( int i = 0; CRemoteReport::reportLocation( true ) != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };


    static void report_battery( void *arg = NULL )
    {
        UNUSED_ARG( arg );

        for( int i = 0; CRemoteReport::reportBattery() != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };

    static void report_gps_fence( report_gps_fence_t arg )
    {
        for( int i = 0; CRemoteReport::reportGeoFence( arg.index, arg.is_in == 1 ) != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };

    static void report_help( void *arg = NULL )
    {
        UNUSED_ARG( arg );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_SOS_ON );

        for( int i = 0; CRemoteReport::reportHelp() != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }

        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_SOS_OFF );
    };

    static void report_new_firmware( report_new_firmware_t arg )
    {
        for( int i = 0; CRemoteReport::reportNewFirmware( arg.version ) != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };


    static void activate_register_and_poll( void *arg = NULL )
    {
        UNUSED_ARG( arg );

        if( !CoreApp::getInstance().isActivated() )
        {
            for( int i = 0; CRemoteReport::activate() != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
            {
                sleep( 1 );
            }

            if( !CoreApp::getInstance().isActivated() )
            {
                // do nothing, because status will automatically update periodic
                LOGE( "activate_register_and_poll failed when activate" );
                asyncInput( EVT_ACTIVATE_FAIL );
                return;
            }
        }

        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_ACTIVATE_OFF );
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_REGISTER );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }


    // ************************************************************************
    // ****                        STATUS MACHINE                          ****
    enum
    {
        STAT_NOT_READY = STAT_BEGIN, // 1
        STAT_READY,
        STAT_VM_PLAYING,
        STAT_VM_RECORDING,
        STAT_CALL_RINGING,
        STAT_CALL_DIALING,
        STAT_CALL_CONVERSATION,
        STAT_LM_PLAY,
        STAT_TEST_MIC,
    };

    enum
    {
        EVT_NETWORK_CONNECTED = EVT_DEFAULT + 1, // 1
        EVT_NETWORK_LOST,
        EVT_ACTIVATE_FAIL,
        EVT_REGISTER_SUCCESS,
        EVT_REGISTER_LOST,
        EVT_CALL_RECEIVED,
        EVT_CALL_ACCEPTED,
        EVT_CALL_HANGUPED,
        EVT_CALL_RING_TIMEOUT,
        EVT_CALL_DIAL_TIMEOUT,
        EVT_VM_PLAY,
        EVT_VM_PLAY_END,
        EVT_VM_RECORD_TIMEOUT,
        EVT_VM_RECORD_END,
        EVT_LM_PLAY,
        EVT_LM_TIMEOUT,
        EVT_DIAL_TEST,
        EVT_TEST_MIC,

        EVT_KEY_FIRST,
        EVT_KEY_PRESS_MAIN = EVT_KEY_FIRST,
        EVT_KEY_PRESS_MAIN_WITH_SUB1,
        EVT_KEY_PRESS_MAIN_WITH_SUB2,
        EVT_KEY_PRESS_MAIN_WITH_SUB3,
        EVT_KEY_LONGPRESS_MAIN,
        EVT_KEY_LONGPRESS_MAIN_WITH_POWR,
        EVT_KEY_LONGPRESS_ALL_SUB_KEY,
        EVT_KEY_CLICK_MAIN,
        EVT_KEY_DOUBLE_CLICK_MAIN,
        EVT_KEY_TRIPLE_CLICK_MAIN,
        EVT_KEY_RELEASE_MAIN,
        EVT_KEY_PRESS_PTT,
        EVT_KEY_LONGPRESS_PTT,
        EVT_KEY_CLICK_PTT,
        EVT_KEY_RELEASE_PTT,
        EVT_KEY_LONGPRESS_SUB1,
        EVT_KEY_CLICK_SUB1,
        EVT_KEY_RELEASE_SUB1,
        EVT_KEY_LONGPRESS_SUB2,
        EVT_KEY_CLICK_SUB2,
        EVT_KEY_RELEASE_SUB2,
        EVT_KEY_LONGPRESS_SUB3,
        EVT_KEY_CLICK_SUB3,
        EVT_KEY_RELEASE_SUB3,
        EVT_KEY_LONGPRESS_POWR,
        EVT_KEY_END
    };


#define ACT_CLASS(name) name##Act
#define ACT_NAME(name) m_##name##_act
#define IMPLEMENT_ACTION(name) \
    class ACT_CLASS(name) : public IAction { \
    public: \
        virtual bool onStatChange(TStat stat_from, TEvt evt, TStat stat_to, void* data) { \
            LOGD( #name" action =>" );


#define IMPLEMENT_ACTION_END(name) \
    return true;\
}; \
};\
friend class ACT_CLASS(name); \
ACT_CLASS(name)  ACT_NAME(name);


    IMPLEMENT_ACTION( activate_register_and_poll )
    {
        CAsyncRun<>::async_run( activate_register_and_poll );
    }
    IMPLEMENT_ACTION_END( activate_register_and_poll );

    IMPLEMENT_ACTION( stop_ring_and_register )
    {
        CoreApp::getInstance().stopRing();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_REGISTER );
    }
    IMPLEMENT_ACTION_END( stop_ring_and_register );

    IMPLEMENT_ACTION( stop_dial_and_register )
    {
        CoreApp::getInstance().stopDial();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_REGISTER );
    }
    IMPLEMENT_ACTION_END( stop_dial_and_register );

    IMPLEMENT_ACTION( set_activate_timer )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_ACTIVATE_ON );
        CoreApp::getInstance().setActivateTimer();
    }
    IMPLEMENT_ACTION_END( set_activate_timer );

    IMPLEMENT_ACTION( stop_ring )
    {
        CoreApp::getInstance().stopRing();
    }
    IMPLEMENT_ACTION_END( stop_ring );

    IMPLEMENT_ACTION( stop_dial )
    {
        CoreApp::getInstance().stopDial();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
    }
    IMPLEMENT_ACTION_END( stop_dial );


    IMPLEMENT_ACTION( show_signal )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_USR_CHK_NET_ON );
    }
    IMPLEMENT_ACTION_END( show_signal );

    IMPLEMENT_ACTION( show_battery )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_USR_CHK_BAT_ON );
    }
    IMPLEMENT_ACTION_END( show_battery );

    IMPLEMENT_ACTION( show_gps )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_USR_CHK_GPS_ON );
    }
    IMPLEMENT_ACTION_END( show_gps );

    IMPLEMENT_ACTION( end_all_shows )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_USR_CHK_NET_OFF );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_USR_CHK_BAT_OFF );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_USR_CHK_GPS_OFF );
    }
    IMPLEMENT_ACTION_END( end_all_shows );

    IMPLEMENT_ACTION( shutdown_system )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CoreApp::getInstance().shutdownSystem();
    }
    IMPLEMENT_ACTION_END( shutdown_system );

    IMPLEMENT_ACTION( restore_factory )
    {
        LOGE( "restore_factory setting ..." );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_RESTORING_ON );
        //CUtil::syscommand( "rm -rf %s && sync", CUBIC_CONFIG_PATH );
        CUtil::removeDir( CUBIC_CONFIG_PATH );
        CoreApp::getInstance().shutdownSystem( true );
        // power off will off the light anyway
        //CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_RESTORING_OFF );
    }
    IMPLEMENT_ACTION_END( restore_factory );

    IMPLEMENT_ACTION( report_help )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CAsyncRun<>::async_run( report_help );
    }
    IMPLEMENT_ACTION_END( report_help );

    IMPLEMENT_ACTION( play_lm )
    {
        evt_lm_play *arg = ( evt_lm_play * )data;

        if( !CoreApp::getInstance().playAudio( arg->path ) )
        {
            LOGE( "play_voice_message failed, playAudio failed: %s", arg->path );
            // treat as play done, that mean we will skip this one
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            CoreApp::getInstance().playAudio( CUBIC_BEEP_PATH );
            return false;
        }
    }
    IMPLEMENT_ACTION_END( play_lm );

    IMPLEMENT_ACTION( set_lm_timeout )
    {
        CoreApp::getInstance().setLMTimer();
    }
    IMPLEMENT_ACTION_END( set_lm_timeout );

    IMPLEMENT_ACTION( enter_lm_play )
    {
        int max_idx = CubicStatGetI( CUBIC_STAT_vm_read );

        if( max_idx <= 0 )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            return false;
        }

        CoreApp::getInstance().m_curr_play_lm_idx = 0;
        cubic_msg_vm_get_lm arg;
        arg.index = CoreApp::getInstance().m_curr_play_lm_idx;
        CubicPostReq( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_GET_LM, arg );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_ON );
        CubicWakeupLockSet( CUBIC_WAKELOCK_ID_LEAVE_MSG );
    }
    IMPLEMENT_ACTION_END( enter_lm_play );

    IMPLEMENT_ACTION( exit_lm_play_and_resume_vm )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_OFF );
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().m_curr_play_lm_idx = 0;
        CoreApp::getInstance().playAudio( CUBIC_BEEP_PATH );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
        CubicWakeupLockClear( CUBIC_WAKELOCK_ID_LEAVE_MSG );
    }
    IMPLEMENT_ACTION_END( exit_lm_play_and_resume_vm );

    IMPLEMENT_ACTION( play_prev_lm )
    {
        CoreApp::getInstance().setLMTimer();

        if( 0 == CoreApp::getInstance().m_curr_play_lm_idx )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            return false;
        }

        CoreApp::getInstance().m_curr_play_lm_idx--;
        cubic_msg_vm_get_lm arg;
        arg.index = CoreApp::getInstance().m_curr_play_lm_idx;
        CubicPostReq( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_GET_LM, arg );
    }
    IMPLEMENT_ACTION_END( play_prev_lm );

    IMPLEMENT_ACTION( play_next_lm )
    {
        CoreApp::getInstance().setLMTimer();
        int max_idx = CubicStatGetI( CUBIC_STAT_vm_read );

        if( max_idx <= CoreApp::getInstance().m_curr_play_lm_idx + 1 )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            return false;
        }

        CoreApp::getInstance().m_curr_play_lm_idx++;
        cubic_msg_vm_get_lm arg;
        arg.index = CoreApp::getInstance().m_curr_play_lm_idx;
        CubicPostReq( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_GET_LM, arg );
    }
    IMPLEMENT_ACTION_END( play_next_lm );

    IMPLEMENT_ACTION( play_current_lm )
    {
        CoreApp::getInstance().setLMTimer();
        cubic_msg_vm_get_lm arg;
        arg.index = CoreApp::getInstance().m_curr_play_lm_idx;
        CubicPostReq( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_GET_LM, arg );
    }
    IMPLEMENT_ACTION_END( play_current_lm );

    IMPLEMENT_ACTION( start_ring )
    {
        CoreApp::getInstance().startRing();
    }
    IMPLEMENT_ACTION_END( start_ring );

    IMPLEMENT_ACTION( play_voice_message )
    {
        evt_vm_play *arg = ( evt_vm_play * )data;

        if( !CoreApp::getInstance().playAudio( arg->path ) )
        {
            LOGE( "play_voice_message failed, playAudio failed: %s", arg->path );
            // treat as play done, that mean we will skip this one
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            CoreApp::getInstance().playAudio( CUBIC_BEEP_PATH );
            return false;
        }

        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_ON );
    }
    IMPLEMENT_ACTION_END( play_voice_message );

    IMPLEMENT_ACTION( start_vm_record )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        string group_id = CubicCfgGetStr( CUBIC_CFG_push_group );
        LOGD( "group_id=%s, length=%d", group_id.c_str(), group_id.length() );
        RETNIF_LOGE( group_id.length() == 0 || group_id == "null", false, "not in any group yet !" );
        RETNIF_LOGE( !CoreApp::getInstance().startVMRecord(),
                     false, "start_vm_record failed, startVMRecord failed" );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_ON );
    }
    IMPLEMENT_ACTION_END( start_vm_record );

    IMPLEMENT_ACTION( stop_play_and_start_vm_record )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CoreApp::getInstance().stopPlay();
        RETNIF_LOGE( !CoreApp::getInstance().startVMRecord(),
                     false, "stop_play_and_start_vm_record failed, startVMRecord failed" );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_ON );
    }
    IMPLEMENT_ACTION_END( stop_play_and_start_vm_record );

    IMPLEMENT_ACTION( stop_vm_record )
    {
        CoreApp::getInstance().stopVMRecord();
    }
    IMPLEMENT_ACTION_END( stop_vm_record );

    IMPLEMENT_ACTION( send_and_resume_vm )
    {
        evt_vm_record_end *pdata = ( evt_vm_record_end * )data;
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_OFF );

        if( pdata->error != 0 )
        {
            LOGE( "send_and_resume_vm, error when record" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
            return true;
        }

        cubic_msg_vm_send arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.path, pdata->path, CUBIC_PATH_MAX );
        CubicPostReq( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_SEND, arg );
        CoreApp::getInstance().playAudio( CUBIC_BEEP_PATH );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( send_and_resume_vm );

    IMPLEMENT_ACTION( begin_mic_test )
    {
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_ON );
    }
    IMPLEMENT_ACTION_END( begin_mic_test );

    IMPLEMENT_ACTION( end_mic_test )
    {
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_OFF );
    }
    IMPLEMENT_ACTION_END( end_mic_test );

    IMPLEMENT_ACTION( start_record_test )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        RETNIF_LOGE( !CoreApp::getInstance().startVMRecord(),
                     false, "start_vm_record failed, startVMRecord failed" );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_ON );
    }
    IMPLEMENT_ACTION_END( start_record_test );

    IMPLEMENT_ACTION( play_test_file )
    {
        evt_vm_record_end *pdata = ( evt_vm_record_end * )data;
        CoreApp::getInstance().playAudio( pdata->path );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_OFF );
    }
    IMPLEMENT_ACTION_END( play_test_file );

    IMPLEMENT_ACTION( remove_test_file )
    {
        evt_vm_play_end *pdata = ( evt_vm_play_end * )data;
        CoreApp::getInstance().stopPlay();
        LOGD( "remove_test_file: %s", pdata->path );
        unlink( pdata->path );
    }
    IMPLEMENT_ACTION_END( remove_test_file );

    IMPLEMENT_ACTION( volume_down )
    {
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_VOL_DOWN );
    }
    IMPLEMENT_ACTION_END( volume_down );

    IMPLEMENT_ACTION( volume_up )
    {
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_VOL_UP );
    }
    IMPLEMENT_ACTION_END( volume_up );

    IMPLEMENT_ACTION( enable_ble )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_ENABLE );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_BT_ON );
    }
    IMPLEMENT_ACTION_END( enable_ble );

    IMPLEMENT_ACTION( dial_preset_num_1 )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        string number = CubicCfgGetVStr( CUBIC_CFG_preset_number, 1 );
        RETNIF_LOGE( number.length() <= 0 || number == "null", false, "dial_preset_num_1 failed, the preset number is empty" );
        RETNIF_LOGE( !CoreApp::getInstance().startDial( number ), false, "dial_preset_num_1 failed, startDial failed" );
    }
    IMPLEMENT_ACTION_END( dial_preset_num_1 );

    IMPLEMENT_ACTION( dial_preset_num_2 )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        string number = CubicCfgGetVStr( CUBIC_CFG_preset_number, 2 );
        RETNIF_LOGE( number.length() <= 0 || number == "null", false, "dial_preset_num_2 failed, the preset number is empty" );
        RETNIF_LOGE( !CoreApp::getInstance().startDial( number ), false, "dial_preset_num_2 failed, startDial failed" );
    }
    IMPLEMENT_ACTION_END( dial_preset_num_2 );

    IMPLEMENT_ACTION( dial_preset_num_3 )
    {
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        string number = CubicCfgGetVStr( CUBIC_CFG_preset_number, 3 );
        RETNIF_LOGE( number.length() <= 0 || number == "null", false, "dial_preset_num_3 failed, the preset number is empty" );
        RETNIF_LOGE( !CoreApp::getInstance().startDial( number ), false, "dial_preset_num_3 failed, startDial failed" );
    }
    IMPLEMENT_ACTION_END( dial_preset_num_3 );

    IMPLEMENT_ACTION( dial_num )
    {
        string number = ( char * )data;
        RETNIF_LOGE( number.length() <= 0 || number == "null", false, "dial_num failed, the preset number is empty" );
        RETNIF_LOGE( !CoreApp::getInstance().startDial( number ), false, "dial_num failed, startDial failed" );
    }
    IMPLEMENT_ACTION_END( dial_num );

    IMPLEMENT_ACTION( setup_sip_sound_and_accept )
    {
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_SIP_START );
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_ACCEPT );
    }
    IMPLEMENT_ACTION_END( setup_sip_sound_and_accept );

    IMPLEMENT_ACTION( stop_sound_and_hangup_and_resume_vm )
    {
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( stop_sound_and_hangup_and_resume_vm );

    IMPLEMENT_ACTION( stop_ring_and_hangup_and_resume_vm )
    {
        CoreApp::getInstance().stopRing();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( stop_ring_and_hangup_and_resume_vm );


    IMPLEMENT_ACTION( stop_dial_and_setup_sip_sound )
    {
        CoreApp::getInstance().stopDial();
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_SIP_START );
    }
    IMPLEMENT_ACTION_END( stop_dial_and_setup_sip_sound );

    IMPLEMENT_ACTION( stop_dial_and_resume_vm )
    {
        CoreApp::getInstance().stopDial();
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( stop_dial_and_resume_vm );

    IMPLEMENT_ACTION( stop_dial_and_and_hangup )
    {
        CoreApp::getInstance().stopDial();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( stop_dial_and_and_hangup );

    IMPLEMENT_ACTION( stop_sound_and_hangup )
    {
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
    }
    IMPLEMENT_ACTION_END( stop_sound_and_hangup );

    IMPLEMENT_ACTION( stop_sound_and_hangup_and_register )
    {
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_CALL_HANGUP );
        CubicPost( CUBIC_APP_NAME_SIP_SERVICE, CUBIC_MSG_SIP_REGISTER );
    }
    IMPLEMENT_ACTION_END( stop_sound_and_hangup_and_register );

    IMPLEMENT_ACTION( mute_or_resume )
    {
        CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_VOL_MUTE_OR_RESUME );
    }
    IMPLEMENT_ACTION_END( mute_or_resume );

    IMPLEMENT_ACTION( stop_sound_and_resume_vm )
    {
        CoreApp::getInstance().stopPlay();
        CoreApp::getInstance().stopRecord();
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( stop_sound_and_resume_vm );

    IMPLEMENT_ACTION( stop_play_and_poll )
    {
        evt_vm_play_end *pdata = ( evt_vm_play_end * )data;
        CoreApp::getInstance().stopPlay();
        cubic_msg_vm_read arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.path, pdata->path, CUBIC_PATH_MAX );
        arg.error = !pdata->complete;
        CubicPostReq( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_READ, arg );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_MESSAGE_OFF );
        CubicPost( CUBIC_APP_NAME_VM_SERVICE, CUBIC_MSG_VM_POLL );
    }
    IMPLEMENT_ACTION_END( stop_play_and_poll );



    void init_status_bridge()
    {
        // STAT_NOT_READY
        insertAction( STAT_NOT_READY, EVT_NETWORK_CONNECTED,             STAT_READY,         &ACT_NAME( activate_register_and_poll ) );
        insertAction( STAT_NOT_READY, EVT_REGISTER_SUCCESS,              STAT_READY,         NULL );
        insertAction( STAT_NOT_READY, EVT_ACTIVATE_FAIL,                 STAT_NOT_READY,     &ACT_NAME( set_activate_timer ) );
        //insertAction( STAT_NOT_READY, EVT_KEY_CLICK_SUB1,                STAT_NOT_READY,     &ACT_NAME( volume_down ) );
        insertAction( STAT_NOT_READY, EVT_KEY_CLICK_SUB2,                STAT_NOT_READY,     &ACT_NAME( enable_ble ) );
        //insertAction( STAT_NOT_READY, EVT_KEY_CLICK_SUB3,                STAT_NOT_READY,     &ACT_NAME( volume_up ) );
        insertAction( STAT_NOT_READY, EVT_KEY_PRESS_MAIN_WITH_SUB1,      STAT_NOT_READY,     &ACT_NAME( show_signal ) );
        insertAction( STAT_NOT_READY, EVT_KEY_PRESS_MAIN_WITH_SUB2,      STAT_NOT_READY,     &ACT_NAME( show_battery ) );
        insertAction( STAT_NOT_READY, EVT_KEY_PRESS_MAIN_WITH_SUB3,      STAT_NOT_READY,     &ACT_NAME( show_gps ) );
        //insertAction( STAT_NOT_READY, EVT_KEY_CLICK_MAIN,                STAT_NOT_READY,     &ACT_NAME( end_all_shows ) );
        insertAction( STAT_NOT_READY, EVT_KEY_RELEASE_MAIN,              STAT_NOT_READY,     &ACT_NAME( end_all_shows ) );
        insertAction( STAT_NOT_READY, EVT_KEY_LONGPRESS_POWR,            STAT_NOT_READY,     &ACT_NAME( shutdown_system ) );
        insertAction( STAT_NOT_READY, EVT_KEY_LONGPRESS_MAIN_WITH_POWR,  STAT_NOT_READY,     &ACT_NAME( restore_factory ) );
        insertAction( STAT_NOT_READY, EVT_VM_PLAY,                       STAT_VM_PLAYING,    &ACT_NAME( play_voice_message ) );
        insertAction( STAT_NOT_READY, EVT_TEST_MIC,                      STAT_TEST_MIC,      &ACT_NAME( begin_mic_test ) );
        // STAT_READY
        insertAction( STAT_READY,     EVT_NETWORK_LOST,                  STAT_NOT_READY,     NULL );
        insertAction( STAT_READY,     EVT_ACTIVATE_FAIL,                 STAT_NOT_READY,     &ACT_NAME( set_activate_timer ) );
        insertAction( STAT_READY,     EVT_CALL_RECEIVED,                 STAT_CALL_RINGING,  &ACT_NAME( start_ring ) );
        insertAction( STAT_READY,     EVT_VM_PLAY,                       STAT_VM_PLAYING,    &ACT_NAME( play_voice_message ) );
        //insertAction( STAT_READY,     EVT_KEY_CLICK_SUB1,                STAT_READY,         &ACT_NAME( volume_down ) );
        insertAction( STAT_READY,     EVT_KEY_CLICK_SUB2,                STAT_READY,         &ACT_NAME( enable_ble ) );
        //insertAction( STAT_READY,     EVT_KEY_CLICK_SUB3,                STAT_READY,         &ACT_NAME( volume_up ) );
        insertAction( STAT_READY,     EVT_KEY_PRESS_MAIN,                STAT_VM_RECORDING,  &ACT_NAME( start_vm_record ) );
        insertAction( STAT_READY,     EVT_KEY_PRESS_PTT,                 STAT_VM_RECORDING,  &ACT_NAME( start_vm_record ) );
        insertAction( STAT_READY,     EVT_KEY_LONGPRESS_SUB1,            STAT_CALL_DIALING,  &ACT_NAME( dial_preset_num_1 ) );
        insertAction( STAT_READY,     EVT_KEY_LONGPRESS_SUB2,            STAT_CALL_DIALING,  &ACT_NAME( dial_preset_num_2 ) );
        insertAction( STAT_READY,     EVT_KEY_LONGPRESS_SUB3,            STAT_CALL_DIALING,  &ACT_NAME( dial_preset_num_3 ) );
        insertAction( STAT_READY,     EVT_KEY_PRESS_MAIN_WITH_SUB1,      STAT_READY,         &ACT_NAME( show_signal ) );
        insertAction( STAT_READY,     EVT_KEY_PRESS_MAIN_WITH_SUB2,      STAT_READY,         &ACT_NAME( show_battery ) );
        insertAction( STAT_READY,     EVT_KEY_PRESS_MAIN_WITH_SUB3,      STAT_READY,         &ACT_NAME( show_gps ) );
        //insertAction( STAT_READY,     EVT_KEY_CLICK_MAIN,                STAT_READY,         &ACT_NAME( end_all_shows ) );
        insertAction( STAT_READY,     EVT_KEY_RELEASE_MAIN,              STAT_READY,         &ACT_NAME( end_all_shows ) );
        insertAction( STAT_READY,     EVT_KEY_LONGPRESS_POWR,            STAT_READY,         &ACT_NAME( shutdown_system ) );
        insertAction( STAT_READY,     EVT_KEY_LONGPRESS_MAIN_WITH_POWR,  STAT_READY,         &ACT_NAME( restore_factory ) );
        insertAction( STAT_READY,     EVT_KEY_TRIPLE_CLICK_MAIN,         STAT_READY,         &ACT_NAME( report_help ) );
        insertAction( STAT_READY,     EVT_KEY_LONGPRESS_ALL_SUB_KEY,     STAT_READY,         &ACT_NAME( report_help ) );
        insertAction( STAT_READY,     EVT_KEY_DOUBLE_CLICK_MAIN,         STAT_LM_PLAY,       &ACT_NAME( enter_lm_play ) );
        insertAction( STAT_READY,     EVT_DIAL_TEST,                     STAT_CALL_DIALING,  &ACT_NAME( dial_num ) );
        insertAction( STAT_READY,     EVT_TEST_MIC,                      STAT_TEST_MIC,      &ACT_NAME( begin_mic_test ) );
        // STAT_CALL_RINGING
        insertAction( STAT_CALL_RINGING,     EVT_NETWORK_LOST,           STAT_NOT_READY,         &ACT_NAME( stop_ring ) );
        insertAction( STAT_CALL_RINGING,     EVT_REGISTER_LOST,          STAT_READY,             &ACT_NAME( stop_ring_and_register ) );
        insertAction( STAT_CALL_RINGING,     EVT_CALL_HANGUPED,          STAT_READY,             &ACT_NAME( stop_ring_and_register ) );
        insertAction( STAT_CALL_RINGING,     EVT_KEY_CLICK_MAIN,         STAT_CALL_CONVERSATION, &ACT_NAME( setup_sip_sound_and_accept ) );
        insertAction( STAT_CALL_RINGING,     EVT_KEY_CLICK_PTT ,         STAT_CALL_CONVERSATION, &ACT_NAME( setup_sip_sound_and_accept ) );
        insertAction( STAT_CALL_RINGING,     EVT_KEY_LONGPRESS_POWR,     STAT_READY,             &ACT_NAME( stop_ring_and_hangup_and_resume_vm ) );
        insertAction( STAT_CALL_RINGING,     EVT_CALL_RING_TIMEOUT,      STAT_READY,             &ACT_NAME( stop_ring_and_hangup_and_resume_vm ) );
        // STAT_CALL_DIALING
        insertAction( STAT_CALL_DIALING,     EVT_NETWORK_LOST,           STAT_NOT_READY,         &ACT_NAME( stop_dial ) );
        insertAction( STAT_CALL_DIALING,     EVT_REGISTER_LOST,          STAT_READY,             &ACT_NAME( stop_dial_and_register ) );
        insertAction( STAT_CALL_DIALING,     EVT_CALL_ACCEPTED,          STAT_CALL_CONVERSATION, &ACT_NAME( stop_dial_and_setup_sip_sound ) );
        insertAction( STAT_CALL_DIALING,     EVT_CALL_HANGUPED,          STAT_READY,             &ACT_NAME( stop_dial_and_resume_vm ) );
        insertAction( STAT_CALL_DIALING,     EVT_CALL_DIAL_TIMEOUT,      STAT_READY,             &ACT_NAME( stop_dial_and_and_hangup ) );
        // STAT_CALL_CONVERSATION
        insertAction( STAT_CALL_CONVERSATION,     EVT_NETWORK_LOST,       STAT_NOT_READY,            &ACT_NAME( stop_sound_and_hangup ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_REGISTER_LOST,      STAT_READY,                &ACT_NAME( stop_sound_and_hangup_and_register ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_KEY_CLICK_SUB1,     STAT_CALL_CONVERSATION,    &ACT_NAME( volume_down ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_KEY_CLICK_SUB3,     STAT_CALL_CONVERSATION,    &ACT_NAME( volume_up ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_KEY_CLICK_MAIN,     STAT_READY,                &ACT_NAME( stop_sound_and_hangup_and_resume_vm ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_KEY_CLICK_PTT,      STAT_READY,                &ACT_NAME( stop_sound_and_hangup_and_resume_vm ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_KEY_LONGPRESS_MAIN, STAT_CALL_CONVERSATION,    &ACT_NAME( mute_or_resume ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_KEY_LONGPRESS_PTT,  STAT_CALL_CONVERSATION,    &ACT_NAME( mute_or_resume ) );
        insertAction( STAT_CALL_CONVERSATION,     EVT_CALL_HANGUPED,      STAT_READY,                &ACT_NAME( stop_sound_and_resume_vm ) );
        // STAT_VM_PLAYING
        insertAction( STAT_VM_PLAYING,     EVT_VM_PLAY_END,               STAT_READY,         &ACT_NAME( stop_play_and_poll ) );
        //insertAction(STAT_VM_PLAYING,     EVT_NETWORK_LOST,              STAT_NOT_READY,     &ACT_NAME(stop_play));
        insertAction( STAT_VM_PLAYING,     EVT_KEY_CLICK_SUB1,            STAT_VM_PLAYING,    &ACT_NAME( volume_down ) );
        insertAction( STAT_VM_PLAYING,     EVT_KEY_CLICK_SUB3,            STAT_VM_PLAYING,    &ACT_NAME( volume_up ) );
        //insertAction( STAT_VM_PLAYING,     EVT_KEY_PRESS_MAIN,            STAT_VM_RECORDING,  &ACT_NAME( stop_play_and_start_vm_record ) );
        //insertAction( STAT_VM_PLAYING,     EVT_KEY_PRESS_PTT,             STAT_VM_RECORDING,  &ACT_NAME( stop_play_and_start_vm_record ) );
        // STAT_VM_RECORDING
        insertAction( STAT_VM_RECORDING,     EVT_NETWORK_LOST,            STAT_NOT_READY,     &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_VM_RECORDING,     EVT_KEY_CLICK_MAIN,          STAT_VM_RECORDING,  &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_VM_RECORDING,     EVT_KEY_CLICK_PTT,           STAT_VM_RECORDING,  &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_VM_RECORDING,     EVT_KEY_RELEASE_MAIN,        STAT_VM_RECORDING,  &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_VM_RECORDING,     EVT_KEY_RELEASE_PTT,         STAT_VM_RECORDING,  &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_VM_RECORDING,     EVT_VM_RECORD_TIMEOUT,       STAT_VM_RECORDING,  &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_VM_RECORDING,     EVT_VM_RECORD_END,           STAT_READY,         &ACT_NAME( send_and_resume_vm ) );
        // STAT_LM_PLAY
        insertAction( STAT_LM_PLAY,     EVT_LM_PLAY,               STAT_LM_PLAY,         &ACT_NAME( play_lm ) );
        insertAction( STAT_LM_PLAY,     EVT_VM_PLAY_END,           STAT_LM_PLAY,         &ACT_NAME( set_lm_timeout ) );
        insertAction( STAT_LM_PLAY,     EVT_LM_TIMEOUT,            STAT_READY,           &ACT_NAME( exit_lm_play_and_resume_vm ) );
        insertAction( STAT_LM_PLAY,     EVT_KEY_CLICK_SUB1,        STAT_LM_PLAY,         &ACT_NAME( play_prev_lm ) );
        insertAction( STAT_LM_PLAY,     EVT_KEY_CLICK_SUB2,        STAT_LM_PLAY,         &ACT_NAME( play_current_lm ) );
        insertAction( STAT_LM_PLAY,     EVT_KEY_CLICK_SUB3,        STAT_LM_PLAY,         &ACT_NAME( play_next_lm ) );
        insertAction( STAT_LM_PLAY,     EVT_KEY_DOUBLE_CLICK_MAIN, STAT_READY,           &ACT_NAME( exit_lm_play_and_resume_vm ) );
        // STAT_TEST_MIC
        insertAction( STAT_TEST_MIC,    EVT_KEY_CLICK_SUB1,        STAT_TEST_MIC,         &ACT_NAME( volume_down ) );
        insertAction( STAT_TEST_MIC,    EVT_KEY_CLICK_SUB3,        STAT_TEST_MIC,         &ACT_NAME( volume_up ) );
        insertAction( STAT_TEST_MIC,    EVT_KEY_PRESS_MAIN,        STAT_TEST_MIC,         &ACT_NAME( start_record_test ) );
        insertAction( STAT_TEST_MIC,    EVT_KEY_PRESS_PTT,         STAT_TEST_MIC,         &ACT_NAME( start_record_test ) );
        insertAction( STAT_TEST_MIC,    EVT_KEY_RELEASE_MAIN,      STAT_TEST_MIC,         &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_TEST_MIC,    EVT_KEY_RELEASE_PTT,       STAT_TEST_MIC,         &ACT_NAME( stop_vm_record ) );
        insertAction( STAT_TEST_MIC,    EVT_VM_RECORD_END,         STAT_TEST_MIC,         &ACT_NAME( play_test_file ) );
        insertAction( STAT_TEST_MIC,    EVT_VM_PLAY_END,           STAT_TEST_MIC,         &ACT_NAME( remove_test_file ) );
        insertAction( STAT_TEST_MIC,    EVT_TEST_MIC,              STAT_READY,            &ACT_NAME( end_mic_test ) );
    };

    // will process by core, asynchronously
    static int asyncInput( TEvt evt )
    {
        return CubicPost( CUBIC_THIS_APP, CUBIC_MSG_APP_PRIVATE + evt );
    };

    template<class T>
    static int asyncInput( TEvt evt, const T &data )
    {
        return CubicPostReq( CUBIC_THIS_APP, CUBIC_MSG_APP_PRIVATE + evt, data );
    };
    // ****                        STATUS MACHINE                          ****
    // ************************************************************************



    CoreApp()
        : CStatMachine( this )
        , m_record_vm_timer( -1 )
        , m_call_ring_timer( -1 )
        , m_call_dial_timer( -1 )
        , m_activate_timer( -1 )
        , m_report_tracking_timer( -1 )
        , m_join_group_timer( -1 )
        , m_bt_start_timer( -1 )
        , m_bt_flash_timer( -1 )
        , m_bt_active_timer( -1 )
        , m_ota_err_flash_timer( -1 )
        , m_active_timer( -1 )
        , m_lm_timer( -1 )
        , m_is_in_active_mode( false )
        , m_curr_play_lm_idx( 0 )
    {
        init_status_bridge();
    };


public:
    virtual ~CoreApp()
    {};

    static CoreApp &getInstance()
    {
        static CoreApp instance;
        return instance;
    };


    // interface for ICubicApp
    virtual bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_POWER_ACT);
        curl_global_init( CURL_GLOBAL_ALL );
        CUtil::WriteFile( "/sys/devices/78b8000.i2c/i2c-4/4-005b/led_node", "1", 1 );

        // start joinGroup timer if need
        if( CubicCfgGetStr( CUBIC_CFG_push_group ).length() <= 0
                && CubicCfgGetStr( CUBIC_CFG_push_group_invite_gid ).length() > 0
                && CubicCfgGetStr( CUBIC_CFG_push_group_invite_token ).length() > 0
                && CubicCfgGetI( CUBIC_CFG_push_group_invite_retry_cnt ) < CUBIC_JOIN_GROUP_MAX_RETRY )
        {
            setJoinGroupTimer();
        }

        onStatChange( STAT_NOT_READY );
        updateStatus();
        setReportTrackingTimer();
        LOGD( "onInit: success !" );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_PASSIVE_ON );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
        CAsyncRun<>::async_run( report_heartbeat );
        CubicWakeupLockClear(CUBIC_WAKELOCK_ID_POWER_ACT);
        return true;
    };

    // interface for ICubicApp
    virtual void onDeInit()
    {
        LOGD( "%s onDeInit", CUBIC_THIS_APP );
        CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_PASSIVE_OFF );
        cancelReportTrackingTimer();
        return;
    };

    // interface for ICubicApp
    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMessage: %s, %d", str_src_app_name.c_str(), n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_SIM_READY:
            LOGD( "onMessage: CUBIC_MSG_SIM_READY" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_SIM_OFF );
            break;

        case CUBIC_MSG_SIM_ERROR:
            LOGD( "onMessage: CUBIC_MSG_SIM_ERROR" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_SIM_ON );
            break;

        case CUBIC_MSG_NETWORK_CONNECTED:
            LOGD( "onMessage: CUBIC_MSG_NETWORK_CONNECTED" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_NET_OFF );
            input( EVT_NETWORK_CONNECTED );
            break;

        case CUBIC_MSG_NETWORK_LOST:
            LOGD( "onMessage: CUBIC_MSG_NETWORK_LOST" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_NET_ON );
            input( EVT_NETWORK_LOST );
            break;

        case CUBIC_MSG_REGISTER_SUCCESS:
            LOGD( "onMessage: CUBIC_MSG_REGISTER_SUCCESS" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_SIP_OFF );
            input( EVT_REGISTER_SUCCESS );
            break;

        case CUBIC_MSG_REGISTER_LOST:
            LOGD( "onMessage: CUBIC_MSG_REGISTER_LOST" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_SIP_ON );
            input( EVT_REGISTER_LOST );
            break;

        case CUBIC_MSG_GPS_RENEW:
            LOGD( "onMessage: CUBIC_MSG_GPS_RENEW" );
            break;

        case CUBIC_MSG_GPS_FENCE_EVT:
            LOGD( "onMessage: CUBIC_MSG_GPS_FENCE_EVT" );
            {
                report_gps_fence_t *data = ( report_gps_fence_t * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_GPS_FENCE_EVT argument missing !" );
                CAsyncRun<report_gps_fence_t>::async_run( report_gps_fence, ( *data ) );
            }
            break;

        case CUBIC_MSG_GPS_LOST:
            LOGD( "onMessage: CUBIC_MSG_GPS_LOST" );
            break;

        case CUBIC_MSG_CALL_RECEIVED:
            LOGD( "onMessage: CUBIC_MSG_CALL_RECEIVED" );
            BREAKIF( ERR_NO_ERROR != input( EVT_CALL_RECEIVED ) );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_ON );
            break;

        case CUBIC_MSG_CALL_ACCEPTED:
            LOGD( "onMessage: CUBIC_MSG_CALL_ACCEPTED" );
            BREAKIF( ERR_NO_ERROR != input( EVT_CALL_ACCEPTED ) );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_ON );
            break;

        case CUBIC_MSG_CALL_HANGUPED:
            LOGD( "onMessage: CUBIC_MSG_CALL_HANGUPED" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_OFF );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_OFF );
            input( EVT_CALL_HANGUPED );
            break;

        case CUBIC_MSG_KEEP_TRACKING:
            LOGD( "onMessage: CUBIC_MSG_KEEP_TRACKING" );
            setActiveTimer();
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ACTIVE_ON );

            if( !m_is_in_active_mode )
            {
                m_is_in_active_mode = true;
                setReportTrackingTimer();
            }

            break;

        case CUBIC_MSG_FLASH_NEW_FIRMWARE_DOWNLOAD_LIGHT:
            LOGD( "onMessage: CUBIC_MSG_FLASH_NEW_FIRMWARE_DOWNLOAD_LIGHT" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_DOWNLOAD_FINISH_ON );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            break;

        case CUBIC_MSG_FLASH_OPERATION_FAIL_LIGHT:
            LOGD( "onMessage: CUBIC_MSG_FLASH_OPERATION_FAIL_LIGHT" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_OTA_ON );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            setOTAErrFlashTimer();
            break;

        case CUBIC_MSG_BATTERY_LOW:
            LOGD( "onMessage: CUBIC_MSG_BATTERY_LOW" );
            CAsyncRun<>::async_run( report_battery );
            break;

        case CUBIC_MSG_BATTERY_OUT:
            LOGD( "onMessage: CUBIC_MSG_BATTERY_OUT" );
            LOGD( "CUBIC_MSG_BATTERY_OUT shutdown by battery out!" );
            report_battery(); // sync run report battery
            shutdownSystem();
            break;

        case CUBIC_MSG_CHARGER_IN:
            LOGD( "onMessage: CUBIC_MSG_CHARGER_IN" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            break;

        case CUBIC_MSG_CHARGER_FULL:
            LOGD( "onMessage: CUBIC_MSG_CHARGER_FULL" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            break;

        case CUBIC_MSG_CHARGER_OUT:
            LOGD( "onMessage: CUBIC_MSG_CHARGER_OUT" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            break;

        case CUBIC_MSG_JOIN_GROUP:
            LOGD( "onMessage: CUBIC_MSG_JOIN_GROUP" );
            {
                cubic_msg_join_group *data = ( cubic_msg_join_group * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_JOIN_GROUP argument missing !" );
                // setup info
                CubicCfgSet( CUBIC_CFG_push_group_invite_gid,        ( string )data->group_uuid );
                CubicCfgSet( CUBIC_CFG_push_group_invite_token,      ( string )data->token );
                CubicCfgSet( CUBIC_CFG_push_group_invite_retry_cnt,  0 );
                joinGroup();
            }
            break;

        case CUBIC_MSG_TEST_POWER_OFF:
            LOGD( "onMessage: CUBIC_MSG_TEST_POWER_OFF" );
            LOGD( "shutdown system by test command" );
            shutdownSystem();
            break;

        case CUBIC_MSG_TEST_RESET_FACTORY:
            LOGD( "onMessage: CUBIC_MSG_TEST_RESET_FACTORY" );

            if( current() > STAT_READY )
            {
                LOGD( "reject reset, in busy status:%d", current() );
            }

            LOGD( "reset to factory by test command" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_RESTORING_ON );
            LOGD( "clear user setting ...." );
            CUtil::removeDir( CUBIC_CONFIG_PATH );
            LOGD( "clear setting done !" );
            shutdownSystem( true );
            break;

        case CUBIC_MSG_TEST_PLAY_SOUND:
            LOGD( "onMessage: CUBIC_MSG_TEST_PLAY_SOUND" );
            {
                cubic_msg_test_play_sound *data = ( cubic_msg_test_play_sound * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_TEST_PLAY_SOUND argument missing !" );

                if( strcmp( data->file_path, "off" ) == 0 )
                {
                    stopPlay();
                }
                else
                {
                    playAudio( data->file_path );
                }
            }
            break;

        case CUBIC_MSG_TEST_RECORD_SOUND:
            LOGD( "onMessage: CUBIC_MSG_TEST_RECORD_SOUND" );
            {
                cubic_msg_test_play_sound *data = ( cubic_msg_test_play_sound * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_TEST_RECORD_SOUND argument missing !" );

                if( strcmp( data->file_path, "off" ) == 0 )
                {
                    stopRecord();
                }
                else
                {
                    recordAudio( data->file_path );
                }
            }
            break;

        case CUBIC_MSG_TEST_HELP:
            LOGD( "onMessage: CUBIC_MSG_TEST_HELP" );
            CRemoteReport::reportHelp();
            break;

        case CUBIC_MSG_TEST_ENV:
            LOGD( "onMessage: CUBIC_MSG_TEST_ENV" );
            CRemoteReport::reportEnvironment( 1, 2, 3 );
            break;

        case CUBIC_MSG_TEST_BAT:
            LOGD( "onMessage: CUBIC_MSG_TEST_BAT" );
            CRemoteReport::reportBattery();
            break;

        case CUBIC_MSG_TEST_SHOCK:
            LOGD( "onMessage: CUBIC_MSG_TEST_SHOCK" );
            CRemoteReport::reportShock( 1, 2, 3 );
            break;

        case CUBIC_MSG_TEST_TRACK:
            LOGD( "onMessage: CUBIC_MSG_TEST_TRACK" );
            CRemoteReport::reportLocation();
            break;

        case CUBIC_MSG_FLASH_BT_LIGHT:
            LOGD( "onMessage: CUBIC_MSG_FLASH_BT_LIGHT" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_BT_ON );
            setBTFlashTimer();
            setBTActiveTimer();
            break;

        case CUBIC_MSG_BT_ON:
            LOGD( "onMessage: CUBIC_MSG_BT_ON" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_BT_OFF );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            setBTActiveTimer();
            break;

        case CUBIC_MSG_BT_OFF:
            LOGD( "onMessage: CUBIC_MSG_BT_OFF" );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_BT_OFF );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_UPDATE );
            cancelBTActiveTimer();
            break;

        case CUBIC_MSG_NEW_FIRMWARE_NOTIFY:
        {
            LOGD( "onMessage: CUBIC_MSG_UPGRADE_FIRMWARE" );
            report_new_firmware_t *data = ( report_new_firmware_t * )p_data;
            BREAKIF_LOGE( data == NULL, "CUBIC_MSG_UPGRADE_FIRMWARE argument missing !" );
            CAsyncRun<report_new_firmware_t>::async_run( report_new_firmware, ( *data ) );
        }
        break;

        case CUBIC_MSG_TEST_DIAL:
            LOGD( "onMessage: CUBIC_MSG_TEST_DIAL" );
            {
                cubic_msg_test_dial *data = ( cubic_msg_test_dial * )p_data;
                input( EVT_DIAL_TEST, data->number );
            }
            break;

        case CUBIC_MSG_EVT_SPK_IN:
            LOGD( "onMessage: CUBIC_MSG_EVT_SPK_IN" );
            CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_ROUTE_PLAY_EARPHONE );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            break;

        case CUBIC_MSG_EVT_SPK_OUT:
            LOGD( "onMessage: CUBIC_MSG_EVT_SPK_OUT" );
            CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_ROUTE_PLAY_SPEAKER );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            break;

        case CUBIC_MSG_EVT_MIC_IN:
            LOGD( "onMessage: CUBIC_MSG_EVT_MIC_IN" );
            CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_ROUTE_REC_EARPHONE );
            break;

        case CUBIC_MSG_EVT_MIC_OUT:
            LOGD( "onMessage: CUBIC_MSG_EVT_MIC_OUT" );
            CubicPost( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_ROUTE_REC_MIC );
            break;

        case CUBIC_MSG_LAST_UNREAD_VM:
            LOGD( "onMessage: CUBIC_MSG_LAST_UNREAD_VM" );
            {
                evt_vm_play *data = ( evt_vm_play * )p_data;
                input( EVT_VM_PLAY, data );
            }
            break;

        case CUBIC_MSG_LM:
            LOGD( "onMessage: CUBIC_MSG_LM" );
            {
                evt_lm_play *data = ( evt_lm_play * )p_data;
                input( EVT_LM_PLAY, data );
            }
            break;

        case CUBIC_MSG_PLAY_DONE:
            LOGD( "onMessage: CUBIC_MSG_PLAY_DONE" );
            {
                evt_vm_play_end *data = ( evt_vm_play_end * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_PLAY_DONE argument missing !" );
                input( EVT_VM_PLAY_END, data );
            }
            break;

        case CUBIC_MSG_RECORD_DONE:
            LOGD( "onMessage: CUBIC_MSG_RECORD_DONE" );
            {
                evt_vm_record_end *data = ( evt_vm_record_end * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_PLAY_DONE argument missing !" );
                input( EVT_VM_RECORD_END, data );
            }
            break;

        case CUBIC_MSG_TEST_MIC:
            LOGD( "onMessage: CUBIC_MSG_TEST_MIC" );
            input( EVT_TEST_MIC );
            break;

        default:

            // handle key event
            if( n_msg_id >= CUBIC_MSG_EVT_KEY_FIRST &&
                    n_msg_id < CUBIC_MSG_EVT_KEY_END )
            {
                int evt = n_msg_id - CUBIC_MSG_EVT_KEY_FIRST;
                LOGD( "onMessage: key event %d", evt );
                return input( evt + EVT_KEY_FIRST );
            }

            // handle internal event
            if( n_msg_id >= CUBIC_MSG_APP_PRIVATE )
            {
                int evt = n_msg_id - CUBIC_MSG_APP_PRIVATE;
                LOGD( "onMessage: private event %d", evt );
                return input( evt );
            }

            break;
        }

        return 0;
    };


    // interface for ITimer
    virtual void onTimer( int n_timer_id )
    {
        LOGD( "onTimer: %d", n_timer_id );

        if( n_timer_id == m_record_vm_timer )
        {
            asyncInput( EVT_VM_RECORD_TIMEOUT );
        }

        if( n_timer_id == m_call_ring_timer )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_OFF );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_OFF );
            asyncInput( EVT_CALL_RING_TIMEOUT );
        }

        if( n_timer_id == m_call_dial_timer )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_OFF );
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_OFF );
            asyncInput( EVT_CALL_DIAL_TIMEOUT );
        }

        if( n_timer_id == m_activate_timer )
        {
            activate_register_and_poll();
        }

        if( n_timer_id == m_active_timer )
        {
            m_is_in_active_mode = false;
            setReportTrackingTimer();
            cancelActiveTimer();
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ACTIVE_OFF );
        }

        if( n_timer_id == m_join_group_timer )
        {
            joinGroup();
        }

        if( n_timer_id == m_bt_start_timer )
        {
            LOGD( "start bt" );
            CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_ENABLE );
        }

        if( n_timer_id == m_bt_flash_timer )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_BT_OFF );
        }

        if( n_timer_id == m_ota_err_flash_timer )
        {
            CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_ERROR_OTA_OFF );
        }

        if( n_timer_id == m_bt_active_timer )
        {
            LOGD( "bt active time out, disable it" );
            CubicPost( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_DISABLE );
        }

        if( n_timer_id == m_lm_timer )
        {
            asyncInput( EVT_LM_TIMEOUT );
        }
    };

    // interface for IAbsTimer
    virtual void onAbsTimer( int n_timer_id )
    {
        LOGD( "onAbsTimer: %d", n_timer_id );
        if( n_timer_id == m_report_tracking_timer )
        {
            // timer callback is running in standalone thread, just do report here
            CubicWakeupLockSet(CUBIC_WAKELOCK_ID_REPORT_TRACK);
            report_tracking();
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_REPORT_TRACK);
        }

        if( n_timer_id == m_report_heartbeat_timer )
        {
            // timer callback is running in standalone thread, just do report here
            CubicWakeupLockSet(CUBIC_WAKELOCK_ID_REPORT_TRACK);
            report_heartbeat();
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_REPORT_TRACK);
        }
    }

    // interface for IStatListener
    virtual void onStatChange( TStat stat )
    {
        LOGD( "onStatChange ======> %d", stat );
        CubicStatSet( CUBIC_STAT_core_stat, CStringTool::toString( ( int )stat ) );
    };
};


// IMPLEMENT_CUBIC_APP(CoreApp)
static ICubicApp *cubic_get_app_instance()
{
    return &CoreApp::getInstance();
};
static const char *cubic_get_app_name()
{
    return "CoreApp";
};



