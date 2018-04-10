/**
 * @file EventListener.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief listener for every event, Ex. key board, USB etc.
 * @detail listener for every event, Ex. key board, USB etc.
 */
#ifndef _EVENT_LISTENER_CC_
#define _EVENT_LISTENER_CC_ 1

#include "cubic_inc.h"
#include "CThread.cc"
#include "CForecast.cc"
#include "CTimer.cc"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <vector>

#include <linux/input.h>

using namespace std;


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "EventListener"

#define E3_EARPHONE_SUPPORT 1

static const int CUBIC_EVENT_DEVICE_MAX = 16;
static const char *CUBIC_EVENT_DEVICE[CUBIC_EVENT_DEVICE_MAX] =
{
    "/dev/input/event0",
    "/dev/input/event1",
    "/dev/input/event2",
    "/dev/input/event3",
    "/dev/input/event4",
    "/dev/input/event5",
    "/dev/input/event6",
    "/dev/input/event7",
    NULL,
};

static const int CUBIC_EVENT_FACTOR_SHOCK = 2048; // to g, depend on acc_range
static const int CUBIC_EVENT_FACTOR_TEMPERATURE = 100; // 0.01 to Celsius degree
static const int CUBIC_EVENT_FACTOR_PRESSURE = 1000; // pa to kpa
static const int CUBIC_EVENT_FACTOR_HUMIDITY = 1024;  // %RH

#define CUBIC_KEY_PRESSURE      MSC_RAW
#define CUBIC_KEY_HUMIDITY      MSC_SCAN
#define CUBIC_KEY_TEMPERAtURE   MSC_GESTURE

#define CUBIC_KEY_EARPHONE_SPK  0x07
#define CUBIC_KEY_EARPHONE_MIC  0x04

#if E3_EARPHONE_SUPPORT
#define E3_EARPHONE_PTT_KEY_CODE 250
#define E3_EARPHONE_PTT_TIMEOUT 1000 // ms
#endif //E3_EARPHONE_SUPPORT


static const CForecast::Point S_CUBIC_TEMP_REFS_TAB[] =
{
    { 7.131, 0},
    {17.129, 10},
    {30.229, 20},
    {44.791, 30},
    {59.081, 40},
    {68.911, 50},
    {76.981, 60},
};

class IEventListener
{
public:
    typedef enum EventType
    {
        EVT_TYPE_KEY_PRESS = 1,
        EVT_TYPE_KEY_CLICK,
        EVT_TYPE_KEY_DOUBLE_CLICK,
        EVT_TYPE_KEY_TRIPLE_CLICK,
        EVT_TYPE_KEY_LONG_PRESS,
        EVT_TYPE_KEY_RELEASE,
    } EventType;

    typedef enum EarphoneEventType
    {
        EARPHONE_SPK_IN = 1,
        EARPHONE_SPK_OUT,
        EARPHONE_MIC_IN,
        EARPHONE_MIC_OUT,
    } EarphoneEventType;

    virtual void onWakeEvt() = 0;
    virtual void onEvent( int code, EventType evt ) = 0;
    virtual void onShock( float x, float y, float z ) = 0;
    virtual void onEnvironment( float temperature, float pressure, float humidity ) = 0;
    virtual void onEarphoneEvent( EarphoneEventType evt ) = 0;
};


class EventListener       : public CThread
#if E3_EARPHONE_SUPPORT
    , ITimer
#endif //E3_EARPHONE_SUPPORT
{
private:
    static const int KEY_CODE_MAX   =  255;
    static const int KEY_EVENT_STEP  =  100000; // us
    static const unsigned int TIMEOUT_SEC = KEY_EVENT_STEP / 1000000;
    static const unsigned int TIMEOUT_USEC = KEY_EVENT_STEP % 1000000;


    typedef enum KeyStatE
    {
        KEY_STAT_OFF,
        KEY_STAT_PRESSED,
        KEY_STAT_PRESSED_HOLD,
        KEY_STAT_PRESSED_LONG,
    } KeyStat;

    typedef struct KeyAttrT
    {
        KeyStat          stat;
        unsigned int     time;
        unsigned int     click_count;
        unsigned int     last_click_time;
    } KeyAttr;

    unsigned int    m_hold_press_time; // us
    unsigned int    m_long_press_time; // us

    int             m_event_fds[CUBIC_EVENT_DEVICE_MAX];
    unsigned int    m_last_time;
    unsigned int    m_current_time;
    KeyAttr         m_key_attr[KEY_CODE_MAX];
    IEventListener *m_listener;

    int             m_shock_last_x;
    int             m_shock_last_y;
    int             m_shock_last_z;
    int             m_shock_thrshold_x;
    int             m_shock_thrshold_y;
    int             m_shock_thrshold_z;

    float           m_env_last_temperature;
    float           m_env_last_pressure;
    float           m_env_last_humidity;
    float           m_env_thrshold_temperature_high;
    float           m_env_thrshold_temperature_low;
    float           m_env_thrshold_pressure_high;
    float           m_env_thrshold_pressure_low;
    float           m_env_thrshold_humidity_high;
    float           m_env_thrshold_humidity_low;

    CForecast       m_env_forecast_temperature;

#if E3_EARPHONE_SUPPORT
    uint32_t        m_spk_last_remove;
    uint32_t        m_mic_last_remove;
    int             m_spk_evt_timer;
    int             m_spk_evt_value;

    void cancelSpeakerEvtTimer()
    {
        RETIF( m_spk_evt_timer < 0 );
        LOGD( "cancelSpeakerEvtTimer" );
        CubicKillTimer( m_spk_evt_timer );
        m_spk_evt_timer = -1;
    };

    void setSpeakerEvtTimer( int value )
    {
        cancelSpeakerEvtTimer();
        m_spk_evt_value = value;
        m_spk_evt_timer = CubicSetTimer( E3_EARPHONE_PTT_TIMEOUT, this );
        LOGD( "setSpeakerEvtTimer id=%d", m_spk_evt_timer );
    };

public:
    virtual void onTimer( int n_timer_id )
    {
        if( m_listener != NULL && n_timer_id == m_spk_evt_timer )
        {
            m_listener->onEarphoneEvent( ( m_spk_evt_value == 0 ) ? IEventListener::EARPHONE_SPK_OUT : IEventListener::EARPHONE_SPK_IN );
        }
    }
private:
#endif //E3_EARPHONE_SUPPORT

    inline void pushEvt( int code, IEventListener::EventType evt )
    {
        LOGD( "pushEvt %d  ==>  %d", code, evt );

        if( m_listener != NULL )
        {
            m_listener->onEvent( code, evt );
        }
    };

    void timePass( int us )
    {
        for( int i = 0; i < KEY_CODE_MAX; i++ )
        {
            if( m_key_attr[i].stat != KEY_STAT_OFF )
            {
                m_key_attr[i].time += us;

                if( m_key_attr[i].stat < KEY_STAT_PRESSED_HOLD && m_key_attr[i].time > m_hold_press_time )
                {
                    m_key_attr[i].stat = KEY_STAT_PRESSED_HOLD;
                    pushEvt( i, IEventListener::EVT_TYPE_KEY_PRESS );
                }

                if( m_key_attr[i].stat < KEY_STAT_PRESSED_LONG && m_key_attr[i].time > m_long_press_time )
                {
                    m_key_attr[i].stat = KEY_STAT_PRESSED_LONG;
                    pushEvt( i, IEventListener::EVT_TYPE_KEY_LONG_PRESS );
                }
            }
        }
    };

    void processKeyEvent( const struct input_event &evt )
    {
        int key_code = evt.code & KEY_CODE_MAX;
        LOGD( "processKeyEvent read event type[%d] code[%d] value[%d], time:%ld.%ld",
              evt.type, evt.code, evt.value, evt.time.tv_sec, evt.time.tv_usec );

        switch( evt.value )
        {
        case 1: // KEY_DOWN
            m_listener->onWakeEvt();
            if( m_key_attr[key_code].stat == KEY_STAT_OFF )
            {
                m_key_attr[key_code].time = 0;
                m_key_attr[key_code].stat = KEY_STAT_PRESSED;
            }

            break;

        case 0: // KEY_UP
        default:
            m_listener->onWakeEvt();
            switch( m_key_attr[key_code].stat )
            {
            case KEY_STAT_PRESSED:
                m_key_attr[key_code].time = 0;
                m_key_attr[key_code].stat = KEY_STAT_OFF;

                if( m_current_time - m_key_attr[key_code].last_click_time < ( m_hold_press_time + m_hold_press_time ) )
                {
                    m_key_attr[key_code].click_count ++;
                }
                else
                {
                    m_key_attr[key_code].click_count = 1;
                }

                switch( m_key_attr[key_code].click_count )
                {
                case 3:
                    pushEvt( key_code, IEventListener::EVT_TYPE_KEY_TRIPLE_CLICK );
                    break;

                case 2:
                    pushEvt( key_code, IEventListener::EVT_TYPE_KEY_DOUBLE_CLICK );
                    break;

                case 1:
                default:
                    pushEvt( key_code, IEventListener::EVT_TYPE_KEY_CLICK );
                    break;
                }

                m_key_attr[key_code].last_click_time = m_current_time;
                break;

            default:
                m_key_attr[key_code].time = 0;
                m_key_attr[key_code].stat = KEY_STAT_OFF;
                pushEvt( key_code, IEventListener::EVT_TYPE_KEY_RELEASE );
                m_key_attr[key_code].click_count = 0;
                m_key_attr[key_code].last_click_time = 0;
                break;
            }

            break;
        };
    };

    void processEnvEvent( const struct input_event &evt )
    {
        float *last_val = NULL;
        float *threshold_high = NULL;
        float *threshold_low = NULL;

        switch( evt.code )
        {
        case CUBIC_KEY_PRESSURE:
            m_env_last_pressure = ( float )evt.value / CUBIC_EVENT_FACTOR_PRESSURE;
            last_val = &m_env_last_pressure;
            threshold_high = &m_env_thrshold_pressure_high;
            threshold_low = &m_env_thrshold_pressure_low;
            CubicStatSet( CUBIC_STAT_env_pressure, m_env_last_pressure );
            break;

        case CUBIC_KEY_HUMIDITY:
            m_env_last_humidity = ( float )evt.value / CUBIC_EVENT_FACTOR_HUMIDITY;
            last_val = &m_env_last_humidity;
            threshold_high = &m_env_thrshold_humidity_high;
            threshold_low = &m_env_thrshold_humidity_low;
            CubicStatSet( CUBIC_STAT_env_humidity, m_env_last_humidity );
            break;

        case CUBIC_KEY_TEMPERAtURE:
            m_env_last_temperature = m_env_forecast_temperature[ ( float )evt.value / CUBIC_EVENT_FACTOR_TEMPERATURE ];
            last_val = &m_env_last_temperature;
            threshold_high = &m_env_thrshold_temperature_high;
            threshold_low = &m_env_thrshold_temperature_low;
            CubicStatSet( CUBIC_STAT_env_temperature, m_env_last_temperature );
            break;

        default:
            return;
        }

        if( *threshold_high != 0.0 &&
                ( *last_val > *threshold_high || *last_val < *threshold_low ) &&
                m_listener )
        {
            m_listener->onWakeEvt();
            m_listener->onEnvironment(
                m_env_last_temperature,
                m_env_last_pressure,
                m_env_last_humidity );
        }
    };

    void processShockEvent( const struct input_event &evt )
    {
        int *last_val = NULL;
        int *threshold_val = NULL;

        switch( evt.code )
        {
        case REL_X:
            last_val = &m_shock_last_x;
            threshold_val = &m_shock_thrshold_x;
            CubicStatSet(
                CUBIC_STAT_accel_x,
                ( float )m_shock_last_x / CUBIC_EVENT_FACTOR_SHOCK
            );
            break;

        case REL_Y:
            last_val = &m_shock_last_y;
            threshold_val = &m_shock_thrshold_y;
            CubicStatSet(
                CUBIC_STAT_accel_y,
                ( float )m_shock_last_y / CUBIC_EVENT_FACTOR_SHOCK
            );
            break;

        case REL_Z:
            last_val = &m_shock_last_z;
            threshold_val = &m_shock_thrshold_z;
            CubicStatSet(
                CUBIC_STAT_accel_z,
                ( float )m_shock_last_z / CUBIC_EVENT_FACTOR_SHOCK
            );
            break;

        default:
            return;
        }

        int val = evt.value;

        if( val < 0 )
        {
            val = 0 - val;
        }

        *last_val = evt.value;

        if( *threshold_val != 0.0 && val > *threshold_val && m_listener )
        {
            m_listener->onWakeEvt();
            m_listener->onShock(
                ( float )m_shock_last_x / CUBIC_EVENT_FACTOR_SHOCK,
                ( float )m_shock_last_y / CUBIC_EVENT_FACTOR_SHOCK,
                ( float )m_shock_last_z / CUBIC_EVENT_FACTOR_SHOCK );
        }
    };


    void processHeadsetEvent( const struct input_event &evt )
    {
        switch( evt.code )
        {
        case CUBIC_KEY_EARPHONE_SPK:
            BREAKIF( m_listener == NULL );
            m_listener->onWakeEvt();
#if E3_EARPHONE_SUPPORT

            if( evt.value == 0 )
            {
                m_spk_last_remove = ( evt.time.tv_sec * 1000 ) + ( evt.time.tv_usec / 1000 );
            }

            setSpeakerEvtTimer( evt.value );
#else //E3_EARPHONE_SUPPORT
            m_listener->onEarphoneEvent( ( evt.value == 0 ) ? IEventListener::EARPHONE_SPK_OUT : IEventListener::EARPHONE_SPK_IN );
#endif //E3_EARPHONE_SUPPORT
            break;

        case CUBIC_KEY_EARPHONE_MIC:
            BREAKIF( m_listener == NULL );
            m_listener->onWakeEvt();
#if E3_EARPHONE_SUPPORT

            if( evt.value == 0 )
            {
                m_mic_last_remove = ( evt.time.tv_sec * 1000 ) + ( evt.time.tv_usec / 1000 );
                pushEvt( E3_EARPHONE_PTT_KEY_CODE, IEventListener::EVT_TYPE_KEY_RELEASE );
                m_listener->onEarphoneEvent( IEventListener::EARPHONE_MIC_OUT );
            }
            else
            {
                uint32_t now = ( evt.time.tv_sec * 1000 ) + ( evt.time.tv_usec / 1000 );

                // E3 earphone PTT press
                if( now - m_mic_last_remove > E3_EARPHONE_PTT_TIMEOUT &&
                        now - m_spk_last_remove < E3_EARPHONE_PTT_TIMEOUT )
                {
                    m_listener->onEarphoneEvent( IEventListener::EARPHONE_MIC_IN );
                    usleep( 100000 );
                    pushEvt( E3_EARPHONE_PTT_KEY_CODE, IEventListener::EVT_TYPE_KEY_PRESS );
                }
            }

#else //E3_EARPHONE_SUPPORT
            m_listener->onEarphoneEvent( ( evt.value == 0 ) ? IEventListener::EARPHONE_MIC_OUT : IEventListener::EARPHONE_MIC_IN );
#endif //E3_EARPHONE_SUPPORT
            break;
        }
    };

    EventListener()
        : m_hold_press_time( 0 )
        , m_long_press_time( 0 )
        , m_event_fds()
        , m_last_time( 0 )
        , m_current_time( 0 )
        , m_key_attr()
        , m_listener( NULL )
        , m_shock_last_x( 0 )
        , m_shock_last_y( 0 )
        , m_shock_last_z( 0 )
        , m_shock_thrshold_x( 0 )
        , m_shock_thrshold_y( 0 )
        , m_shock_thrshold_z( 0 )
        , m_env_last_temperature( 0 )
        , m_env_last_pressure( 0 )
        , m_env_last_humidity( 0 )
        , m_env_thrshold_temperature_high( 0 )
        , m_env_thrshold_temperature_low( 0 )
        , m_env_thrshold_pressure_high( 0 )
        , m_env_thrshold_pressure_low( 0 )
        , m_env_thrshold_humidity_high( 0 )
        , m_env_thrshold_humidity_low( 0 )
        , m_env_forecast_temperature( S_CUBIC_TEMP_REFS_TAB, sizeof( S_CUBIC_TEMP_REFS_TAB ) / sizeof( CForecast::Point ), 0.8 )
#if E3_EARPHONE_SUPPORT
        , m_spk_last_remove( 0 )
        , m_mic_last_remove( 0 )
        , m_spk_evt_timer( -1 )
        , m_spk_evt_value( 0 )
#endif //E3_EARPHONE_SUPPORT
    {
        for( int i = 0; i < CUBIC_EVENT_DEVICE_MAX; i++ )
        {
            m_event_fds[i] = -1;
        }
    };

public:
    ~EventListener()
    {};

    virtual void onStart( void *user )
    {
        UNUSED_ARG( user );
        CubicStatSet( CUBIC_STAT_env_pressure,      0.0f );
        CubicStatSet( CUBIC_STAT_env_humidity,      0.0f );
        CubicStatSet( CUBIC_STAT_env_temperature,   0.0f );
        CubicStatSet( CUBIC_STAT_accel_x,           0.0f );
        CubicStatSet( CUBIC_STAT_accel_y,           0.0f );
        CubicStatSet( CUBIC_STAT_accel_z,           0.0f );

        for( int i = 0; i < CUBIC_EVENT_DEVICE_MAX; i++ )
        {
            BREAKIF( CUBIC_EVENT_DEVICE[i] == NULL );
            LOGD( "ListenThread start to event device: %s", CUBIC_EVENT_DEVICE[i] );
            m_event_fds[i] = open( CUBIC_EVENT_DEVICE[i], O_RDONLY );
            RETIF_LOGE( m_event_fds[i] <= 0, "open file failed, %s", strerror( errno ) );
        }
    };

    virtual void onStop( void *user )
    {
        UNUSED_ARG( user );

        for( int i = 0; i < CUBIC_EVENT_DEVICE_MAX; i++ )
        {
            CONTINUEIF( m_event_fds[i] <= 0 );
            close( m_event_fds[i] );
            m_event_fds[i] = -1;
        }
    };

    virtual RunRet run( void *user )
    {
        struct input_event evt;
        int evt_size = sizeof( evt );
        int     ret = 0;
        int     max_fd = 0;
        fd_set  fds = {0};
        struct timeval timeout = {TIMEOUT_SEC, TIMEOUT_USEC};
        // ********************************* read event ************************
        // init fds
        FD_ZERO( &fds );

        for( int i = 0; i < CUBIC_EVENT_DEVICE_MAX; i++ )
        {
            CONTINUEIF( m_event_fds[i] <= 0 );
            max_fd = MAX( m_event_fds[i], max_fd );
            FD_SET( m_event_fds[i], &fds );
        }

        // select
        ret = select( max_fd + 1, &fds, 0, 0, &timeout );
        RETNIF( ret < 0, CThread::RUN_CONTINUE );

        if( ret == 0 )
        {
            // time out
            timePass( KEY_EVENT_STEP );
            return CThread::RUN_CONTINUE;
        }

        for( int i = 0; i < CUBIC_EVENT_DEVICE_MAX; i++ )
        {
            CONTINUEIF( m_event_fds[i] <= 0 );
            CONTINUEIF( !FD_ISSET( m_event_fds[i], &fds ) );
            CONTINUEIF( read( m_event_fds[i], &evt, evt_size ) != evt_size );
            CONTINUEIF( evt.type == EV_SYN );
            /*
            LOGI( "EVENT read event type[%d] code[%d] value[%d], time:%ld.%ld",
                    evt.type, evt.code, evt.value, evt.time.tv_sec, evt.time.tv_usec );
            */
            m_current_time = evt.time.tv_sec * 1000000 + evt.time.tv_usec;

            // ******************************** handle event ************************
            if( evt.type == EV_KEY )
            {
                processKeyEvent( evt );
            }

            if( evt.type == EV_MSC )
            {
                processEnvEvent( evt );
            }

            if( evt.type == EV_REL )
            {
                processShockEvent( evt );
            }

            if( evt.type == EV_SW )
            {
                processHeadsetEvent( evt );
            }

            // calculate passed time
            CONTINUEIF( m_last_time == 0 );
            timePass( m_current_time - m_last_time );
            m_last_time = m_current_time;
        }

        return CThread::RUN_CONTINUE;
    };

    static EventListener &getInstance()
    {
        static EventListener instance;
        return instance;
    };

    void reloadConfig()
    {
        m_hold_press_time = CubicCfgGetU( CUBIC_CFG_key_press_time_hold ) * 1000;
        m_long_press_time  = CubicCfgGetU( CUBIC_CFG_key_press_time_long )  * 1000;
        m_shock_thrshold_x = CubicCfgGetF( CUBIC_CFG_env_threshold_shock_x ) * CUBIC_EVENT_FACTOR_SHOCK;
        m_shock_thrshold_y = CubicCfgGetF( CUBIC_CFG_env_threshold_shock_y ) * CUBIC_EVENT_FACTOR_SHOCK;
        m_shock_thrshold_z = CubicCfgGetF( CUBIC_CFG_env_threshold_shock_z ) * CUBIC_EVENT_FACTOR_SHOCK;
        m_env_thrshold_temperature_low  = CubicCfgGetF( CUBIC_CFG_env_threshold_temperature_low );
        m_env_thrshold_temperature_high = CubicCfgGetF( CUBIC_CFG_env_threshold_temperature_high );
        m_env_thrshold_pressure_low  = CubicCfgGetF( CUBIC_CFG_env_threshold_pressure_low );
        m_env_thrshold_pressure_high = CubicCfgGetF( CUBIC_CFG_env_threshold_pressure_high );
        m_env_thrshold_humidity_low  = CubicCfgGetF( CUBIC_CFG_env_threshold_humidity_low );
        m_env_thrshold_humidity_high = CubicCfgGetF( CUBIC_CFG_env_threshold_humidity_high );
    };

    int start( IEventListener *listener )
    {
        LOGD( "EventListener start" );
        RETNIF_LOGE( this->isRunning(), 1, "is already running !" );
        reloadConfig();
        m_listener = listener;
        RETNIF_LOGE( !CThread::start(), -1, "EventListener failed to start" );
        return 0;
    };

    bool isPressed( int key_code )
    {
        RETNIF( key_code < 0 || key_code >= KEY_CODE_MAX, false );
        return ( m_key_attr[key_code].stat != KEY_STAT_OFF );
    };
};

#endif //_EVENT_LISTENER_CC_
