#ifndef __U_LOCATION_SERVICE_CC__
#define __U_LOCATION_SERVICE_CC__ 1

#include "cubic_inc.h"
#include "CFramework.cc"
#include "CUtil.cc"
#include "CThread.cc"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <iostream>

//#define _REMOTE_DEBUGGING_ 1

#ifdef _REMOTE_DEBUGGING_
#include "CTcpConnection.cc"
#include "CTcpServer.cc"
#endif //_REMOTE_DEBUGGING_

#include <libserialport.h>
#include <nmea/nmea.h>

using namespace std;


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "ULocationService"


#define CUBIC_ULOC_NMEA_BUF_SZ 1024
#define CUBIC_ULOC_PORT_PATH   "/dev/ttyHSL2"
#define CUBIC_ULOC_POWER_PATH  "/sys/class/leds/GpsRest/brightness"
#define CUBIC_ULOC_FENCE_MAX   32
#define CUBIC_ULOC_LAT_FACTOR  111000
#define CUBIC_ULOC_PI          3.14159265358979323846
#define CUBIC_ULOC_PI_DIV_180  0.01745329251994329577

using namespace std;


class ULocationService
    : public ICubicApp
    , protected CThread
#ifdef _REMOTE_DEBUGGING_
    , public IConnectionListener
    , public ITcpListener
#endif //_REMOTE_DEBUGGING_
{
private:
#pragma pack(push)
#pragma pack(0)
    typedef struct UBXPacketH
    {
        uint8_t sync_char_1;
        uint8_t sync_char_2;
        uint8_t cls_id;
        uint8_t msg_id;
        uint16_t payload_len;
    } UBXPacketH;

    typedef struct UBXPacketT
    {
        uint8_t chksum_1;
        uint8_t chksum_2;
    } UBXPacketT;

    typedef struct UBX_ack
    {
        uint8_t  cls_id;
        uint8_t  msg_id;
    } UBX_ack;

    typedef struct UBX_cfg_nav5
    {
        uint16_t mask;
        uint8_t  dynModel;
        uint8_t  fixMode;
        int32_t  fixedAlt;
        uint32_t fixedAltVar;
        int8_t   minElev;
        uint8_t  drLimit;
        uint16_t pDop;
        uint16_t tDop;
        uint16_t pAcc;
        uint16_t tAcc;
        uint8_t  staticHoldThresh;
        uint8_t  dgnssTimeout;
        uint8_t  cnoThreshNumSVs;
        uint8_t  cnoThresh;
        uint16_t pAccAdr;
        uint16_t staticHoldMaxDist;
        uint8_t  utcStandard;
        uint8_t  reserved[5];
    } UBX_cfg_nav5;

    typedef struct UBX_cfg_rst
    {
        uint16_t  navBbrMask;
        uint8_t   resetMode;
        uint8_t   reserved;
    } UBX_cfg_rst;
#pragma pack(pop)

    typedef struct GpsFence
    {
        double edge_e;
        double edge_w;
        double edge_n;
        double edge_s;
        string name;
    } GpsFence;

#ifdef _REMOTE_DEBUGGING_
    static const int PRIVATE_MSG_DISCONNECT = CUBIC_MSG_APP_PRIVATE + 1;
#endif //_REMOTE_DEBUGGING_
    int             m_gps_fixed;
    int             m_nmea_log_fd;
    int             m_fence_curr_idx;
    double          m_curr_lat;
    double          m_curr_lon;
    GpsFence        m_fence_tab[CUBIC_ULOC_FENCE_MAX + 1];
    struct sp_port *m_serial_port;
    nmeaTIME        m_last;
    nmeaPARSER      m_gps_parser;
    nmeaINFO        m_gps_info;
    int             m_MEM_FENCE;
#ifdef _REMOTE_DEBUGGING_
    CTcpServer      m_rd_server; // remote debugging server
    CTcpConnection *m_p_rdc;     // remote debugging connection
#endif //_REMOTE_DEBUGGING_

    double getDegree( double val )
    {
        double degree = ( int )val / 100;
        double min = val - ( degree * 100 );
        degree += min / 60;
        return degree;
    }

    void update_status()
    {
        int signal_level = 0;

        for( int i = 0; i < m_gps_info.satinfo.inview; i++ )
        {
            LOGI( "update_status sta=%d, id:%d, in_use:%d, elv:%d, azimuth:%d, sig%d", i,
                  m_gps_info.satinfo.sat[i].id,
                  m_gps_info.satinfo.sat[i].in_use,
                  m_gps_info.satinfo.sat[i].elv,
                  m_gps_info.satinfo.sat[i].azimuth,
                  m_gps_info.satinfo.sat[i].sig );
            signal_level += m_gps_info.satinfo.sat[i].sig;
        }

        CubicStatSet( CUBIC_STAT_location_stars, m_gps_info.satinfo.inview );
        CubicStatSet( CUBIC_STAT_location_signal_measure, ( int )signal_level );

        if( m_gps_info.sig != 0 )
        {
            m_curr_lat = getDegree( m_gps_info.lat );
            m_curr_lon = getDegree( m_gps_info.lon );
            CubicStatSet( CUBIC_STAT_location_lat,  m_curr_lat );
            CubicStatSet( CUBIC_STAT_location_long, m_curr_lon );
            CubicStatSet( CUBIC_STAT_location_alt,  m_gps_info.elv );
            CubicStatSet( CUBIC_STAT_location_vel,  m_gps_info.speed );
            CubicStatSet( CUBIC_STAT_location_dir,  m_gps_info.direction );
            LOGI( "update_status, lat=%lf, lon=%lf", m_curr_lat, m_curr_lon );
        }

        LOGI( "update_status sig=%d fix=%d lat=%lf lon=%lf elv=%lf speed=%lf dir=%lf sate=%d/%d, signal:%d",
              m_gps_info.sig, m_gps_info.fix, m_gps_info.lat, m_gps_info.lon, m_gps_info.elv,
              m_gps_info.speed, m_gps_info.direction, m_gps_info.satinfo.inuse, m_gps_info.satinfo.inview, signal_level );

        if( signal_level < 20 )
        {
            CubicStatSet( CUBIC_STAT_location_signal, ( int )0 );
        }
        else if( signal_level < 50 )
        {
            CubicStatSet( CUBIC_STAT_location_signal, ( int )1 );
        }
        else if( signal_level < 90 )
        {
            CubicStatSet( CUBIC_STAT_location_signal, ( int )2 );
        }
        else
        {
            CubicStatSet( CUBIC_STAT_location_signal, ( int )3 );
        }

        RETIF( m_gps_info.sig == m_gps_fixed );
        LOGD( "update_status, fix status update %d ==> %d", m_gps_fixed, m_gps_info.sig );
        m_gps_fixed = m_gps_info.sig;
        CubicStatSet( CUBIC_STAT_location_valid, ( int )m_gps_info.sig );

        if( m_gps_info.sig == 0 )
        {
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_LOST );
            return;
        }

        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_RENEW );
        // sync system time if needed
        nmeaTIME sys_time;
        nmea_time_now( &sys_time );

        if( sys_time.year != m_gps_info.utc.year )
        {
            LOGD( "update_status set time of year:%d mon:%d day:%d, hour:%d, min:%d, sec:%d",
                  m_gps_info.utc.year, m_gps_info.utc.mon, m_gps_info.utc.day,
                  m_gps_info.utc.hour, m_gps_info.utc.min, m_gps_info.utc.sec );
            CUtil::setSystemTime(
                m_gps_info.utc.year,
                m_gps_info.utc.mon,
                m_gps_info.utc.day,
                m_gps_info.utc.hour,
                m_gps_info.utc.min,
                m_gps_info.utc.sec );
        }
    }


    void load_gps_fence_setting()
    {
        m_fence_curr_idx = -1;

        for( int i = 0; i < CUBIC_ULOC_FENCE_MAX; i++ )
        {
            m_fence_tab[i].name = CubicCfgGetVStr( CUBIC_CFG_loc_fence_name, i );
            CONTINUEIF( m_fence_tab[i].name.length() <= 0 );
            float latitude = CubicCfgGetVF( CUBIC_CFG_loc_fence_lat, i );
            float longitude = CubicCfgGetVF( CUBIC_CFG_loc_fence_long, i );
            double radius = CubicCfgGetVF( CUBIC_CFG_loc_fence_rad, i );
            double lat_def = radius / CUBIC_ULOC_LAT_FACTOR;
            double long_def = cos( latitude * CUBIC_ULOC_PI_DIV_180 ) * radius / CUBIC_ULOC_LAT_FACTOR;
            m_fence_tab[i].edge_e = longitude + long_def;
            m_fence_tab[i].edge_w = longitude - long_def;
            m_fence_tab[i].edge_n = latitude + lat_def;
            m_fence_tab[i].edge_s = latitude - lat_def;
            LOGD( "load_gps_fence_setting: [%d] east:%lf west:%lf north:%lf south:%lf",
                  i, m_fence_tab[i].edge_e, m_fence_tab[i].edge_w,
                  m_fence_tab[i].edge_n, m_fence_tab[i].edge_s );
        }
    }

    void check_gps_fencs()
    {
        int curr_idx = -1;
        RETIF( m_curr_lat == 0.0 && m_curr_lon == 0.0 );

        for( int i = 0; i < CUBIC_ULOC_FENCE_MAX; i++ )
        {
            CONTINUEIF( m_fence_tab[i].name.length() <= 0 );

            if( m_curr_lat < m_fence_tab[i].edge_n &&
                    m_curr_lat > m_fence_tab[i].edge_s &&
                    m_curr_lon < m_fence_tab[i].edge_e &&
                    m_curr_lon > m_fence_tab[i].edge_w )
            {
                LOGI( "check_gps_fencs, in fence: %d, e=%lf,w=%lf,n=%lf,s=%lf", i,
                      m_fence_tab[i].edge_e, m_fence_tab[i].edge_w, m_fence_tab[i].edge_n, m_fence_tab[i].edge_s );
                curr_idx = i;
                break;
            }
        }

        RETIF( curr_idx == m_fence_curr_idx );
        LOGD( "check_gps_fencs, curr_idx=%d, last=%d", curr_idx, m_fence_curr_idx );
        // if get in some fence
        cubic_msg_gps_fence_evt evt;

        if( m_fence_curr_idx == -1 )
        {
            evt.index = curr_idx;
            evt.is_in = TRUE;
            CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_FENCE_EVT, evt );
            m_fence_curr_idx = curr_idx;
            return;
        }

        // get out fence
        evt.index = m_fence_curr_idx;
        evt.is_in = FALSE;
        CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_FENCE_EVT, evt );
        m_fence_curr_idx = -1;
    }


    int create_ubxp_cmd( uint8_t cls_id, uint8_t msg_id, void *data, uint16_t data_len, uint8_t **buffer )
    {
        uint8_t *ret = ( uint8_t * )malloc( sizeof( UBXPacketH ) + data_len + sizeof( UBXPacketT ) );

        if( ret == NULL )
        {
            return -1;
        }

        memset( ret, 0, sizeof( UBXPacketH ) + data_len + sizeof( UBXPacketT ) );
        UBXPacketH *p_head     = ( UBXPacketH * ) ret;
        UBXPacketT *p_tail     = ( UBXPacketT * )( ret + sizeof( UBXPacketH ) + data_len );
        uint8_t    *p_playload = ret + sizeof( UBXPacketH );
        p_head->sync_char_1 = 0xB5;
        p_head->sync_char_2 = 0x62;
        p_head->cls_id      = cls_id;
        p_head->msg_id      = msg_id;
        p_head->payload_len = data_len;
        memcpy( p_playload, data, data_len );

        for( uint8_t *p = &( p_head->cls_id ); p < & ( p_tail->chksum_1 ); p++ )
        {
            p_tail->chksum_1 = p_tail->chksum_1 + ( *p );
            p_tail->chksum_2 = p_tail->chksum_2 + p_tail->chksum_1;
        }

        *buffer = ret;
        return sizeof( UBXPacketH ) + data_len + sizeof( UBXPacketT );
    };


    inline void logout_binary( const char *tag, uint8_t *data, int size )
    {
        static int BUF_SZ = 1024;
        char buf[BUF_SZ + 4];
        int n = 0;
        LOGD( "%s, data %d ==============================>>", tag, size );

        for( int i = 0; i < size;  )
        {
            n = 0;
            memset( buf, 0, sizeof( buf ) );

            for( int j = 0; j < 8 && i < size && n < BUF_SZ; i++, j++ )
            {
                n += snprintf( buf + n, BUF_SZ - n, "%02X ", data[i] );
            }

            LOGD( "data: %s # %d", buf, i );
        }

        LOGD( "%s, data end <<=============================", tag, size );
    }

    void send_ubx_cfg_cfg( uint16_t staticHoldMaxDist, uint8_t staticHoldThresh )
    {
        uint8_t *data = NULL;
        int data_len = 0;
        LOGD( "send_ubx_cfg_cfg" );
        RETIF_LOGE( m_serial_port == NULL, "send_ubx_cfg_cfg, port not ready" );
        UBX_cfg_nav5 cfg;
        memset( &cfg, 0, sizeof( cfg ) );
        cfg.mask = 0x0007; // static hold
        cfg.staticHoldMaxDist = staticHoldMaxDist; // metre
        cfg.staticHoldThresh = staticHoldThresh; // cm/m
        data_len = create_ubxp_cmd( 0x06, 0x24, &cfg, sizeof( cfg ), &data );
        RETIF( data_len <= 0 );
        // send to module
        logout_binary( "send_ubx_cfg_cfg", data, data_len );
        sp_blocking_write( m_serial_port, data, data_len, 500 );
        free( data );
    }

    void send_ubx_cfg_rst( bool startOrStop = true )
    {
        uint8_t *data = NULL;
        int data_len = 0;
        LOGD( "send_ubx_cfg_rst" );
        RETIF_LOGE( m_serial_port == NULL, "send_ubx_cfg_rst, port not ready" );
        UBX_cfg_rst cfg;
        memset( &cfg, 0, sizeof( cfg ) );
        cfg.navBbrMask = 0x0000;
        cfg.resetMode = startOrStop ? 0x09 : 0x08;
        data_len = create_ubxp_cmd( 0x06, 0x04, &cfg, sizeof( cfg ), &data );
        RETIF( data_len <= 0 );
        // send to module
        logout_binary( "send_ubx_cfg_rst", data, data_len );
        sp_blocking_write( m_serial_port, data, data_len, 500 );
        free( data );
    }

#ifdef _REMOTE_DEBUGGING_
    // disconnect debugging connection
    void disconnect_rdc()
    {
        RETIF( m_p_rdc == NULL );
        delete m_p_rdc;
        m_p_rdc = NULL;
    };
#endif //_REMOTE_DEBUGGING_

public:
    ULocationService()
        : m_gps_fixed( 0 )
        , m_nmea_log_fd( -1 )
        , m_fence_curr_idx( -1 )
        , m_curr_lat( 0.0 )
        , m_curr_lon( 0.0 )
        , m_fence_tab()
        , m_serial_port( NULL )
        , m_last()
        , m_gps_parser()
        , m_gps_info()
        , m_MEM_FENCE( 0 )
#ifdef _REMOTE_DEBUGGING_
        , m_rd_server( this )
        , m_p_rdc( NULL )
#endif //_REMOTE_DEBUGGING_
    {
        memset( &m_gps_info, 0, sizeof( m_gps_info ) );
    };

    ~ULocationService()
    {
        this->onDeInit();
    };

    bool onInit()
    {
        int ret = 0;
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        // init all share status
        CubicStatSet( CUBIC_STAT_location_stars,            ( int )0 );
        CubicStatSet( CUBIC_STAT_location_signal_measure,   ( int )0 );
        CubicStatSet( CUBIC_STAT_location_signal,           ( int )0 );
        CubicStatSet( CUBIC_STAT_location_valid,            ( int )0 );
        CubicStatSet( CUBIC_STAT_location_lat,              0.0f );
        CubicStatSet( CUBIC_STAT_location_long,             0.0f );
        CubicStatSet( CUBIC_STAT_location_alt,              0.0f );
        CubicStatSet( CUBIC_STAT_location_vel,              0.0f );
        CubicStatSet( CUBIC_STAT_location_dir,              0.0f );
        load_gps_fence_setting();
        // open port
        ret = sp_get_port_by_name( CUBIC_ULOC_PORT_PATH, &m_serial_port );
        RETNIF_LOGE( SP_OK != ret || m_serial_port == NULL, false, "can not found port" );
        ret = sp_open( m_serial_port, SP_MODE_READ_WRITE );
        RETNIF_LOGE( SP_OK != ret, false, "fail to open port" );
        sp_set_baudrate(    m_serial_port, 9600 );
        sp_set_bits(        m_serial_port, 8 );
        sp_set_parity(      m_serial_port, SP_PARITY_NONE );
        sp_set_stopbits(    m_serial_port, 1 );
        sp_set_flowcontrol( m_serial_port, SP_FLOWCONTROL_NONE );
        // power on ublox
        CUtil::WriteFile( CUBIC_ULOC_POWER_PATH, "255", 3 );
        // init nmea parser
        nmea_zero_INFO( &m_gps_info );
        nmea_parser_init( &m_gps_parser );
        memset( &m_last, 0, sizeof( nmeaTIME ) );
        string nmea_log = CubicCfgGetStr( CUBIC_CFG_log_nmea );

        if( nmea_log.length() > 0 )
        {
            m_nmea_log_fd = open( nmea_log.c_str(), O_WRONLY | O_CREAT, S_IRWXU );
            LOGE( "save NMEA raw to:%s ret:%d", nmea_log.c_str(), m_nmea_log_fd );
        }

        // set staticHold
        send_ubx_cfg_cfg(
            CubicCfgGetI( CUBIC_CFG_loc_static_hold_distance ),
            CubicCfgGetI( CUBIC_CFG_loc_static_hold_speed ) );
#ifdef _REMOTE_DEBUGGING_
        // remote debugging server
        m_rd_server.initTcpServer( 59140, "127.0.0.1", 1024 );
        m_rd_server.start();
#endif //_REMOTE_DEBUGGING_
        // start read thread
        return this->start();
    };

    void onDeInit()
    {
        LOGD( "onDeInit" );
#ifdef _REMOTE_DEBUGGING_
        // remote debugging server
        m_rd_server.stop();
        disconnect_rdc();
#endif //_REMOTE_DEBUGGING_
        // stop thread
        this->stop();
        // end parser
        nmea_parser_destroy( &m_gps_parser );
        // power off ublox
        CUtil::WriteFile( CUBIC_ULOC_POWER_PATH, "0", 1 );

        // close port
        if( m_serial_port )
        {
            sp_close( m_serial_port );
            sp_free_port( m_serial_port );
            m_serial_port = NULL;
        }

        if( m_nmea_log_fd > 0 )
        {
            close( m_nmea_log_fd );
        }

        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMeassage n_msg_id : %d", n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_GPS_SET_GEOFENCE:
            LOGD( "CUBIC_MSG_GPS_SET_GEOFENCE" );
            load_gps_fence_setting();
            break;

        case CUBIC_MSG_GPS_ENABLE:
            LOGD( "CUBIC_MSG_GPS_ENABLE" );
            CUtil::WriteFile( CUBIC_ULOC_POWER_PATH, "1", 1 );
            break;

        case CUBIC_MSG_GPS_DISABLE:
            LOGD( "CUBIC_MSG_GPS_DISABLE" );
            CUtil::WriteFile( CUBIC_ULOC_POWER_PATH, "0", 1 );
            break;

        case CUBIC_MSG_GPS_HOT_START:
            LOGD( "CUBIC_MSG_GPS_HOT_START" );
            this->send_ubx_cfg_rst( true );
            break;

        case CUBIC_MSG_GPS_HOT_STOP:
            LOGD( "CUBIC_MSG_GPS_HOT_STOP" );
            this->send_ubx_cfg_rst( false );
            break;
#ifdef _REMOTE_DEBUGGING_

        case PRIVATE_MSG_DISCONNECT:
            LOGD( "PRIVATE_MSG_DISCONNECT" );
            disconnect_rdc();
            break;
#endif //_REMOTE_DEBUGGING_

        default:
            break;
        };

        return 0;
    };

    virtual RunRet run( void *user )
    {
        int ret = 0;

        if( m_MEM_FENCE != 0 )
        {
            LOGE( "memory test failed, overflow !" );
            exit( -1 );
        }

        RETNIF( m_serial_port == NULL, RUN_END );
        // read from serial port
        char buf[CUBIC_ULOC_NMEA_BUF_SZ + 4];
        memset( buf, 0, sizeof( buf ) );
        ret = sp_blocking_read_next( m_serial_port, buf, CUBIC_ULOC_NMEA_BUF_SZ, 1000 );
        RETNIF( needAbort() || ret <= 0, RUN_CONTINUE );
        LOGI( "run ret=%d,buf=%s", ret, buf );

        if( m_nmea_log_fd > 0 )
        {
            write( m_nmea_log_fd, buf, ret );
        }

#ifdef _REMOTE_DEBUGGING_

        // remote debugging connection
        if( m_p_rdc != NULL )
        {
            m_p_rdc->send( buf, ret );
        }

#endif //_REMOTE_DEBUGGING_
        // feed to parser
        nmea_parse( &m_gps_parser, buf, ret, &m_gps_info );
        RETNIF( memcmp( &m_last, &m_gps_info.utc, sizeof( nmeaTIME ) ) == 0, RUN_CONTINUE );
        memcpy( &m_last, &m_gps_info.utc, sizeof( nmeaTIME ) );
        // now we can check current info, if any thing we can update to other app
        update_status();
        // and now to check gps fences
        check_gps_fencs();
        return RUN_CONTINUE;
    };

#ifdef _REMOTE_DEBUGGING_
    // callback for IConnectionListener
    virtual void onAccept( int n_socket, unsigned short n_port, string s_addr )
    {
        LOGD( "onAccept, Remote debugging, new client connect(%s)", s_addr.c_str() );
        disconnect_rdc();
        m_p_rdc = new CTcpConnection( this, n_socket, 1024 );
    };

    // callback for ITcpListener
    virtual void onDisconnected()
    {
        // post delete
        CubicPost( CUBIC_THIS_APP, PRIVATE_MSG_DISCONNECT );
    };

    // callback for ITcpListener
    virtual int onMessage( void *p_data, int n_data_sz )
    {
        RETNIF( m_serial_port == NULL, 0 );
        LOGD( "onMessage, Remote debugging, write data:%d", n_data_sz );
        logout_binary( "onMessage, Remote debugging", ( uint8_t * )p_data, n_data_sz );
        sp_blocking_write( m_serial_port, p_data, n_data_sz, 500 );
        return 0;
    };
#endif //_REMOTE_DEBUGGING_
};

IMPLEMENT_CUBIC_APP( ULocationService )

#endif
