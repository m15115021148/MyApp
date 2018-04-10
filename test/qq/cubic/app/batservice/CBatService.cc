#ifndef __CBATSERIVCE_CC__
#define __CBATSERIVCE_CC__ 1

#include "CFramework.cc"
#include <iostream>

#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/un.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <linux/reboot.h>
#include <linux/input.h>
#include <syscall.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>
#include <cctype>
#include <cstring>
#include <math.h>


#include "cubic_func.h"
#include "CForecast.cc"


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "BatService"

#define TIMER_TIMEOUT          1000
#define WAIT_AND_READ_BATTERY  201

#define CUBIC_VOLTAGE_ALPHA         0.8f
#define CUBIC_VOLTAGE_COMPENSATION  0.0966f


const static int S_BAT_MAX_CAP = 1400;

const static CForecast::Point S_CHARGE_REFS_TAB [] =
{
    {3.3,  0},
    {3.5,  30},
    {3.9,  150},
    {4.0,  300},
    {4.1,  700},
    {4.15, 1000},
    {4.17, 1200},
    {4.18, 1450},
};

const static CForecast::Point S_DISCHARGE_REFS_TAB [] =
{
    {2.50, 1450},
    {3.30, 1400},
    {3.35, 1350},
    {3.45, 1200},
    {3.50, 1100},
    {3.60, 900},
    {3.80, 300},
    {3.90, 100},
    {4.10, 0},
};


using namespace std;
class BatService : public ICubicApp, public ITimer
{
private:
    typedef enum ChargeStatus
    {
        CHARGE_UNKNOWN = -1,
        NOT_CHARGING  = 0 ,
        CHARGING,
    } ChargeStatus;

    typedef enum ChargeType
    {
        TYPE_UNKNOWN = -1,
        NO_CHARGER  = 0 ,
        TYPE_USB,
        TYPE_WIRELESS,
    } ChargeType;

    typedef enum BatteryLevel
    {
        BATTERY_UNKNOWN = -2,
        BATTERY_OUT = -1,
        BATTERY_EMERGENCY,
        BATTERY_LOW,
        BATTERY_NORMAL,
        BATTERY_GOOD,
    } BatteryLevel;

    typedef struct BatFilesTab
    {
        const char *batpath;
        void ( *bathandle )( BatService *, char * );
    } BatFilesTab;


    int              m_watch_timer_id ;
    ChargeStatus     m_charge_status;
    ChargeType       m_charge_type;
    BatteryLevel     m_battery_level;
    float            m_battery_voltage;
    float            m_battery_percent;
    CForecast        m_charging_forecast;
    CForecast        m_discarge_forecast;


    void open_bat_fds()
    {
        static const int BAT_LENGTH_MAX = 256;
        static const BatFilesTab bat_dev_defs[] =
        {
            {"/sys/class/power_supply/usb/online",          handle_chg_usb_status},
            {"/sys/devices/soc:hardware_chg/get_dc_in",     handle_chg_wireless_status},
            {"/sys/class/power_supply/battery/status",      handle_bat_battery_charge_status},
            {"/sys/class/power_supply/battery/voltage_now", handle_bat_battery_voltage_now},
            {NULL, NULL}
        };
        char buf[BAT_LENGTH_MAX + 4];

        for( const BatFilesTab *dev_def = bat_dev_defs; dev_def && dev_def->batpath != NULL; dev_def++ )
        {
            memset( buf, 0, sizeof( buf ) );

            if( 0 < CUtil::ReadFile( dev_def->batpath, buf, BAT_LENGTH_MAX ) )
            {
                dev_def->bathandle( this, buf );
            }
        }
    };

    void update_charger_status()
    {
        switch( m_charge_status )
        {
        case CHARGING:
            if( m_battery_percent > 0.95 )
            {
                CubicStatSet( CUBIC_STAT_charger_status, "full" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_FULL );
                break;
            }

            CubicStatSet( CUBIC_STAT_charger_status, "on" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_IN );
            break;

        case NOT_CHARGING:
        default: // include unknown
            if( m_charge_type == TYPE_WIRELESS || m_charge_type == TYPE_USB )
            {
                CubicStatSet( CUBIC_STAT_charger_status, "full" );
                CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_FULL );
                break;
            }

            CubicStatSet( CUBIC_STAT_charger_status, "none" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_OUT );
            break;
        }
    };

    static void handle_chg_usb_status( BatService *pthis, char *buf )
    {
        LOGI( "USB status value: %s", buf );
        int usb_online = ( int )strtol( buf, NULL, 10 );
        RETIF( usb_online == 1 && TYPE_USB == pthis->m_charge_type );
        RETIF( usb_online != 1 && TYPE_USB != pthis->m_charge_type );
        LOGD( "USB status update to: %d when type:%d", usb_online, pthis->m_charge_type );

        if( usb_online )
        {
            pthis->m_charge_type = TYPE_USB;
            CubicStatSet( CUBIC_STAT_charger_type, "usb" );
            CubicWakeupLockSet(CUBIC_WAKELOCK_ID_CHARGE);
        }
        else
        {
            pthis->m_charge_type = NO_CHARGER;
            CubicStatSet( CUBIC_STAT_charger_type, "none" );
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_CHARGE);
        }

        pthis->update_charger_status();
    };

    static void handle_chg_wireless_status( BatService *pthis, char *buf )
    {
        LOGI( "wireless status value: %s", buf );
        int wireless_online = ( int )strtol( buf, NULL, 10 );
        wireless_online = !wireless_online;
        RETIF( wireless_online == 1 && TYPE_WIRELESS == pthis->m_charge_type );
        RETIF( wireless_online != 1 && TYPE_WIRELESS != pthis->m_charge_type );
        LOGD( "wireless status update to: %d when type:%d", wireless_online, pthis->m_charge_type );

        if( wireless_online )
        {
            pthis->m_charge_type = TYPE_WIRELESS;
            CubicStatSet( CUBIC_STAT_charger_type, "air" );
            CubicWakeupLockSet(CUBIC_WAKELOCK_ID_CHARGE);
        }
        else
        {
            pthis->m_charge_type = NO_CHARGER;
            CubicStatSet( CUBIC_STAT_charger_type, "none" );
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_CHARGE);
        }

        pthis->update_charger_status();
    };

    static void handle_bat_battery_charge_status( BatService *pthis, char *buf )
    {
        LOGI( "charge status value: %s", buf );
        ChargeStatus curr_stat = CHARGE_UNKNOWN;
        string str_stat( buf );
        str_stat = CStringTool::trim( CStringTool::toLower( str_stat ) );

        if( str_stat == "charging" || str_stat == "charge_full" || str_stat == "full" )
        {
            curr_stat = CHARGING;
        }
        else
        {
            curr_stat = NOT_CHARGING;
        }

        RETIF( curr_stat == pthis->m_charge_status );
        LOGD( "charge status update to: %d", curr_stat );
        pthis->m_charge_status = curr_stat;
        pthis->update_charger_status();
    };




    static void handle_bat_battery_voltage_now( BatService *pthis, char *buf )
    {
        LOGI( "battery voltage value: %s", buf );
        float curr_voltage = strtof( buf, NULL );
        RETIF( curr_voltage <= 0 );
        curr_voltage /= 1000000;
        CubicStatSet( CUBIC_STAT_bat_vol_measure, curr_voltage );
        LOGI( "battery curr_voltage raw: %f", curr_voltage );
        // smooth voltage change
        curr_voltage += CUBIC_VOLTAGE_COMPENSATION;

        if( pthis->m_battery_voltage == 0 )
        {
            pthis->m_battery_voltage = curr_voltage;
            return;
        }

        curr_voltage = pthis->m_battery_voltage * CUBIC_VOLTAGE_ALPHA + curr_voltage * ( 1 - CUBIC_VOLTAGE_ALPHA );
        CubicStatSet( CUBIC_STAT_bat_vol, curr_voltage );
        LOGI( "battery curr_voltage: %f <> %f", curr_voltage, pthis->m_battery_voltage );
        RETIF( pthis->m_charge_status == CHARGE_UNKNOWN );
        RETIF( pthis->m_charge_status == NOT_CHARGING && curr_voltage > pthis->m_battery_voltage );
        RETIF( pthis->m_charge_status != NOT_CHARGING && curr_voltage < pthis->m_battery_voltage );
        pthis->m_battery_voltage = curr_voltage;
        // judge percent and level
        float percent = 0;

        if( pthis->m_charge_status == CHARGING )
        {
            double capacity = pthis->m_charging_forecast[curr_voltage];
            percent = capacity / S_BAT_MAX_CAP;
            LOGI( "charging capacity: %lf, percent=%f", capacity, percent );
        }
        else
        {
            double used = pthis->m_discarge_forecast[curr_voltage];
            percent = 1 - ( used / S_BAT_MAX_CAP );
            LOGI( "discharge used: %lf, percent=%f", used, percent );
        }

        if( percent > 1 )
        {
            percent = 1;
        }

        pthis->m_battery_percent = percent;
        CubicStatSet( CUBIC_STAT_bat_percent, percent );
        BatteryLevel level = BATTERY_UNKNOWN;

        if( percent < 0.03 )
        {
            level = BATTERY_OUT;
        }
        else if( percent < 0.10 )
        {
            level = BATTERY_EMERGENCY;
        }
        else if( percent < 0.40 )
        {
            level = BATTERY_LOW;
        }
        else if( percent < 0.70 )
        {
            level = BATTERY_NORMAL;
        }
        else
        {
            level = BATTERY_GOOD;
        }

        RETIF( level == pthis->m_battery_level );
        LOGD( "battery level update to: %d, %lf, %f", ( int )level, curr_voltage, percent );
        CubicStatSet( CUBIC_STAT_bat_level,   ( int )level );
        pthis->m_battery_level = level;
        // no battery event, if in charging
        RETIF( pthis->m_charge_status == CHARGING );

        switch( level )
        {
        case BATTERY_OUT:
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_BATTERY_OUT );
            break;

        case BATTERY_EMERGENCY:
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_BATTERY_LOW );
            break;

        default: // include unknown status
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_BATTERY_NORMAL );
            break;
        }
    };


public:
    BatService()
        : m_watch_timer_id( -1 )
        , m_charge_status( CHARGE_UNKNOWN )
        , m_charge_type( TYPE_UNKNOWN )
        , m_battery_level( BATTERY_UNKNOWN )
        , m_battery_voltage( 0 )
        , m_battery_percent( 0 )
        , m_charging_forecast( S_CHARGE_REFS_TAB,    sizeof( S_CHARGE_REFS_TAB ) / sizeof( CForecast::Point ),    0.9 )
        , m_discarge_forecast( S_DISCHARGE_REFS_TAB, sizeof( S_DISCHARGE_REFS_TAB ) / sizeof( CForecast::Point ), 0.9 )
    {};

    ~BatService()
    {};

    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        m_watch_timer_id =  CubicSetTimerInterval( TIMER_TIMEOUT, this );
        CubicStatSet( CUBIC_STAT_charger_status,  "none" );
        CubicStatSet( CUBIC_STAT_bat_vol_measure, "0.0" );
        CubicStatSet( CUBIC_STAT_bat_vol,         "0.0" );
        CubicStatSet( CUBIC_STAT_bat_percent,     "1.00" );
        CubicStatSet( CUBIC_STAT_bat_level,       ( int )BATTERY_GOOD );
        open_bat_fds();
        return true;
    };

    void onDeInit()
    {
        LOGD( "onDeInit" );
        CubicKillTimer( m_watch_timer_id );
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMeassage n_msg_id : %d", n_msg_id );
        int ret = 0 ;

        switch( n_msg_id )
        {
        default:
            LOGE( "invalid message" );
            break;
        }

        return ret;
    };

    void onTimer( int n_timer_id )
    {
        LOGI( "onTimer" );
        open_bat_fds();
    };
};


IMPLEMENT_CUBIC_APP( BatService )

#endif
