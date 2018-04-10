#ifndef __CLOCATIONSERVICE_CC__
#define __CLOCATIONSERVICE_CC__ 1

#include "CFramework.cc"
#include "CThread.cc"
#include "CLock.cc"
#include "cubic_func.h"
#include "CLocationServiceCommon.h"
#include <iostream>

using namespace std;


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "LocationService"
#define INIT_CUBIC_STATUS 0


class LocationService : public ICubicApp
{
private:
    typedef struct GFenceDef
    {
        string name;
        float north;
        float south;
        float east;
        float west;
        int is_in;
    } GFenceDef;

    GFenceDef   m_fence_config_tab[GEOFENCE_INDEX_MAX];
    int         m_fence_config_num;
    CLock       m_fence_config_lock;
    static int  s_loc_info_valid;
    static int  s_loc_state;

    void init_cubic_state()
    {
        LOGI( "init cubic share state" );
        CubicStatSet( CUBIC_STAT_location_signal,           INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_signal_measure,   INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_valid,            INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_stars,            INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_lat,              INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_long,             INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_alt,              INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_vel,              INIT_CUBIC_STATUS );
        CubicStatSet( CUBIC_STAT_location_dir,              INIT_CUBIC_STATUS );
    }

    int load_geo_fence_config()
    {
        CLock::Auto lock( m_fence_config_lock );
        m_fence_config_num = 0;
        vector<string> geofence_configs = CubicCfgEnum( CUBIC_CFG_loc_fence );
        unsigned int geofenceNum = geofence_configs.size();
        geofenceNum = MIN( GEOFENCE_INDEX_MAX, geofenceNum );

        for( unsigned int i = 0 ; i < geofenceNum; i++ )
        {
            m_fence_config_tab[i].name = CubicCfgGetVStr( CUBIC_CFG_loc_fence_name, i );
            BREAKIF( m_fence_config_tab[i].name.length() == 0 || m_fence_config_tab[i].name == "null" );
            float latitude = CubicCfgGetVF( CUBIC_CFG_loc_fence_lat, i );
            float longitude = CubicCfgGetVF( CUBIC_CFG_loc_fence_long, i );
            float radius = CubicCfgGetVF( CUBIC_CFG_loc_fence_rad, i );
            float lat_def = radius / LATITUDE_M;
            float long_def = cos( latitude ) * radius / LATITUDE_M;
            m_fence_config_tab[i].north = latitude + lat_def;
            m_fence_config_tab[i].south = latitude - lat_def;
            m_fence_config_tab[i].east = longitude + long_def;
            m_fence_config_tab[i].west = longitude - long_def;
            m_fence_config_tab[i].is_in = 0;
            m_fence_config_num++;
        }

        return 0;
    };


    static void loc_gps_location_cb( GpsLocation *location )
    {
        CubicLogI( "cubic_loc LAT: %f, LON: %f, ALT: %f, SPEED: %f",
                   location->latitude,
                   location->longitude,
                   location->altitude,
                   location->speed );
        CubicStatSet( CUBIC_STAT_location_lat,  location->latitude );
        CubicStatSet( CUBIC_STAT_location_long, location->longitude );
        CubicStatSet( CUBIC_STAT_location_alt,  location->altitude );
        CubicStatSet( CUBIC_STAT_location_vel,  location->speed );
        CubicStatSet( CUBIC_STAT_location_valid, TRUE );
        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_RENEW );
        s_loc_info_valid = LOC_INFO_VALID;
        CubicPost( CUBIC_THIS_APP, CUBIC_MSG_GPS_GEOFENCE_MEASURE );
    };
    static void loc_gps_status_callback( GpsStatus *status )
    {
        CubicLogI( "cubic_loc GpsStatus.size=%d,GpsStatus.status=%d\n", status->size, status->status );

        if( status->status != GPS_STATUS_SESSION_BEGIN )
        {
            CubicStatSet( CUBIC_STAT_location_signal, SIGNAL_STRENGTH_NONE_OR_UNKNOWN );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_LOST );
            CubicStatSet( CUBIC_STAT_location_valid, FALSE );
            s_loc_info_valid = LOC_INFO_INVALID;
        }

        s_loc_state = status->status;
    };

    static void loc_gps_sv_status_cb ( GpsSvStatus *sv_status )
    {
        int loc_signal_level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
        int loc_valid_stars = 0;
        float loc_snr_sum = 0;
        CubicLogI( "cubic_loc Number of SVs currently visible : %d\n", sv_status->num_svs );

        for( int i = 0; i < sv_status->num_svs && i < GPS_MAX_SVS; i++ )
        {
            loc_snr_sum += sv_status->sv_list[i].snr;

            if( sv_status->sv_list[i].snr > 0 )
            {
                loc_valid_stars ++;
            }
        }

        if ( loc_snr_sum >= 85 )
        {
            loc_signal_level = SIGNAL_STRENGTH_GREAT;
        }
        else if ( loc_snr_sum >= 45 )
        {
            loc_signal_level = SIGNAL_STRENGTH_GOOD;
        }
        else if ( loc_snr_sum > 5 )
        {
            loc_signal_level = SIGNAL_STRENGTH_POOR;
        }
        else
        {
            loc_signal_level = SIGNAL_STRENGTH_NONE_OR_UNKNOWN;
        }

        LOGD( "cubic_loc loc_valid_stars[%d] loc_snr_sum=[%f]", loc_valid_stars, loc_snr_sum );
        CubicLogI( "cubic_loc CUBIC_STAT_location_signal : %d", loc_signal_level );
        CubicStatSet( CUBIC_STAT_location_signal, loc_signal_level );
        CubicStatSet( CUBIC_STAT_location_stars,  loc_valid_stars );
    };

    static void init_gps_cbs()
    {
        GpsCallbacks *gps_cb;
        CubicLogI( "cubic_loc report_gps_cbs start" );
        gps_cb = ( GpsCallbacks * )malloc( sizeof( GpsCallbacks ) );
        gps_cb->size = sizeof( GpsCallbacks );
        gps_cb->location_cb = loc_gps_location_cb;
        gps_cb->status_cb = loc_gps_status_callback;
        gps_cb->sv_status_cb = loc_gps_sv_status_cb;
        gps_cb->nmea_cb = NULL;
        gps_cb->set_capabilities_cb = NULL;
        gps_cb->acquire_wakelock_cb = NULL;
        gps_cb->release_wakelock_cb = NULL;
        gps_cb->create_thread_cb = NULL;
        gps_ctl_init( gps_cb );
    };

    int cubic_msg_gps_update_stat()
    {
        CubicLogD( "cubic_loc cubic_msg_gps_update_stat start " );

        if( s_loc_state != GPS_STATUS_SESSION_BEGIN )
        {
            CubicLogD( "cubic_loc CUBIC_MSG_GPS_LOST" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_LOST );
        }

        if( s_loc_info_valid == LOC_INFO_VALID )
        {
            CubicLogD( "cubic_loc CUBIC_MSG_GPS_RENEW" );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_RENEW );
        }

        return 0;
    };

    int cubic_msg_gps_geofence_measure()
    {
        CLock::Auto lock( m_fence_config_lock );
        CubicLogD( "cubic_loc cubic_msg_gps_geofence_measure start " );
        float m_loc_latitude = CubicStatGetF( CUBIC_STAT_location_lat );
        float m_loc_longitude = CubicStatGetF( CUBIC_STAT_location_long );

        for( int i = 0 ; i < GEOFENCE_INDEX_MAX && i < m_fence_config_num; i++ )
        {
            if( m_loc_latitude > m_fence_config_tab[i].south ||
                    m_loc_latitude < m_fence_config_tab[i].north ||
                    m_loc_longitude > m_fence_config_tab[i].west ||
                    m_loc_longitude < m_fence_config_tab[i].east )
            {
                if( m_fence_config_tab[i].is_in == 1 )
                {
                    cubic_msg_gps_fence_evt arg;
                    memset( &arg, 0, sizeof( arg ) );
                    arg.index = i;
                    arg.is_in = 0;
                    CubicLogD( "cubic_loc out fence report --->index : %d name : %s ", i, m_fence_config_tab[i].name.c_str() );
                    CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_FENCE_EVT, arg );
                    m_fence_config_tab[i].is_in = 0;
                }
            }
            else
            {
                if( m_fence_config_tab[i].is_in == 0 )
                {
                    cubic_msg_gps_fence_evt arg;
                    memset( &arg, 0, sizeof( arg ) );
                    arg.index = i;
                    arg.is_in = 1;
                    CubicLogD( "cubic_loc in fence report --->index : %d name : %s", i, m_fence_config_tab[i].name.c_str() );
                    CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_FENCE_EVT, arg );
                    m_fence_config_tab[i].is_in = 1;
                }
            }
        }

        return 0;
    };

public:

    LocationService()
        : m_fence_config_lock()
    {
    };

    ~LocationService()
    {
    };

    virtual bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        init_cubic_state();
        gps_ctl_option_t ctl_option = {0, 0};
        gps_ctl_position_mode_t ctl_position_mode = {0, 0, 25, 6, 0}; // AGPS 1,0,25,6,0 only supurt CU CMCC
        load_geo_fence_config();
        init_gps_cbs();
        gps_ctl_set_option( ctl_option );
        gps_ctl_set_position_mode( ctl_position_mode );
        gps_ctl_start( GPS_START_STATE_HOT );
        CFramework::GetInstance().getLoger().setLevelLimit( 4 );
        return true;
    };

    virtual void onDeInit()
    {
        CubicLogD( "onDeInit" );
        gps_ctl_stop();
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        int ret = 0;
        CubicLogD( "onMeassage n_msg_id : %d", n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_GPS_UPDATE_STAT:
        {
            ret = cubic_msg_gps_update_stat();
        }
        break ;

        case CUBIC_MSG_GPS_SET_GEOFENCE:
        {
            ret = load_geo_fence_config();
        }
        break ;

        case CUBIC_MSG_GPS_GEOFENCE_MEASURE:
        {
            ret = cubic_msg_gps_geofence_measure();
        }
        break ;

        case CUBIC_MSG_GPS_ENABLE:
        {
            LOGD( "CUBIC_MSG_GPS_ENABLE" );
            gps_ctl_start( GPS_START_STATE_HOT );
        }
        break ;

        case CUBIC_MSG_GPS_DISABLE:
        {
            LOGD( "CUBIC_MSG_GPS_DISABLE" );
            gps_ctl_stop();
        }
        break ;

        default:
            break;
        }

        return ret ;
    };
};

int LocationService::s_loc_info_valid = LOC_INFO_INVALID;
int LocationService::s_loc_state = 0;


IMPLEMENT_CUBIC_APP( LocationService )

#endif
