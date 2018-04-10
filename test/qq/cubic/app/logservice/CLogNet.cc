/**
 * @file CLogNet.cc
 * @author chenyong
 * @version 1.0
 * @brief cubic log outpu network handle
 * @detail cubic log outpu network handle
 */



#ifndef _CNET_LOG_HANDLE_CC_
#define _CNET_LOG_HANDLE_CC_ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include "CLogService.h"
#include "CTcpServer.cc"


class CLogNet : public IConnectionListener
{
private:
    static const int INVALID_SOCKET = -1;
    int             m_socket;
    CTcpServer      m_server;
    FILE           *m_fp; // so we can use fprintf

    void close_conection()
    {
        if( m_fp != NULL )
        {
            fclose( m_fp );
            m_fp = NULL;
        }

        if( m_socket != INVALID_SOCKET )
        {
            close( m_socket );
            m_socket = INVALID_SOCKET;
        }
    };

public:
    CLogNet( const string &listen_addr, uint16_t listen_port )
        : m_socket( INVALID_SOCKET )
        , m_server( this )
        , m_fp( NULL )
    {
        m_server.initTcpServer( listen_port, listen_addr.c_str(), 4 );
        m_server.start();
    };

    virtual ~CLogNet()
    {
        m_server.stop();
        close_conection();
    };

    int send( const TLogMessage &data )
    {
        RETNIF( m_fp == NULL, -1 );
        // format to buffer
        char lev = 'W';

        switch( data.n_level )
        {
        case 0:
        case CUBIC_LOG_LEVEL_ERROR:
            lev = 'E';
            break;

        case CUBIC_LOG_LEVEL_DEBUG:
            lev = 'D';
            break;

        case CUBIC_LOG_LEVEL_INFO:
            lev = 'I';
            break;

        default:
            lev = 'V';
            break;
        }

        if( 0 >= fprintf( m_fp,
                          "[%08lu.%03lu] [%c] [%s] [%s]:%s\n",
                          data.time_stamp.tv_sec,
                          ( data.time_stamp.tv_usec / 1000 ),
                          lev,
                          data.str_appname,
                          data.str_tag,
                          data.str_data ) )
        {
            close_conection();
            return -2;
        }

        fflush( m_fp );
        return 0;
    };

    virtual void onAccept( int n_socket, unsigned short n_port, string s_addr )
    {
        if( m_socket != INVALID_SOCKET )
        {
            close( n_socket ); // reject new connection if old exist
            return;
        }

        m_socket = n_socket;
        m_fp = fdopen( n_socket, "w" );
    };
};


#endif // _CLOGGER_CC_

