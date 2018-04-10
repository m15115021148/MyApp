/**
 * @file CLogService.cc
 * @author shujie.li
 * @version 1.0
 * @brief Cubic Log Service, record and management log for all module
 * @detail Cubic Log Service, record and management log for all module
 */

#include "cubic_inc.h"
#ifndef _CLOG_SERVICE_CC_
#define _CLOG_SERVICE_CC_ 1

#include <netinet/in.h>
#include <arpa/inet.h>
#include <android/log.h>
#include "CFramework.cc"
#include "CStringTool.cc"
#include "CLogService.h"
#include "CLogFile.cc"
#include "CLogNet.cc"
#include "CRemoteReport.cc"
#include "CAsyncRun.cc"

//#include "msg.h"

using namespace std;



class LogService : public ICubicApp
{
public:
    typedef enum ErrCode
    {
        ERR_NO_ERROR = 0,
        ERR_CREATE_FAIL = -1,
        ERR_SET_OPTION_FAIL = -2,
        ERR_BIND_ADDR_FAIL = -3,
        ERR_LISTENER_ALREADY_START = -4,
        ERR_CREATE_LISTEN_THREAD_FAIL = -5,
        ERR_SOCKET_NOT_READY = -6,
        ERR_WAIT_SOCKET_FAIL = -7,
        ERR_TIMEOUT = -8,
        ERR_DATA_RECV_FAIL = -9,
    } EErrCode;

private:
    static const int INVALID_SOCKET = -1;
    static const int RECV_TIMEOUT = 1000; // ms

    int         m_socket;
    string      m_sock_addr;
    bool        mb_listen_stop_flag;
    pthread_t   m_listen_thread;

    CLogFile   *m_log_file;
    CLogNet    *m_log_net;


protected:
    EErrCode initSocket()
    {
        const unsigned int ASYN_TRUE = 1;
        int ret = 0;

        // ------------ create socket --------------
        if( m_socket == INVALID_SOCKET )
        {
            m_socket = socket( AF_UNIX, SOCK_DGRAM, 0 );
        }

        if( m_socket == INVALID_SOCKET )
        {
            perror( "Create Socket" );
            return ERR_CREATE_FAIL;
        }

        // ------------ set to async ----------------
        ret = ioctl( m_socket, FIONBIO, &ASYN_TRUE );

        if( ret < 0 )
        {
            perror( "Set ASYNC Socket" );
            return ERR_SET_OPTION_FAIL;
        }

        // ------------ bind address ----------------
        struct sockaddr_un t_addr;
        t_addr.sun_family = AF_UNIX;
        strncpy( t_addr.sun_path, m_sock_addr.c_str(), sizeof( t_addr.sun_path ) );
        unlink( m_sock_addr.c_str() );
        ret = bind( m_socket, ( struct sockaddr * )( &t_addr ), ( socklen_t )sizeof( struct sockaddr_un ) );

        if( ret < 0 )
        {
            perror( "Bind Address" );
            return ERR_BIND_ADDR_FAIL;
        }

        return ERR_NO_ERROR;
    };

    void deinitSocket()
    {
        if( m_socket == INVALID_SOCKET )
        {
            return;
        }

        close( m_socket );
        m_socket = INVALID_SOCKET;
        unlink( m_sock_addr.c_str() );
    };

    void initLogger()
    {
        string addr = CubicCfgGetStr( CUBIC_CFG_log_file_path );
        RETIF( addr.length() <= 0 || addr == "null" );
        m_log_file = new CLogFile( addr.c_str() );
        addr = CubicCfgGetStr( CUBIC_CFG_log_net_addr );
        RETIF( addr.length() <= 0 || addr == "null" );
        vector<string> addrs = CStringTool::split( addr, ":" );
        uint16_t port = 51920;

        if( addrs.size() > 1 )
        {
            port = CStringTool::fromString<uint16_t>( addrs[1] );
        }

        m_log_net = new CLogNet( addrs[0], port );
    };

    void deinitLoger()
    {
        DELETE( m_log_file );
        DELETE( m_log_net );
    }

    EErrCode waitData( int &n_socket, int n_timeout )
    {
        int     ret = 0;
        int     maxfd = 0;
        fd_set  fds;
        struct timeval timeout = { n_timeout / 1000, n_timeout % 1000 };
        memset( &fds, 0, sizeof( fd_set ) );

        if( n_socket <= 0 )
        {
            return ERR_SOCKET_NOT_READY;
        }

        /* init fds */
        FD_ZERO( &fds );
        FD_SET( n_socket, &fds );
        maxfd = n_socket;
        maxfd ++;
        /* select read*/
        ret = select( maxfd, &fds, 0, 0, &timeout );

        switch( ret )
        {
        case -1:
            perror( "Select Socket" );
            return ERR_WAIT_SOCKET_FAIL;

        case 0:
            return ERR_TIMEOUT;

        default:
            if( FD_ISSET( n_socket, &fds ) )
            {
                return ERR_NO_ERROR;
            }

            break;
        }

        return ERR_WAIT_SOCKET_FAIL;
    };

    EErrCode recvData( int &n_socket, const TLogMessage &data )
    {
        int ret = 0;
        struct sockaddr_un t_addr;
        socklen_t t_addr_size = sizeof( struct sockaddr_un );
        memset( &t_addr, 0, sizeof( t_addr ) );
        ret = recvfrom( n_socket, ( void * )( &data ), sizeof( TLogMessage ), 0, ( struct sockaddr * )( &t_addr ), &t_addr_size );

        if( ret != sizeof( TLogMessage ) )
        {
            perror( "Recv Data" );
            return ERR_DATA_RECV_FAIL;
        }

        return ERR_NO_ERROR;
    };

    static void *static_procListen( void *p_msger )
    {
        LogService *p_this = ( LogService * )p_msger;

        if( p_this )
        {
            return p_this->procListen();
        }

        return NULL;
    }

    void *procListen()
    {
        EErrCode ret = ERR_NO_ERROR;
        TLogMessage t_message_recv;

        while( !mb_listen_stop_flag )
        {
            // ------------- wait message --------------------
            ret = waitData( m_socket, RECV_TIMEOUT );

            if( ret == ERR_TIMEOUT )
            {
                continue;
            }
            else if( ret != ERR_NO_ERROR )
            {
                break;
            }

            // ------------- recieve ---------------------
            memset( &t_message_recv, 0, sizeof( TLogMessage ) );
            ret = recvData( m_socket, t_message_recv );

            if( ERR_NO_ERROR != ret )
            {
                continue;
            }

#if 0
            // ------------- logto adb port ---------------
            android_LogPriority and_priority = ANDROID_LOG_INFO;

            switch( t_message_recv.n_level )
            {
            case CUBIC_LOG_LEVEL_ERROR:
                and_priority = ANDROID_LOG_ERROR;
                break;

            case CUBIC_LOG_LEVEL_DEBUG:
                and_priority = ANDROID_LOG_DEBUG;
                break;
            };

            __android_log_print( and_priority, t_message_recv.str_tag, t_message_recv.str_data );

#endif

            //            void msg_sprintf(const msg_const_type * const_blk,...);

            // ------------- print out log ---------------------
            if( m_log_file != NULL )
            {
                m_log_file->write( t_message_recv );
            }

            if( m_log_net != NULL )
            {
                m_log_net->send( t_message_recv );
            }
        }

        return NULL;
    };

    EErrCode startListen()
    {
        if( m_listen_thread )
        {
            return ERR_LISTENER_ALREADY_START;
        }

        mb_listen_stop_flag = false;

        if( 0 != pthread_create( &m_listen_thread, NULL, LogService::static_procListen, this ) )
        {
            perror( "Create Listen Thread" );
            return ERR_CREATE_LISTEN_THREAD_FAIL;
        }

        return ERR_NO_ERROR;
    };

    void stopListen()
    {
        if( !m_listen_thread )
        {
            return;
        }

        mb_listen_stop_flag = true;
        pthread_join( m_listen_thread, NULL );
        m_listen_thread = 0;
    };

    static void upload_log( void *data )
    {
        CRemoteReport::uploadDeviceLog( CUBIC_BLE_UPLOAD_LOG_PATH );
    };

public:
    LogService()
        : m_socket( INVALID_SOCKET )
        , m_sock_addr( LOG_SERVICE_ADDR )
        , mb_listen_stop_flag( false )
        , m_listen_thread( 0 )
        , m_log_file( NULL )
        , m_log_net( NULL )
    {};

    ~LogService()
    {};

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        switch ( n_msg_id )
        {
        case CUBIC_MSG_BLE_UPLOAD_LOG:// upload device log
            LOGD( "onMessage: CUBIC_MSG_BLE_UPLOAD_LOG" );
            CAsyncRun<>::async_run( upload_log );
            break;

        default:
            break;
        }

        return 0;
    };

    virtual bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        curl_global_init( CURL_GLOBAL_ALL );
        initSocket();
        LOGD( "initLogger" );
        initLogger();
        LOGD( "startListen" );
        return ( startListen() == ERR_NO_ERROR );
    };

    virtual void onDeInit()
    {
        LOGD( "onDeInit" );
        LOGD( "stopListen" );
        stopListen();
        LOGD( "deinitSocket" );
        deinitSocket();
        LOGD( "deinitLoger" );
        deinitLoger();
    };
};

IMPLEMENT_CUBIC_APP( LogService )

#endif // _CLOG_SERVICE_CC_
