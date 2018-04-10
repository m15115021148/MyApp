/**
 * @file: CTcpServer.cc
 * @date: 2015-08-17
 * @auth: thirchina
 * @brif: a tcp server impelement
 */
#ifndef _C_TCP_SERVER_CC_
#define _C_TCP_SERVER_CC_ 1

class IConnectionListener
{
public:
    virtual void onAccept( int n_socket, unsigned short n_port, string s_addr ) = 0;
};

class CTcpServer
{
private:
    static const int SOCKET_WAIT_SPAN = 1000;
    int m_socket;
    IConnectionListener *m_listener;
    bool m_listen_stop_flag;
    pthread_t m_listen_thread;

    void procListen()
    {
        if( m_socket <= 0 && m_listener == NULL )
        {
            return;
        }

        while( !m_listen_stop_flag )
        {
            static const unsigned long timeout_sec = SOCKET_WAIT_SPAN / 1000;
            static const unsigned long timeout_usec = ( SOCKET_WAIT_SPAN % 1000 ) * 1000;
            struct timeval timeout = { timeout_sec, timeout_usec };
            int ret = 0;
            int maxfd = 0;
            fd_set  fds;
            /* init fds */
            FD_ZERO( &fds );
            FD_SET( m_socket, &fds );
            maxfd = m_socket;
            maxfd ++;
            /* select read*/
            ret = select( maxfd, &fds, 0, 0, &timeout );

            switch( ret )
            {
            case -1: // error
                perror( "Select Socket" );
                return;

            case 0: // timeout
                break;

            default:
                if( FD_ISSET( m_socket, &fds ) )
                {
                    if( m_listener )
                    {
                        struct sockaddr_in t_addr;
                        int n_addr_len = sizeof( t_addr );
                        int new_socket = -1;
                        new_socket = accept( m_socket, ( struct sockaddr * )( &t_addr ), ( socklen_t * )( &n_addr_len ) );

                        if( new_socket <= 0 )
                        {
                            perror( "Accept Fail" );
                            break;
                        }

                        m_listener->onAccept( new_socket, t_addr.sin_port, inet_ntoa( t_addr.sin_addr ) );
                    }
                }

                break;
            }
        }
    }

    static void *static_procListen( void *data )
    {
        CTcpServer *p_this = ( CTcpServer * )data;
        p_this->procListen();
        return NULL;
    }

public:
    CTcpServer( IConnectionListener *p_listener )
        : m_socket( -1 )
        , m_listener( p_listener )
        , m_listen_stop_flag( false )
        , m_listen_thread( 0 )
    {
    }

    ~CTcpServer()
    {
        this->stop();
    }

    void initTcpServer( unsigned short port, const char *addr, unsigned int recv_buf_sz )
    {
        int ret = 0;
        // ------------ create socket ----------------
        m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

        if( m_socket <= 0 )
        {
            perror( "Create Socket" );
            return;
        }

        // --------- set socket recv buf size --------------
        ret = setsockopt( m_socket, SOL_SOCKET, SO_RCVBUF, &recv_buf_sz, sizeof( recv_buf_sz ) );

        if( ret != 0 )
        {
            perror( "Set Socket Recieve Buffer Size" );
            return;
        }

        while( 1 )
        {
            // ------------ set address option ----------
            int option = 1;
            ret = setsockopt( m_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof( option ) );

            if( ret < 0 )
            {
                perror( "Set Address Option" );
                usleep( 500 );
                continue;
            }

            // ------------ bind address ----------------
            sockaddr_in t_addr;
            memset( &t_addr, 0, sizeof( t_addr ) );
            t_addr.sin_family = AF_INET;
            t_addr.sin_port = htons( port );
            t_addr.sin_addr.s_addr = inet_addr( addr );
            ret = bind( m_socket, ( struct sockaddr * )( &t_addr ), ( socklen_t )sizeof( t_addr ) );

            if( ret < 0 )
            {
                perror( "Bind Address" );
                usleep( 500 );
                continue;
            }

            // ------------ listen ----------------
            ret = listen( m_socket, 1 );

            if( ret == 0 )
            {
                break;
            }
            else
            {
                perror( "Listen Port" );
            }

            usleep( 500 );
            close( m_socket );
            m_socket = -1;
        }
    };

    bool start()
    {
        if( m_socket <= 0 )
        {
            fprintf( stderr, "socket is not ready!" );
            return false;
        }

        if( m_listen_thread )
        {
            fprintf( stderr, "Listen thread already start!" );
            return true;
        }

        m_listen_stop_flag = false;

        if( 0 != pthread_create( &m_listen_thread, NULL, static_procListen, this ) )
        {
            perror( "Create Listen Thread" );
            return false;
        }

        return true;
    };

    void stop()
    {
        if( !m_listen_thread )
        {
            return;
        }

        m_listen_stop_flag = true;
        pthread_join( m_listen_thread, NULL );
        m_listen_thread = 0;

        if( m_socket > 0 )
        {
            shutdown( m_socket, SHUT_RDWR );
            close( m_socket );
            m_socket = -1;
        }
    }
};

#endif //_C_TCP_SERVER_CC_
