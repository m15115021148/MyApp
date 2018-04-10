/**
 * @file CoreApp.cc
 * @author gang.tian
 * @version 1.0
 * @brief Demo App,test the cubic framework function
 * @detail Demo App,test function and the demo application
 */

#include "CFramework.cc"
#include "cubic_inc.h"
#include <iostream>

#include "CAsyncRun.cc"
#include "CRemoteReport.cc"
#include "EventListener.cc"


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "EventService"


#define CUBIC_KEY_MAIN KEY_ENTER
#define CUBIC_KEY_SUB1 KEY_3
#define CUBIC_KEY_SUB2 KEY_2
#define CUBIC_KEY_SUB3 KEY_1
#define CUBIC_KEY_POWR KEY_POWER

#define CUBIC_EVENT_REPORT_RETRY    5


class EventService : public ICubicApp, public IEventListener, public ITimer
{
private:
    int m_sleep_timer;

    void cancelSleepTimer()
    {
        RETIF( m_sleep_timer < 0 );
        LOGD( "cancelSleepTimer" );
        CubicKillTimer( m_sleep_timer );
        m_sleep_timer = -1;
    };

    void setSleepTimer()
    {
        cancelSleepTimer();
        int timeout = CubicCfgGetI( CUBIC_CFG_evt_sleep_timeout );
        RETIF( timeout <= 0 );
        timeout *= 1000; // to ms
        m_sleep_timer = CubicSetTimer( timeout, this );
        LOGD( "setSleepTimer id=%d", m_sleep_timer );
    };

public:
    EventService() : m_sleep_timer(0)
    {};

    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        curl_global_init( CURL_GLOBAL_ALL );
        EventListener::getInstance().start( this );
        return true;
    };

    void onDeInit()
    {
        LOGD( "%s onDeInit", CUBIC_THIS_APP );
        EventListener::getInstance().stop();
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMessage: %s, %d", str_src_app_name.c_str(), n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_SET_THRESHOLDS:
            LOGD( "onMessage: CUBIC_MSG_SET_THRESHOLDS" );
            EventListener::getInstance().reloadConfig();
            break;

        default:
            break;
        }

        return 0;
    };


    // interface for IEventListener
    virtual void onEvent( int code, EventType evt )
    {
        LOGD( "onEvent %d, %d", code, evt );

        switch( code )
        {
        case CUBIC_KEY_MAIN:
            switch( evt )
            {
            case EVT_TYPE_KEY_PRESS:
                if( EventListener::getInstance().isPressed( CUBIC_KEY_SUB1 ) )
                {
                    LOGD( "keyEvent: EVT_KEY_PRESS_MAIN_WITH_SUB1" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_PRESS_MAIN_WITH_SUB1 );
                    break;
                }

                if( EventListener::getInstance().isPressed( CUBIC_KEY_SUB2 ) )
                {
                    LOGD( "keyEvent: EVT_KEY_PRESS_MAIN_WITH_SUB2" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_PRESS_MAIN_WITH_SUB2 );
                    break;
                }

                if( EventListener::getInstance().isPressed( CUBIC_KEY_SUB3 ) )
                {
                    LOGD( "keyEvent: EVT_KEY_PRESS_MAIN_WITH_SUB3" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_PRESS_MAIN_WITH_SUB3 );
                    break;
                }

                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_POWR ) );
                LOGD( "keyEvent: EVT_KEY_PRESS_MAIN" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_PRESS_MAIN );
                break;

            case EVT_TYPE_KEY_CLICK:
                LOGD( "keyEvent: EVT_KEY_CLICK_MAIN" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_CLICK_MAIN );
                break;

            case EVT_TYPE_KEY_DOUBLE_CLICK:
                LOGD( "keyEvent: EVT_TYPE_KEY_DOUBLE_CLICK" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_DOUBLE_CLICK_MAIN );
                break;

            case EVT_TYPE_KEY_TRIPLE_CLICK:
                LOGD( "keyEvent: EVT_TYPE_KEY_TRIPLE_CLICK" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_TRIPLE_CLICK_MAIN );
                break;

            case EVT_TYPE_KEY_LONG_PRESS:
                if( EventListener::getInstance().isPressed( CUBIC_KEY_POWR ) )
                {
                    LOGD( "keyEvent: EVT_KEY_LONGPRESS_MAIN_WITH_POWR" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_MAIN_WITH_POWR );
                    break;
                }

                LOGD( "keyEvent: EVT_KEY_LONGPRESS_MAIN" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_MAIN );
                break;

            case EVT_TYPE_KEY_RELEASE:
                LOGD( "keyEvent: EVT_KEY_RELEASE_MAIN" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_RELEASE_MAIN );
                break;

            default:
                break;
            }

            break;

        case CUBIC_KEY_SUB1:
            switch( evt )
            {
            case EVT_TYPE_KEY_CLICK:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_CLICK_SUB1" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_CLICK_SUB1 );
                break;

            case EVT_TYPE_KEY_LONG_PRESS:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );

                if( EventListener::getInstance().isPressed( CUBIC_KEY_SUB2 ) &&
                        EventListener::getInstance().isPressed( CUBIC_KEY_SUB3 ) )
                {
                    LOGD( "keyEvent: EVT_KEY_LONGPRESS_ALL_SUB_KEY" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_ALL_SUB_KEY );
                    break;
                }

                LOGD( "keyEvent: EVT_KEY_LONGPRESS_SUB1" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_SUB1 );
                break;

            case EVT_TYPE_KEY_RELEASE:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_RELEASE_SUB1" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_RELEASE_SUB1 );
                break;

            default:
                break;
            }

            break;

        case CUBIC_KEY_SUB2:
            switch( evt )
            {
            case EVT_TYPE_KEY_CLICK:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_CLICK_SUB2" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_CLICK_SUB2 );
                break;

            case EVT_TYPE_KEY_LONG_PRESS:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );

                if( EventListener::getInstance().isPressed( CUBIC_KEY_SUB1 ) &&
                        EventListener::getInstance().isPressed( CUBIC_KEY_SUB3 ) )
                {
                    LOGD( "keyEvent: EVT_KEY_LONGPRESS_ALL_SUB_KEY" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_ALL_SUB_KEY );
                    break;
                }

                LOGD( "keyEvent: EVT_KEY_LONGPRESS_SUB2" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_SUB2 );
                break;

            case EVT_TYPE_KEY_RELEASE:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_RELEASE_SUB2" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_RELEASE_SUB2 );
                break;

            default:
                break;
            }

            break;

        case CUBIC_KEY_SUB3:
            switch( evt )
            {
            case EVT_TYPE_KEY_CLICK:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_CLICK_SUB3" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_CLICK_SUB3 );
                break;

            case EVT_TYPE_KEY_LONG_PRESS:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );

                if( EventListener::getInstance().isPressed( CUBIC_KEY_SUB1 ) &&
                        EventListener::getInstance().isPressed( CUBIC_KEY_SUB2 ) )
                {
                    LOGD( "keyEvent: EVT_KEY_LONGPRESS_ALL_SUB_KEY" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_ALL_SUB_KEY );
                    break;
                }

                LOGD( "keyEvent: EVT_KEY_LONGPRESS_SUB3" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_SUB3 );
                break;

            case EVT_TYPE_KEY_RELEASE:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_RELEASE_SUB3" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_RELEASE_SUB3 );
                break;

            default:
                break;
            }

            break;

        case CUBIC_KEY_POWR:
            switch( evt )
            {
            case EVT_TYPE_KEY_LONG_PRESS:
                BREAKIF( EventListener::getInstance().isPressed( CUBIC_KEY_MAIN ) );
                LOGD( "keyEvent: EVT_KEY_LONGPRESS_POWR" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_POWR );
                break;

            default:
                break;
            }

            break;

        default:
            LOGI( "onEvent %d,%d", code, evt );

            // treat 200+ as PTT key
            if( code >= 200 )
            {
                switch( evt )
                {
                case EVT_TYPE_KEY_PRESS:
                    LOGD( "keyEvent: EVT_KEY_PRESS_PTT" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_PRESS_PTT );
                    break;

                case EVT_TYPE_KEY_CLICK:
                    LOGD( "keyEvent: EVT_KEY_CLICK_PTT" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_CLICK_PTT );
                    break;

                case EVT_TYPE_KEY_LONG_PRESS:
                    LOGD( "keyEvent: EVT_KEY_LONGPRESS_PTT" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_LONGPRESS_PTT );
                    break;

                case EVT_TYPE_KEY_RELEASE:
                    LOGD( "keyEvent: EVT_KEY_RELEASE_PTT" );
                    CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_KEY_RELEASE_PTT );
                    break;

                default:
                    break;
                }
            }

            break;
        }
    };



    typedef struct report_t
    {
        float x;
        float y;
        float z;
    } report_t;

    static void report_shock( report_t arg )
    {
        for( int i = 0; CRemoteReport::reportShock( arg.x, arg.y, arg.z ) != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };

    // interface for IEventListener
    virtual void onShock( float x, float y, float z )
    {
        LOGI( "onShock %f,%f,%f", x, y, z );
        report_t arg;;
        arg.x = x;
        arg.y = y;
        arg.z = z;
        CAsyncRun<report_t>::async_run( report_shock, arg );
    };


    static void report_environment( report_t arg )
    {
        for( int i = 0; CRemoteReport::reportEnvironment( arg.x, arg.y, arg.z ) != 0 && i < CUBIC_EVENT_REPORT_RETRY; i++ )
        {
            sleep( 1 );
        }
    };

    // interface for IEventListener
    virtual void onEnvironment( float temperature, float pressure, float humidity )
    {
        LOGI( "onEnvironment %f,%f,%f", temperature, pressure, humidity );
        report_t arg;
        arg.x = temperature;
        arg.y = pressure;
        arg.z = humidity;
        CAsyncRun<report_t>::async_run( report_environment, arg );
    };

    // interface of IEventListener
    virtual void onEarphoneEvent( EarphoneEventType evt )
    {
        LOGI( "onEarphoneEvent %d", ( int )evt );

        switch( evt )
        {
        case EARPHONE_SPK_IN:
            LOGD( "onEarphoneEvent: EARPHONE_SPK_IN" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_SPK_IN );
            break;

        case EARPHONE_SPK_OUT:
            LOGD( "onEarphoneEvent: EARPHONE_SPK_OUT" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_SPK_OUT );
            break;

        case EARPHONE_MIC_IN:
            LOGD( "onEarphoneEvent: EARPHONE_MIC_IN" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_MIC_IN );
            break;

        case EARPHONE_MIC_OUT:
            LOGD( "onEarphoneEvent: EARPHONE_MIC_OUT" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_EVT_MIC_OUT );
            break;
        }
    };

    // interface of IEventListener
    virtual void onWakeEvt()
    {
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_EVENT);
        setSleepTimer();
    };

    // interface for ITimer
    virtual void onTimer( int n_timer_id )
    {
        if( n_timer_id == m_sleep_timer )
        {
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_EVENT);
        }
    };
};

IMPLEMENT_CUBIC_APP( EventService )

