/**
 * @file CServerSocket.cc
 * @author shujie.li
 * @version 1.0
 * @brief Cubic Server Socket, utility Socket tool
 * @detail Cubic Server Socket, utility Socket tool
 */

#ifndef _CSERVERSOCKET_CC_
#define _CSERVERSOCKET_CC_ 1

#include "cubic_inc.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "CLock.cc"

using namespace std;

class ISocketListener
{
public:
    //  when need response, fill p_resp_buf within limit n_resp_buf_sz
    // return size of response data, 0 when no response
    virtual int onMessageReceive(
        const string &s_src_addr,
        unsigned short n_src_port,
        const void *p_in_mesg,
        unsigned int n_in_mesg_sz,
        void *p_resp_buf,
        int n_resp_buf_sz ) = 0;
};


class CServerSocket
{
public:
    static const int RECV_TIMEOUT = 1000; // ms
    static const int SOCKET_BUFFER_MULTIPLE = 32;
    static const int DEF_PACKET_MAX_SIZE = 0x1000; // 4K
    static const int IP_SIZE_MAX = 64;

    typedef enum ErrCode
    {
        ERR_NO_ERROR = 0,
        ERR_CREATE_FAIL = -1,
        ERR_SET_OPTION_FAIL = -2,
        ERR_BIND_ADDR_FAIL = -3,
        ERR_SOCKET_NOT_READY = -4,
        ERR_WAIT_SOCKET_FAIL = -5,
        ERR_TIMEOUT = -6,
        ERR_CREATE_LISTEN_THREAD_FAIL = -7,
        ERR_OVER_SIZE = -8,
        ERR_NULL_POINTER = -9,
        ERR_BAD_ADDR = -10,
        ERR_DATA_SEND_FAIL = -11,
        ERR_LOW_MEMORY = -12,
        ERR_RECV_OVER_SIZE = -13,
        ERR_RECV_FAIL = -14,
        ERR_IP_FORMAI_ERROR = -15,
        ERR_RESP_OVERFLOW = -16,
        ERR_ZERO_SIZE = -17,
        ERR_RESP_ADDR_MISMATCH = -18,
    } EErrCode;

private:
    int m_packet_size_max;
    int m_serv_socket;
    string m_serv_sock_addr;
    ISocketListener *m_listener;

    bool mb_listen_stop_flag;
    pthread_t m_listen_thread;

    CLock m_send_lock;

    EErrCode setSocketOpt( int &n_sock )
    {
        int ret = 0;
        // ------------ set to async ----------------
        const unsigned int ASYN_TRUE = 1;
        ret = ioctl( n_sock, FIONBIO, &ASYN_TRUE );

        if( ret < 0 )
        {
            perror( "Set ASYNC Socket" );
            return ERR_SET_OPTION_FAIL;
        }

        // ------------ set buffer -----------------
        int buff_size = m_packet_size_max * SOCKET_BUFFER_MULTIPLE;
        ret = setsockopt( n_sock, SOL_SOCKET, SO_SNDBUF, ( char * )&buff_size, sizeof( buff_size ) );

        if( ret < 0 )
        {
            perror( "Set Send Buffer" );
            return ERR_SET_OPTION_FAIL;
        }

        ret = setsockopt( n_sock, SOL_SOCKET, SO_RCVBUF, ( char * )&buff_size, sizeof( buff_size ) );

        if( ret < 0 )
        {
            perror( "Set Recv Buffer" );
            return ERR_SET_OPTION_FAIL;
        }

        return ERR_NO_ERROR;
    };

    EErrCode createSocket( int &n_sock, unsigned short n_port )
    {
        int ret = 0;
        RETNIF( n_sock > 0, ERR_NO_ERROR );
        n_sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );

        if( n_sock <= 0 )
        {
            perror( "Create Socket" );
            return ERR_CREATE_FAIL;
        }

        // set socket option
        EErrCode err = setSocketOpt( n_sock );
        RETNIF( err != ERR_NO_ERROR, err );
        // ------------ bind address ----------------
        struct sockaddr_in t_addr;
        t_addr.sin_family = AF_INET;
        t_addr.sin_addr.s_addr = INADDR_ANY;
        t_addr.sin_port = htons( n_port );
        ret = bind( n_sock, ( struct sockaddr * )( &t_addr ), ( socklen_t )sizeof( t_addr ) );

        if( ret < 0 )
        {
            perror( "Bind Address" );
            return ERR_BIND_ADDR_FAIL;
        }

        return ERR_NO_ERROR;
    };

    inline void closeSocket( int &n_sock, string *p_addr = NULL )
    {
        if( n_sock > 0 )
        {
            close( n_sock );
        }

        n_sock = -1;

        if( p_addr && p_addr->length() > 0 )
        {
            unlink( p_addr->c_str() );
            *p_addr = "";
        }
    };

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

    EErrCode senddata(
        int &n_sock,
        const string &s_ip,
        unsigned short n_port,
        const void *p_data,
        int n_size )
    {
        RETNIF( n_sock < 0, ERR_SOCKET_NOT_READY );
        RETNIF( n_size > m_packet_size_max, ERR_OVER_SIZE );
        RETNIF( n_size <= 0, ERR_ZERO_SIZE );
        RETNIF( !p_data, ERR_NULL_POINTER );
        // get address
        struct sockaddr_in t_addr;
        memset( &t_addr, 0, sizeof( t_addr ) );
        t_addr.sin_family = AF_INET;
        t_addr.sin_port = htons( n_port );
        RETNIF( 0 > inet_pton( AF_INET, s_ip.c_str(), &( t_addr.sin_addr ) ), ERR_BAD_ADDR );
        m_send_lock.lock();

        if( n_size != sendto( n_sock, p_data, n_size,  0, ( struct sockaddr * )( &t_addr ), sizeof( t_addr ) ) )
        {
            perror( "Send Data s" );
            m_send_lock.unlock();
            return ERR_DATA_SEND_FAIL;
        }

        m_send_lock.unlock();
        return ERR_NO_ERROR;
    };

    EErrCode recvdata(
        int &n_sock,
        string &s_ip,
        unsigned short &n_port,
        void *p_data,
        int &n_size )
    {
        RETNIF( n_sock < 0, ERR_SOCKET_NOT_READY );
        RETNIF( !p_data, ERR_NULL_POINTER );
        // ------------- recieve ---------------------
        struct sockaddr_in t_addr;
        socklen_t t_addr_size = sizeof( t_addr );
        memset( p_data, 0, n_size );
        n_size = recvfrom( n_sock, p_data, n_size, 0, ( struct sockaddr * )( &t_addr ), &t_addr_size );

        if( 0 >= n_size )
        {
            perror( "Recieve Data" );
            return ERR_RECV_FAIL;
        }

        // ------------- parsing addr -----------------
        n_port = ntohs( t_addr.sin_port );
        char ip[IP_SIZE_MAX] = {0};
        RETNIF( NULL == inet_ntop( AF_INET, &( t_addr.sin_addr ), ip, IP_SIZE_MAX ), ERR_IP_FORMAI_ERROR );
        s_ip = ip;
        return ERR_NO_ERROR;
    };

    void clearBuffer( int &n_sock )
    {
        int ret = 0;
        struct sockaddr *p_addr = NULL;
        socklen_t t_addr_size = 0;
        unsigned char buf[DEF_PACKET_MAX_SIZE];

        if( m_serv_sock_addr.length() == 0 )
        {
            t_addr_size = sizeof( struct sockaddr_in );
        }
        else
        {
            t_addr_size = sizeof( struct sockaddr_un );
        }

        p_addr = ( struct sockaddr * )malloc( t_addr_size );
        RETIF( !p_addr );

        while( 1 )
        {
            socklen_t n_addr_size = t_addr_size;
            ret = recvfrom( n_sock, buf, DEF_PACKET_MAX_SIZE, 0, p_addr, &n_addr_size );
            BREAKIF( ret <= 0 );
        }

        FREE( p_addr );
    };

    void *procListen()
    {
        EErrCode ret = ERR_NO_ERROR;
        void *recv_buffer = NULL;
        void *resp_buffer = NULL;
        string s_addr;
        unsigned short n_port = 0;
        int n_recv_size;
        int n_resp_size;

        // setup recv buffer
        do
        {
            recv_buffer = malloc( m_packet_size_max + 4 );
            BREAKIF( !recv_buffer );
            memset( recv_buffer, 0, m_packet_size_max + 4 );
            // setup resp buffer
            resp_buffer = malloc( m_packet_size_max + 4 );
            BREAKIF( !resp_buffer );
            memset( resp_buffer, 0, m_packet_size_max + 4 );

            while( !mb_listen_stop_flag )
            {
                // ------------- wait message --------------------
                ret = waitData( m_serv_socket, RECV_TIMEOUT );
                CONTINUEIF( ret == ERR_TIMEOUT );
                BREAKIF( ret != ERR_NO_ERROR );
                // --------------- reset buffer ------------------
                n_recv_size = m_packet_size_max;
                memset( recv_buffer, 0, m_packet_size_max + 4 );
                n_resp_size = m_packet_size_max;
                memset( resp_buffer, 0, m_packet_size_max + 4 );

                // ----------------- recieve ---------------------
                if( m_serv_sock_addr.length() == 0 )
                {
                    ret = recvdata( m_serv_socket, s_addr, n_port, recv_buffer, n_recv_size );
                }
                else
                {
                    ret = recvdata( m_serv_socket, s_addr, recv_buffer, n_recv_size );
                }

                CONTINUEIF( ret != ERR_NO_ERROR );
                // ---------------- call listener ----------------
                n_resp_size = m_listener->onMessageReceive( s_addr, n_port, recv_buffer, n_recv_size, resp_buffer, n_resp_size );
                BREAKNIF( n_resp_size >= m_packet_size_max, ret,  ERR_RESP_OVERFLOW );
                // ---------------- no response ------------------
                CONTINUEIF( n_resp_size <= 0 );

                if( m_serv_sock_addr.length() == 0 )
                {
                    senddata( m_serv_socket, s_addr, n_port, resp_buffer, n_resp_size );
                }
                else
                {
                    senddata( m_serv_socket, s_addr, resp_buffer, n_resp_size );
                }
            }
        }
        while( 0 );

        FREE( recv_buffer );
        FREE( resp_buffer );
        return NULL;
    };

    EErrCode createSocket( int &n_sock, string s_addr )
    {
        int ret = 0;
        RETNIF( n_sock > 0, ERR_NO_ERROR );
        n_sock = socket( AF_UNIX, SOCK_DGRAM, 0 );

        if( n_sock <= 0 )
        {
            perror( "Create Socket" );
            return ERR_CREATE_FAIL;
        }

        // set socket option
        EErrCode err = setSocketOpt( n_sock );
        RETNIF( err != ERR_NO_ERROR, err );
        // ------------ bind address ----------------
        struct sockaddr_un t_addr;
        t_addr.sun_family = AF_UNIX;
        strncpy( t_addr.sun_path, s_addr.c_str(), sizeof( t_addr.sun_path ) );
        unlink( s_addr.c_str() );
        ret = bind( n_sock, ( struct sockaddr * )( &t_addr ), ( socklen_t )sizeof( t_addr ) );

        if( ret < 0 )
        {
            perror( "Bind Address" );
            return ERR_BIND_ADDR_FAIL;
        }

        return ERR_NO_ERROR;
    };

    EErrCode senddata(
        int &n_sock,
        const string &s_dst_addr,
        const void *p_data,
        int n_size )
    {
        RETNIF( n_sock < 0, ERR_SOCKET_NOT_READY );
        RETNIF( n_size > m_packet_size_max, ERR_OVER_SIZE );
        RETNIF( n_size <= 0, ERR_ZERO_SIZE );
        RETNIF( !p_data, ERR_NULL_POINTER );
        // get address
        struct sockaddr_un t_addr;
        memset( &t_addr, 0, sizeof( t_addr ) );
        t_addr.sun_family = AF_UNIX;
        strcpy( t_addr.sun_path, s_dst_addr.c_str() );
        m_send_lock.lock();

        if( n_size != sendto( n_sock, p_data, n_size,  0, ( struct sockaddr * )( &t_addr ), sizeof( t_addr ) ) )
        {
            perror( "Send Data s" );
            m_send_lock.unlock();
            return ERR_DATA_SEND_FAIL;
        }

        m_send_lock.unlock();
        return ERR_NO_ERROR;
    };

    EErrCode recvdata(
        int &n_sock,
        string &s_src_name,
        void *p_data,
        int &n_size )
    {
        RETNIF( n_sock < 0, ERR_SOCKET_NOT_READY );
        RETNIF( !p_data, ERR_NULL_POINTER );
        // ------------- recieve ---------------------
        struct sockaddr_un t_addr;
        socklen_t t_addr_size = sizeof( t_addr );
        memset( p_data, 0, n_size );
        n_size = recvfrom( n_sock, p_data, n_size, 0, ( struct sockaddr * )( &t_addr ), &t_addr_size );

        if( 0 >= n_size )
        {
            perror( "Recieve Data" );
            return ERR_RECV_FAIL;
        }

        // ------------- parsing addr -----------------
        s_src_name = t_addr.sun_path;
        return ERR_NO_ERROR;
    };

public:
    CServerSocket(
        unsigned short n_port,
        ISocketListener *listener,
        unsigned int un_packet_size_max = DEF_PACKET_MAX_SIZE )
        : m_packet_size_max( un_packet_size_max )
        , m_serv_socket( -1 )
        , m_serv_sock_addr()
        , m_listener( listener )
        , mb_listen_stop_flag( true )
        , m_listen_thread()
        , m_send_lock()
    {
        RETIF( m_packet_size_max == 0 );

        if( ERR_NO_ERROR != createSocket( m_serv_socket, n_port ) )
        {
            closeSocket( m_serv_socket );
            return;
        }

        // start listener
        if( m_listener )
        {
            start();
        }
    };

    CServerSocket(
        const string &s_srv_sock_addr,
        ISocketListener *listener,
        unsigned int un_packet_size_max = DEF_PACKET_MAX_SIZE )
        : m_packet_size_max( un_packet_size_max )
        , m_serv_socket( -1 )
        , m_serv_sock_addr( s_srv_sock_addr )
        , m_listener( listener )
        , mb_listen_stop_flag( true )
        , m_listen_thread()
        , m_send_lock()
    {
        RETIF( m_packet_size_max == 0 );

        if( ERR_NO_ERROR != createSocket( m_serv_socket, s_srv_sock_addr ) )
        {
            closeSocket( m_serv_socket, &m_serv_sock_addr );
            return;
        }

        // start listener
        if( m_listener )
        {
            start();
        }
    };

    ~CServerSocket()
    {
        stop();
        closeSocket( m_serv_socket, &m_serv_sock_addr );
    };

    static void *static_procListen( void *p_data )
    {
        CServerSocket *p_this = ( CServerSocket * )p_data;

        if( p_this )
        {
            return p_this->procListen();
        }

        return NULL;
    };

    EErrCode start()
    {
        RETNIF( m_serv_socket < 0, ERR_SOCKET_NOT_READY );
        RETNIF( m_listen_thread, ERR_NO_ERROR );
        mb_listen_stop_flag = false;

        if( 0 != pthread_create( &m_listen_thread, NULL, CServerSocket::static_procListen, this ) )
        {
            perror( "Create Listen Thread" );
            return ERR_CREATE_LISTEN_THREAD_FAIL;
        }

        return ERR_NO_ERROR;
    };

    void stop()
    {
        if( !m_listen_thread )
        {
            return;
        }

        mb_listen_stop_flag = true;
        pthread_join( m_listen_thread, NULL );
        m_listen_thread = 0;
    };
};


#endif //_CServerSocket_CC_

