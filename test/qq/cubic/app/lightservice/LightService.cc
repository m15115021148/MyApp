/**
 * @file LightService.cc
 * @author lishujie
 * @version 1.0
 * @brief Light Service
 * @detail Light Service
 */

#include "CFramework.cc"
#include "cubic_inc.h"
#include <iostream>

#include "CAsyncRun.cc"
#include "LightControl.cc"


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "LightService"

#define CUBIC_SLEEP_FLASH_INTERVAL  10000 //ms

#define CUBIC_VIBRATE_PATH "/sys/class/leds/vibrator/brightness"


class LightService : public ICubicApp, public IAbsTimer
{
private:
    int m_sleep_flash_timer;

    void doVibrate()
    {
        LOGD( "doVibrate" );
        CUtil::WriteFile( CUBIC_VIBRATE_PATH, "1", 1 );
        usleep( 20000 );
        CUtil::WriteFile( CUBIC_VIBRATE_PATH, "0", 1 );
        LOGD( "doVibrate done" );
    };

    void cancelSleepFlashTimer()
    {
        RETIF( m_sleep_flash_timer < 0 );
        LOGD( "cancelSleepFlashTimer" );
        CubicKillAbsTimer( m_sleep_flash_timer );
        m_sleep_flash_timer = -1;
    };

    void setSleepFlashTimer()
    {
        RETIF( CubicCfgGetStr( CUBIC_CFG_pwr_sleep_mode ) == "none" );
        cancelSleepFlashTimer();
        m_sleep_flash_timer = CubicSetAbsTimerInterval( CUBIC_SLEEP_FLASH_INTERVAL, this );
        LOGD( "setSleepFlashTimer id=%d", m_sleep_flash_timer );
    };

public:
    LightService()
        : m_sleep_flash_timer(-1)
    {};

    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        return true;
    };

    void onDeInit()
    {
        LOGD( "%s onDeInit", CUBIC_THIS_APP );
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMessage: %s, %d", str_src_app_name.c_str(), n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_LIGHT_UPDATE:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_UPDATE" );
            LightControl::getInstance().updateStatus();
            break;

        case  CUBIC_MSG_LIGHT_STAT_OFF_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_OFF_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_OFF, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_OFF_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_OFF_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_OFF, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_RESTORING_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_RESTORING_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_RESTORING, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_RESTORING_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_RESTORING_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_RESTORING, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_BOOTING_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_BOOTING_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_BOOTING, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_BOOTING_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_BOOTING_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_BOOTING, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_USR_CHK_BAT_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_USR_CHK_BAT_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_USR_CHK_BAT, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_USR_CHK_BAT_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_USR_CHK_BAT_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_USR_CHK_BAT, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_USR_CHK_NET_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_USR_CHK_NET_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_USR_CHK_NET, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_USR_CHK_NET_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_USR_CHK_NET_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_USR_CHK_NET, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_USR_CHK_GPS_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_USR_CHK_GPS_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_USR_CHK_GPS, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_USR_CHK_GPS_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_USR_CHK_GPS_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_USR_CHK_GPS, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_SOS_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_SOS_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_SOS, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_SOS_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_SOS_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_SOS, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_CALL_MUTE_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_CALL_MUTE_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_CALL_MUTE, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_CALL_MUTE_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_CALL_MUTE_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_CALL_MUTE, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_CALL_CONVERSATION, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_CALL_CONVERSATION_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_CALL_CONVERSATION, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_CALL_UNCONNECT, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_CALL_UNCONNECT_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_CALL_UNCONNECT, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_MESSAGE_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_MESSAGE_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_MESSAGE, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_MESSAGE_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_MESSAGE_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_MESSAGE, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_LEAVE_MESSAGE, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_LEAVE_MESSAGE_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_LEAVE_MESSAGE, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_BT_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_BT_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_BT, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_BT_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_BT_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_BT, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_SIM_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_SIM_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_SIM, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_SIM_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_SIM_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_SIM, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_NET_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_NET_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_NET, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_NET_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_NET_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_NET, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_ACTIVATE_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_ACTIVATE_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_ACTIVATE, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_ACTIVATE_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_ACTIVATE_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_ACTIVATE, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_SIP_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_SIP_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_SIP, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ERROR_SIP_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_SIP_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_SIP, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_UPDATE_READY_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_UPDATE_READY_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_UPDATE_READY, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_UPDATE_READY_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_UPDATE_READY_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_UPDATE_READY, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_UPDATING_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_UPDATING_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_UPDATING, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_UPDATING_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_UPDATING_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_UPDATING, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ACTIVE_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ACTIVE_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ACTIVE, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_ACTIVE_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ACTIVE_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ACTIVE, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_PASSIVE_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_PASSIVE_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_PASSIVE, true );
            break;

        case  CUBIC_MSG_LIGHT_STAT_PASSIVE_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_PASSIVE_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_PASSIVE, false );
            break;

        case CUBIC_MSG_LIGHT_DO_VIBRATE:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_DO_VIBRATE" );
            doVibrate();
            break;

        case CUBIC_MSG_LIGHT_STAT_DOWNLOAD_FINISH_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_DOWNLOAD_FINISH_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_UPGRADE, true );
            break;

        case CUBIC_MSG_LIGHT_STAT_DOWNLOAD_FINISH_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_DOWNLOAD_FINISH_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_UPGRADE, false );
            break;

        case CUBIC_MSG_LIGHT_STAT_ERROR_OTA_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_OTA_ON" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_OTA, true );
            break;

        case CUBIC_MSG_LIGHT_STAT_ERROR_OTA_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_ERROR_OTA_OFF" );
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_ERROR_OTA, false );
            break;

        case  CUBIC_MSG_LIGHT_STAT_SLEEP_ON:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_SLEEP_ON" );
            // turn off light effect first
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_OFF, true );
            setSleepFlashTimer();
            break;

        case  CUBIC_MSG_LIGHT_STAT_SLEEP_OFF:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_STAT_SLEEP_OFF" );
            cancelSleepFlashTimer();
            // turn on light effect
            LightControl::getInstance().setStatus( LightControl::LIGHT_STAT_OFF, false );
            break;

        case CUBIC_MSG_LIGHT_TEST_LIGHT:
            LOGD( "onMessage: CUBIC_MSG_LIGHT_TEST_LIGHT" );
            {
                for( int i = LightControl::LIGHT_STAT_FIRST; i < LightControl::LIGHT_STAT_MAX; i++ )
                {
                    LOGD( "light status: id=%d, stat=%s", i, ( LightControl::getInstance().getStatus( ( LightControl::LightStatus )i ) ? "true" : "false" ) );
                    LightControl::getInstance().setStatus( ( LightControl::LightStatus )i, false, false );
                }

                cubic_msg_light_test_light *data = ( cubic_msg_light_test_light * )p_data;
                LightControl::getInstance().setStatus( ( LightControl::LightStatus )( data->light ), true );
            }
            break;

        default:
            break;
        }

        return 0;
    };

    // interface for IAbsTimer
    virtual void onAbsTimer( int n_timer_id )
    {
        LOGD( "onAbsTimer: %d", n_timer_id );
        if( n_timer_id == m_sleep_flash_timer )
        {
            CubicLightKey light_define[CUBIC_LIGHT_NUM][CUBIC_LIGHT_KEY_MAX];

            CubicWakeupLockSet(CUBIC_WAKELOCK_ID_SLEEP_FLASH);
            memset( light_define, 0, sizeof( light_define ) );
            memcpy( light_define[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_SLEEP, sizeof( CUBIC_LIGHT_EFFECT_SLEEP ) );
            memcpy( light_define[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_SLEEP, sizeof( CUBIC_LIGHT_EFFECT_SLEEP ) );
            memcpy( light_define[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_SLEEP, sizeof( CUBIC_LIGHT_EFFECT_SLEEP ) );
            memcpy( light_define[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_SLEEP, sizeof( CUBIC_LIGHT_EFFECT_SLEEP ) );
            LightControl::getInstance().runSyncLightEffect( light_define ); // end all light effect first
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_SLEEP_FLASH);
        }
    }
};

IMPLEMENT_CUBIC_APP( LightService )

