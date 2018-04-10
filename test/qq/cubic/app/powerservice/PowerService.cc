/**
 * @file PowerService.cc
 * @author gang.tian
 * @version 1.0
 * @brief Demo App,test the cubic framework function
 * @detail Demo App,test function and the demo application
 */

#include "CFramework.cc"
#include "CLock.cc"
#include "cubic_inc.h"
#include <iostream>
#include <map>
#include "errno.h"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "PowerService"

using namespace std;

#define CUBIC_SLEEP_SPAN            1000 //ms

class PowerService : public ICubicApp, public ITimer
{
private:
    static const int  PRIVATE_MSG_RESET_TIMER = CUBIC_MSG_APP_PRIVATE + 1;
    CLock             m_lock;
    map<string, int>   m_map;
    int               m_sleep_timer;

    void logout_map()
    {
        for( map<string, int>::iterator i = m_map.begin();
                i != m_map.end(); i++ )
        {
            LOGD( "PowerService, map: %s ==> %d", i->first.c_str(), i->second );
        }
    }

    void cancelSleepTimer()
    {
        RETIF( m_sleep_timer < 0 );
        LOGD( "cancelSleepTimer %d", m_sleep_timer );
        CubicKillTimer( m_sleep_timer );
        m_sleep_timer = -1;
        LOGD( "cancelSleepTimer ed" );
    };

    void setSleepTimer()
    {
        RETIF( CubicCfgGetStr( CUBIC_CFG_pwr_sleep_mode ) == "none" );
        cancelSleepTimer();
        m_sleep_timer = CubicSetTimer( CUBIC_SLEEP_SPAN, this );
        LOGD( "setSleepTimer id=%d", m_sleep_timer );
    };

    void setWakeLock( const string &key )
    {
        CLock::Auto lock( m_lock );
        LOGD("setWakeLock, %s when %d", key.c_str(), m_map.size() );
        m_map[ key ] = 1;
    };

    void clearWakeLock( const string &key )
    {
        CLock::Auto lock( m_lock );
        LOGD("clearWakeLock, %s when %d", key.c_str(), m_map.size() );
        std::map<string, int>::iterator it = m_map.find( key );
        RETIF( it == m_map.end() );
        m_map.erase( it );
    };

    void tryToSleep()
    {
        CLock::Auto lock( m_lock );

        if( m_map.size() > 0 )
        {
            logout_map();
            resumeFromSleep();
            return;
        }

        string cmd = "rtcwake -m mem -s ";
        cmd += CubicCfgGetStr(CUBIC_CFG_pwr_sleep_period);
        system( cmd.c_str() );
        //CUtil::WriteFile( "/sys/power/state", "mem\n", 4 );
        CubicPost( CUBIC_THIS_APP, PRIVATE_MSG_RESET_TIMER );
    };

    void prepareToSleep()
    {
        LOGD("prepareToSleep");
        CubicPost(CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_SLEEP_ON);
        CubicPost( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_HOT_STOP );
    };

    void resumeFromSleep()
    {
        LOGD("resumeFromSleep");
        CubicPost(CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_STAT_SLEEP_OFF);
        CubicPost( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_HOT_START );
    };

public:
    PowerService()
        : m_lock()
        , m_map()
        , m_sleep_timer(0)
    {};

    ~PowerService()
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
        case CUBIC_MSG_POWER_WAKELOCK_SET:
            LOGD( "onMessage, CUBIC_MSG_POWER_WAKELOCK_SET" );
            {
                cubic_msg_power_wakelock_set *data = (cubic_msg_power_wakelock_set *)p_data;
                BREAKIF_LOGE( data == NULL, "data argument is empty" );
                setWakeLock( data->func_id );
                cancelSleepTimer();
            }
            break;

        case CUBIC_MSG_POWER_WAKELOCK_CLEAR:
            LOGD( "onMessage, CUBIC_MSG_POWER_WAKELOCK_CLEAR" );
            {
                cubic_msg_power_wakelock_clear *data = (cubic_msg_power_wakelock_clear *)p_data;
                BREAKIF_LOGE( data == NULL, "data argument is empty" );
                clearWakeLock( data->func_id );
                m_lock.lock();
                if( m_map.size() <= 0 )
                {
                    prepareToSleep();
                    setSleepTimer();
                }
                else
                {
                    logout_map();
                }
                m_lock.unlock();
            }
            break;

        case PRIVATE_MSG_RESET_TIMER:
            LOGD( "onMessage, PRIVATE_MSG_RESET_TIMER" );
            setSleepTimer();
            break;

        default:
            break;
        }
        return 0;
    };

    // interface for ITimer
    virtual void onTimer( int n_timer_id )
    {
        LOGD( "onTimer: %d", n_timer_id );
        if( n_timer_id == m_sleep_timer )
        {
            tryToSleep();
        }
    }
};

IMPLEMENT_CUBIC_APP( PowerService )

