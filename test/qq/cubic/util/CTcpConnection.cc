/**
 * @file: CTcpConnection.cc
 * @date: 2015-08-17
 * @auth: thirchina
 * @brif: a tcp connection impelement
 */
#ifndef _C_TCP_CONNECTION_CC_
#define _C_TCP_CONNECTION_CC_ 1

class ITcpListener
{
public:
    virtual void onDisconnected() = 0;
    virtual int onMessage( void *p_data, int n_data_sz ) = 0;
};

class CTcpConnection
{
private:
    static const int SOCKET_WAIT_SPAN = 1000;
    ITcpListener *m_listener;
    int m_socket;
    bool m_listen_stop_flag;
    pthread_t m_listen_thread;
    void *m_recv_buf;
    unsigned int m_recv_buf_sz;

    static void *static_procListen( void *data )
    {
        CTcpConnection *p_this = ( CTcpConnection * )data;
        p_this->procListen();
        return NULL;
    }

    void procListen()
    {
        int ResultSum = 0;

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
            fd_set fds;
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
                if( FD_ISSET( m_socket, &fds ) && ( ( char * )m_recv_buf + ResultSum ) )
                {
                    ret = read( m_socket, ( ( char * )m_recv_buf + ResultSum ), m_recv_buf_sz - ResultSum );

                    if( 0 > ret )
                    {
                        perror( "Read From Socket" );
                        break;
                    }
                    else if( ret == 0 )
                    {
                        disconnect();

                        if( m_listener )
                        {
                            m_listener->onDisconnected();
                        }

                        return;
                    }
                    else if( ( ( char * )m_recv_buf )[ResultSum + ret - 1] == '#' && m_listener )
                    {
                        memset( ( char * )m_recv_buf + ResultSum + ret - 1, 0, m_recv_buf_sz - ( ResultSum + ret ) + 1 );
                        m_listener->onMessage( m_recv_buf, ResultSum + ret - 1 );
                        memset( m_recv_buf, 0, m_recv_buf_sz );
                        ResultSum = 0;
                    }
                    else
                    {
                        ResultSum += ret;
                    }
                }

                break;
            }
        };
    }

    bool start()
    {
        if( m_listen_thread )
        {
            return true;
        }

        if( 0 != pthread_create( &m_listen_thread, NULL, static_procListen, this ) )
        {
            perror( "Create Listen Thread" );
            return false;
        }

        return true;
    }

    void stop()
    {
        if( !m_listen_thread )
        {
            return;
        }

        m_listen_stop_flag = true;
        pthread_join( m_listen_thread, NULL );
        m_listen_thread = 0;
    }

public:
    CTcpConnection( ITcpListener *p_listener, int n_socket, unsigned int recv_buf_sz )
        : m_listener( p_listener )
        , m_socket( n_socket )
        , m_listen_stop_flag( false )
        , m_listen_thread( 0 )
        , m_recv_buf( NULL )
        , m_recv_buf_sz( recv_buf_sz )
    {
        m_recv_buf = malloc( recv_buf_sz );

        if( !m_recv_buf )
        {
            m_recv_buf_sz = 0;
            perror( "Malloc Recieve buffer" );
            return;
        }

        // --------- set socket recv buf size --------------
        int ret = setsockopt( m_socket, SOL_SOCKET, SO_RCVBUF, &recv_buf_sz, sizeof( recv_buf_sz ) );

        if( ret != 0 )
        {
            perror( "Set Socket Recieve Buffer Size" );
            return;
        }

        if( isConnected() )
        {
            start();
        }
    }

    ~CTcpConnection()
    {
        this->disconnect();
    }

    inline bool isConnected()
    {
        return ( m_socket > 0 );
    }

    void disconnect()
    {
        this->stop();

        if( m_socket > 0 )
        {
            close( m_socket );
            m_socket = -1;
        }

        free( m_recv_buf );
        m_recv_buf = NULL;
    }

    int send( void *p_data, int n_data_sz )
    {
        int     ret = 0;
        int     maxfd = 0;
        fd_set  fds;

        if( !this->isConnected() )
        {
            return -1;
        }

        /* init fds */
        FD_ZERO( &fds );
        FD_SET( m_socket, &fds );
        maxfd = m_socket;
        maxfd ++;
        /* select write*/
        ret = select( maxfd, 0, &fds, 0, NULL );

        switch( ret )
        {
        case -1: // error
            perror( "Select Socket" );
            break;

        case 0: // timeout
            fprintf( stderr, "!!!Wait Send write Timeout!!!" );
            break;

        default:
            if( FD_ISSET( m_socket, &fds ) )
            {
                ( ( char * )p_data )[n_data_sz] = '#';
                ret = write( m_socket, p_data, n_data_sz + 1 );

                if( ret != ( n_data_sz + 1 ) )
                {
                    disconnect();

                    if( m_listener )
                    {
                        m_listener->onDisconnected();
                    }
                }
            }

            break;
        }

        return ret;
    }
};

#endif //_C_TCP_CONNECTION_CC_
