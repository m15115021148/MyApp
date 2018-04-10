/**
 * @file CLogFile.cc
 * @author chenyong
 * @version 1.0
 * @brief cubic log outpu file handle
 * @detail cubic log outpu file handle
 */


#ifndef _CFILE_LOG_HANDLE_CC_
#define _CFILE_LOG_HANDLE_CC_ 1

#include <netinet/in.h>
#include <arpa/inet.h>
#include "string.h"
#include <time.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include "CLock.cc"



class CLogFile
{
private:
    static const int BUF_NUM = 32;
    static const int TOTAL_NUM = 16384;
    TLogMessage m_buf[BUF_NUM + 2];
    char m_file_name[PATH_MAX + 4];
    int m_fd;
    int m_pos_w;
    int m_pos_buf;
    CLock m_lock;

    int openLogfile()
    {
        int n_fd;

        if( m_file_name[0] == '\0' )
        {
            return -1;
        }

        n_fd = open( m_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );

        if( n_fd < 0 )
        {
            return -1;
        }

        // read pos
        lseek( n_fd, TOTAL_NUM * sizeof( TLogMessage ), SEEK_SET );
        int ret = ::read( n_fd, &m_pos_w, sizeof( m_pos_w ) );

        if( ret != sizeof( m_pos_w ) ||
                m_pos_w < 0 ||
                m_pos_w > TOTAL_NUM )
        {
            m_pos_w = 0;
            ret = ftruncate( n_fd, sizeof( m_pos_w ) + ( sizeof( TLogMessage ) * TOTAL_NUM ) );

            if( ret < 0 )
            {
                perror( "Truncate File" );
            }
        }

        fprintf( stderr, "Start at m_pos_w=%d\n", m_pos_w );
        fflush( stdout );
        return n_fd;
    };

public:
    class LogRead
    {
    public:
        virtual bool onLog( const TLogMessage &log ) = 0; // return false to stop
        virtual ~LogRead() {};
    };

    CLogFile( const char *file )
        : m_buf()
        , m_file_name()
        , m_fd( -1 )
        , m_pos_w( 0 )
        , m_pos_buf( 0 )
        , m_lock()
    {
        strncpy( m_file_name, file, PATH_MAX );
    };

    virtual ~CLogFile()
    {
        if( m_fd > 0 )
        {
            flush( m_pos_buf );
            close( m_fd );
        }
    };

    void write( TLogMessage &log )
    {
        CLock::Auto lock( m_lock );

        if( m_fd < 0 && ( m_fd = openLogfile() ) < 0 )
        {
            return;
        }

        if( TOTAL_NUM <= m_pos_w )
        {
            m_pos_w = 0;
        }

        lseek( m_fd, m_pos_w * sizeof( TLogMessage ), SEEK_SET );
        int ret = sizeof( TLogMessage );

        if( ret != ::write( m_fd, &log, ret ) )
        {
            return;
        }

        m_pos_w ++;
        // save new write pos
        lseek( m_fd, TOTAL_NUM * sizeof( TLogMessage ), SEEK_SET );
        ret = ::write( m_fd, &m_pos_w, sizeof( m_pos_w ) );

        if( ret != sizeof( m_pos_w ) )
        {
            perror( "Save Pos File" );
        }
    };

    void buffered_write( TLogMessage &log )
    {
        CLock::Auto lock( m_lock );

        if( m_fd < 0 && ( m_fd = openLogfile() ) < 0 )
        {
            return;
        }

        memcpy( &m_buf[m_pos_buf], &log, sizeof( TLogMessage ) );
        m_pos_buf++;

        if( m_pos_buf > BUF_NUM )
        {
            flush( BUF_NUM );
        }
    };

    void read( LogRead *p_logRead )
    {
        if( p_logRead == NULL )
        {
            return;
        }

        if( m_fd < 0 && ( m_fd = openLogfile() ) < 0 )
        {
            return;
        }

        CLock::Auto lock( m_lock );
        TLogMessage tmp;
        int ret = 0;
        int pos = 0;
        fprintf( stderr, "read when total[%d] buf[%d]\n", m_pos_w, m_pos_buf );

        for( int i = 0; i < TOTAL_NUM; i++ )
        {
            // read if m_pos_buf
            if( m_pos_buf > i )
            {
                memcpy( &tmp, &m_buf[m_pos_buf - i - 1], sizeof( TLogMessage ) );
            }
            else
            {
                pos = m_pos_w - ( i - m_pos_buf ) - 1;

                if( pos < 0 )
                {
                    pos += TOTAL_NUM;
                }

                lseek( m_fd, pos * sizeof( TLogMessage ), SEEK_SET );
                ret = ::read( m_fd, &tmp, sizeof( TLogMessage ) );

                if( ret != sizeof( TLogMessage ) )
                {
                    perror( "Read TLogMessage" );
                    break;
                }
            }

            if( tmp.n_level == 0 )
            {
                break;
            }

            if( !p_logRead->onLog( tmp ) )
            {
                break;
            }
        }
    };

    void flush( int num )
    {
        CLock::Auto lock( m_lock );

        if( m_fd < 0 && ( m_fd = openLogfile() ) < 0 )
        {
            return;
        }

        if( num > m_pos_buf )
        {
            num = m_pos_buf;
        }

        fprintf( stderr, "output a buffer when m_pos_w=%d, m_pos_buf=%d\n", m_pos_w, m_pos_buf );
        fflush( stdout );
        lseek( m_fd, m_pos_w * sizeof( TLogMessage ), SEEK_SET );
        int ret = 0;

        if( TOTAL_NUM - m_pos_w < num )
        {
            int first_write = TOTAL_NUM - m_pos_w;
            ret = first_write * sizeof( TLogMessage );

            if( ret != ::write( m_fd, m_buf, ret ) )
            {
                return;
            }

            lseek( m_fd, 0, SEEK_SET );
            ret = ( num - first_write ) * sizeof( TLogMessage );

            if( ret != ::write( m_fd, &m_buf[first_write], ret ) )
            {
                return;
            }

            m_pos_w += num;
            m_pos_w -= TOTAL_NUM;
        }
        else
        {
            ret = num * sizeof( TLogMessage );

            if( ret != ::write( m_fd, m_buf, ret ) )
            {
                return;
            }

            m_pos_w += num;
        }

        m_pos_buf -= num;
        // save new write pos
        lseek( m_fd, TOTAL_NUM * sizeof( TLogMessage ), SEEK_SET );
        ret = ::write( m_fd, &m_pos_w, sizeof( m_pos_w ) );

        if( ret != sizeof( m_pos_w ) )
        {
            perror( "Save Pos File" );
        }
    };


};


#endif

