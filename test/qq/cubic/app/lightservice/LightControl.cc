/**
 * @file LightControl.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief light effect control
 * @detail light effect control
 */
#ifndef _LIGHT_CONTROL_CC_
#define _LIGHT_CONTROL_CC_ 1

#include "cubic_inc.h"
#include "light_defs.h"
#include "LightThread.cc"
#include "CAsyncRun.cc"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <assert.h>

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "lightcontrol"



class LightControl
{
public:
    typedef enum LightStatus
    {
        LIGHT_STAT_FIRST = 0,
        LIGHT_STAT_OFF = LIGHT_STAT_FIRST,
        LIGHT_STAT_RESTORING,
        LIGHT_STAT_UPGRADE,
        LIGHT_STAT_BOOTING,
        LIGHT_STAT_USR_CHK_BAT,
        LIGHT_STAT_USR_CHK_NET,
        LIGHT_STAT_USR_CHK_GPS,
        LIGHT_STAT_SOS,
        LIGHT_STAT_CALL_MUTE,
        LIGHT_STAT_CALL_CONVERSATION,
        LIGHT_STAT_CALL_UNCONNECT,
        LIGHT_STAT_MESSAGE,
        LIGHT_STAT_LEAVE_MESSAGE,
        LIGHT_STAT_BT,
        LIGHT_STAT_ERROR_OTA,
        LIGHT_STAT_ERROR_SIM,
        LIGHT_STAT_ERROR_NET,
        LIGHT_STAT_ERROR_ACTIVATE,
        LIGHT_STAT_ERROR_SIP,
        LIGHT_STAT_UPDATE_READY,
        LIGHT_STAT_UPDATING,
        LIGHT_STAT_ACTIVE,
        LIGHT_STAT_PASSIVE,
        LIGHT_STAT_MAX,
    } LightStatus;


    class IDefineLight
    {
    public:
        virtual void define( CubicLightKey out_def[CUBIC_LIGHT_NUM][CUBIC_LIGHT_KEY_MAX] ) = 0;
    };
    static IDefineLight *s_light_def_tab[LIGHT_STAT_MAX];


private:
    LightThread *mp_light_thread;
    int m_stat_tab[LIGHT_STAT_MAX];


    LightControl()
        : mp_light_thread( NULL )
        , m_stat_tab()
    {
        memset( m_stat_tab, 0, sizeof( m_stat_tab ) );
    };

    int findTopStat()
    {
        int stat;

        for( stat = LIGHT_STAT_FIRST; stat < LIGHT_STAT_MAX; stat++ )
        {
            BREAKIF( m_stat_tab[stat] != 0 );
        }

        LOGI( "findTopStat %d", stat );
        return stat;
    };

    void updateLight()
    {
        int stat = findTopStat();

        if( stat >= LIGHT_STAT_MAX || stat <= LIGHT_STAT_OFF )
        {
            DELETE( mp_light_thread );
            char zero[2] = {'0', 0};

            for( int i = 0; i < CUBIC_LIGHT_NUM; i++ )
            {
                CONTINUEIF( CUBIC_LIGHT_PATH[i] == NULL );
                CUtil::WriteFile( CUBIC_LIGHT_PATH[i], zero, 2 );
            }

            return;
        };

        LOGD( "updateLight get light definition to stat=%d", stat );

        //assert( stat != LIGHT_STAT_CALL_MUTE && stat != LIGHT_STAT_CALL_CONVERSATION && stat != LIGHT_STAT_CALL_UNCONNECT );
        CubicLightKey light_def[CUBIC_LIGHT_NUM][CUBIC_LIGHT_KEY_MAX];

        memset( light_def, 0, sizeof( light_def ) );

        RETIF( s_light_def_tab[stat] == NULL );

        s_light_def_tab[stat]->define( light_def );

        LOGI( "updateLight start light thread" );

        DELETE( mp_light_thread );

        mp_light_thread = new LightThread( light_def, CubicCfgGet( CUBIC_CFG_light_particle_len, ( int )20000 ) );

        RETIF( mp_light_thread == NULL );

        mp_light_thread->start();

        LOGI( "updateLight done" );
    };

public:
    ~LightControl()
    {
        DELETE( mp_light_thread );
    };

    static LightControl &getInstance()
    {
        static LightControl instance;
        return instance;
    };

    void setStatus( LightStatus stat, bool set, bool update_immediately = true )
    {
        RETIF( stat >= LIGHT_STAT_MAX || stat < LIGHT_STAT_FIRST );
        LOGI( "setStatus stat[%d] ==> %s", stat, set ? "true" : "false" );
        int val = set ? 1 : 0;
        RETIF( m_stat_tab[stat] == val );
        m_stat_tab[stat] = val;

        if( update_immediately && findTopStat() >= stat )
        {
            updateLight();
        }
    };

    bool getStatus( LightStatus stat )
    {
        RETNIF( stat >= LIGHT_STAT_MAX || stat < LIGHT_STAT_FIRST, false );
        return ( m_stat_tab[stat] != 0 );
    };

    void updateStatus()
    {
        updateLight();
    };

    void runSyncLightEffect( const CubicLightKey light_define[CUBIC_LIGHT_NUM][CUBIC_LIGHT_KEY_MAX], bool recover_after_done = false )
    {
        LOGI( "runSyncLightEffect stat ==>" );
        LOGD( "runSyncLightEffect light thread" );
        DELETE( mp_light_thread );
        mp_light_thread = new LightThread( light_define, CubicCfgGet( CUBIC_CFG_light_particle_len, ( int )20000 ) );
        RETIF( mp_light_thread == NULL );
        mp_light_thread->runOneRound();

        if( recover_after_done )
        {
            LOGI( "runSyncLightEffect recover to normal flashing" );
            updateLight();
        }

        LOGI( "runSyncLightEffect done" );
    };
};

LightControl::IDefineLight *LightControl::s_light_def_tab[LightControl::LIGHT_STAT_MAX] = {NULL};

#define DEFINE_LIGHT( ClassName, ID ) \
    class ClassName : public LightControl::IDefineLight { \
    public: \
        ClassName() {  LightControl::s_light_def_tab[LightControl::ID] = this; } \
        virtual void define( CubicLightKey out_def[CUBIC_LIGHT_NUM][CUBIC_LIGHT_KEY_MAX] )

#define DEFINE_LIGHT_END( ClassName ) \
    }; \
    static ClassName s_light_def_##ClassName


DEFINE_LIGHT( UsrChkBat, LIGHT_STAT_USR_CHK_BAT )
{
    int level = CubicStatGetI( CUBIC_STAT_bat_level );
    string charging = CubicStatGetStr( CUBIC_STAT_charger_status );
    const CubicLightKey *light_def;
    int light_def_size;
    LOGD( "UsrChkBat: level:%d, charging:%s, type:%s, voltage:%s, measure:%s, percent:%s",
          level, charging.c_str(),
          CubicStatGetStr( CUBIC_STAT_charger_type ).c_str(),
          CubicStatGetStr( CUBIC_STAT_bat_vol ).c_str(),
          CubicStatGetStr( CUBIC_STAT_bat_vol_measure ).c_str(),
          CubicStatGetStr( CUBIC_STAT_bat_percent ).c_str() );

    if( charging.length() <= 0 || charging == "null" || charging == "none" )
    {
        light_def = CUBIC_LIGHT_EFFECT_KEEP_LIGHT;
        light_def_size = sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT );
    }
    else
    {
        light_def = CUBIC_LIGHT_EFFECT_QUICK_FLASH;
        light_def_size = sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH );
    }

    switch( level )
    {
    default:
    case 0:
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        break;

    case 1:
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], light_def, light_def_size );
        break;

    case 2:
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        break;

    case 3:
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], light_def, light_def_size );
        break;
    }
}
DEFINE_LIGHT_END( UsrChkBat );


DEFINE_LIGHT( UsrChkNet, LIGHT_STAT_USR_CHK_NET )
{
    int level = CubicStatGetI( CUBIC_STAT_net_signal );
    int uim_stat = CubicStatGetI( CUBIC_STAT_net_uim_state );
    int connected = CubicStatGetI( CUBIC_STAT_net_connected );
    const CubicLightKey *light_def;
    int light_def_size;
    LOGD( "UsrChkNet: uim_stat:%d, level:%d, connected:%d", uim_stat, level, connected );

    if( uim_stat != 0 )
    {
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
        return;
    }

    if( connected != 0 )
    {
        light_def = CUBIC_LIGHT_EFFECT_KEEP_LIGHT;
        light_def_size = sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT );
    }
    else
    {
        light_def = CUBIC_LIGHT_EFFECT_QUICK_FLASH;
        light_def_size = sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH );
    }

    switch( level )
    {
    default:
    case 0:
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
        break;

    case 1:
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], light_def, light_def_size );
        break;

    case 2:
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        break;

    case 3:
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], light_def, light_def_size );
        break;
    }
}
DEFINE_LIGHT_END( UsrChkNet );


DEFINE_LIGHT( UsrChkGps, LIGHT_STAT_USR_CHK_GPS )
{
    int level = CubicStatGetI( CUBIC_STAT_location_signal );
    int valid = CubicStatGetI( CUBIC_STAT_location_valid );
    const CubicLightKey *light_def;
    int light_def_size;
    LOGD( "UsrChkGps: valid:%d, level:%d, inview:%s, measure:%s", valid, level, CubicStatGetStr( CUBIC_STAT_location_stars ).c_str(), CubicStatGetStr( CUBIC_STAT_location_signal_measure ).c_str() );

    if( CubicStatGetI( CUBIC_STAT_location_valid ) != 0 )
    {
        light_def = CUBIC_LIGHT_EFFECT_KEEP_LIGHT;
        light_def_size = sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT );
    }
    else
    {
        light_def = CUBIC_LIGHT_EFFECT_QUICK_FLASH;
        light_def_size = sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH );
    }

    switch( level )
    {
    default:
    case 0:
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
        break;

    case 1:
        memcpy( out_def[CUBIC_LIGHT_ID_RED_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_RED_4], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], light_def, light_def_size );
        break;

    case 2:
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], light_def, light_def_size );
        break;

    case 3:
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], light_def, light_def_size );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], light_def, light_def_size );
        break;
    }
}
DEFINE_LIGHT_END( UsrChkGps );

DEFINE_LIGHT( SOS, LIGHT_STAT_SOS )
{
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
}
DEFINE_LIGHT_END( SOS );

DEFINE_LIGHT( CallMute, LIGHT_STAT_CALL_MUTE )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_FLASH ) );
}
DEFINE_LIGHT_END( CallMute );

DEFINE_LIGHT( CallConversation, LIGHT_STAT_CALL_CONVERSATION )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
}
DEFINE_LIGHT_END( CallConversation );

DEFINE_LIGHT( CallUnconnect, LIGHT_STAT_CALL_UNCONNECT )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
}
DEFINE_LIGHT_END( CallUnconnect );

DEFINE_LIGHT( Message, LIGHT_STAT_MESSAGE )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
}
DEFINE_LIGHT_END( Message );

DEFINE_LIGHT( LM, LIGHT_STAT_LEAVE_MESSAGE )
{
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_KEEP_LIGHT, sizeof( CUBIC_LIGHT_EFFECT_KEEP_LIGHT ) );
}
DEFINE_LIGHT_END( LM );

DEFINE_LIGHT( Bluetooth, LIGHT_STAT_BT )
{
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], CUBIC_LIGHT_EFFECT_BT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_BT_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_2], CUBIC_LIGHT_EFFECT_BT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_BT_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_3], CUBIC_LIGHT_EFFECT_BT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_BT_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_4], CUBIC_LIGHT_EFFECT_BT_FLASH, sizeof( CUBIC_LIGHT_EFFECT_BT_FLASH ) );
}
DEFINE_LIGHT_END( Bluetooth );

DEFINE_LIGHT( ErrorNet, LIGHT_STAT_ERROR_NET )
{
    memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_NET, sizeof( CUBIC_LIGHT_EFFECT_ERR_NET ) );
}
DEFINE_LIGHT_END( ErrorNet );


DEFINE_LIGHT( ErrorSIM, LIGHT_STAT_ERROR_SIM )
{
    memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_SIM, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIM ) );
}
DEFINE_LIGHT_END( ErrorSIM );

DEFINE_LIGHT( ErrorActivate, LIGHT_STAT_ERROR_ACTIVATE )
{
    memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_ACTIVATE, sizeof( CUBIC_LIGHT_EFFECT_ERR_ACTIVATE ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_ACTIVATE, sizeof( CUBIC_LIGHT_EFFECT_ERR_ACTIVATE ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_ACTIVATE, sizeof( CUBIC_LIGHT_EFFECT_ERR_ACTIVATE ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_ACTIVATE, sizeof( CUBIC_LIGHT_EFFECT_ERR_ACTIVATE ) );
}
DEFINE_LIGHT_END( ErrorActivate );

DEFINE_LIGHT( ErrorSip, LIGHT_STAT_ERROR_SIP )
{
    memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_SIP, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIP ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_SIP, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIP ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_SIP, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIP ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_SIP, sizeof( CUBIC_LIGHT_EFFECT_ERR_SIP ) );
}
DEFINE_LIGHT_END( ErrorSip );

DEFINE_LIGHT( Restoring, LIGHT_STAT_RESTORING )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_QUICK_FLASH, sizeof( CUBIC_LIGHT_EFFECT_QUICK_FLASH ) );
}
DEFINE_LIGHT_END( Restoring );

DEFINE_LIGHT( UpdateReady, LIGHT_STAT_UPDATE_READY )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_RING_1, sizeof( CUBIC_LIGHT_EFFECT_RING_1 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_RING_2, sizeof( CUBIC_LIGHT_EFFECT_RING_2 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_RING_3, sizeof( CUBIC_LIGHT_EFFECT_RING_3 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_RING_4, sizeof( CUBIC_LIGHT_EFFECT_RING_4 ) );
}
DEFINE_LIGHT_END( UpdateReady );

DEFINE_LIGHT( Updating, LIGHT_STAT_UPDATING )
{
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_QUICK_RING_1, sizeof( CUBIC_LIGHT_EFFECT_QUICK_RING_1 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_QUICK_RING_2, sizeof( CUBIC_LIGHT_EFFECT_QUICK_RING_2 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_QUICK_RING_3, sizeof( CUBIC_LIGHT_EFFECT_QUICK_RING_3 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_QUICK_RING_4, sizeof( CUBIC_LIGHT_EFFECT_QUICK_RING_4 ) );
}
DEFINE_LIGHT_END( Updating );

DEFINE_LIGHT( Booting, LIGHT_STAT_BOOTING )
{
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_2], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_3], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_BLUE_4], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_1 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2 ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2, sizeof( CUBIC_LIGHT_EFFECT_ALTENATELY_FLASH_2 ) );
}
DEFINE_LIGHT_END( Booting );

DEFINE_LIGHT( Active, LIGHT_STAT_ACTIVE )
{
    string charg_stat = CubicStatGetStr( CUBIC_STAT_charger_status );
    string bt_stat = CubicStatGetStr( CUBIC_STAT_bt_status );

    if( bt_stat == "on" )
    {
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_2], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_3], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_4], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
    }
    else if( charg_stat == "on" )
    {
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
    }
    else
    {
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_ACTIVE, sizeof( CUBIC_LIGHT_EFFECT_ACTIVE ) );
    }
}
DEFINE_LIGHT_END( Active );

DEFINE_LIGHT( Passive, LIGHT_STAT_PASSIVE )
{
    string charg_stat = CubicStatGetStr( CUBIC_STAT_charger_status );
    string bt_stat = CubicStatGetStr( CUBIC_STAT_bt_status );

    if( bt_stat == "on" )
    {
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_1], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_2], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_3], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_BLUE_4], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
    }
    else if( charg_stat == "on" )
    {
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
    }
    else
    {
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_1], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_2], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_3], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
        memcpy( out_def[CUBIC_LIGHT_ID_WHITE_4], CUBIC_LIGHT_EFFECT_PASSIVE, sizeof( CUBIC_LIGHT_EFFECT_PASSIVE ) );
    }
}
DEFINE_LIGHT_END( Passive );

DEFINE_LIGHT( OtaUpgrade, LIGHT_STAT_UPGRADE )
{
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_1], CUBIC_LIGHT_EFFECT_OTA_FLASH, sizeof( CUBIC_LIGHT_EFFECT_OTA_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_2], CUBIC_LIGHT_EFFECT_OTA_FLASH, sizeof( CUBIC_LIGHT_EFFECT_OTA_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_3], CUBIC_LIGHT_EFFECT_OTA_FLASH, sizeof( CUBIC_LIGHT_EFFECT_OTA_FLASH ) );
    memcpy( out_def[CUBIC_LIGHT_ID_GREEN_4], CUBIC_LIGHT_EFFECT_OTA_FLASH, sizeof( CUBIC_LIGHT_EFFECT_OTA_FLASH ) );
}
DEFINE_LIGHT_END( OtaUpgrade );

DEFINE_LIGHT( ErrorOta, LIGHT_STAT_ERROR_OTA )
{
    memcpy( out_def[CUBIC_LIGHT_ID_RED_1], CUBIC_LIGHT_EFFECT_ERR_OTA, sizeof( CUBIC_LIGHT_EFFECT_ERR_OTA ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_2], CUBIC_LIGHT_EFFECT_ERR_OTA, sizeof( CUBIC_LIGHT_EFFECT_ERR_OTA ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_3], CUBIC_LIGHT_EFFECT_ERR_OTA, sizeof( CUBIC_LIGHT_EFFECT_ERR_OTA ) );
    memcpy( out_def[CUBIC_LIGHT_ID_RED_4], CUBIC_LIGHT_EFFECT_ERR_OTA, sizeof( CUBIC_LIGHT_EFFECT_ERR_OTA ) );
}
DEFINE_LIGHT_END( ErrorOta );

#endif //_LIGHT_CONTROL_CC_

