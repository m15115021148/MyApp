/**
 * @file CRingBuffer.cc
 * @author shujie.li
 * @version 1.0
 * @brief Cubic Ring Buffer, utility tool, help to buffer byte data, FIFO
 * @detail Cubic Ring Buffer, utility tool, help to buffer byte data, FIFO
 */

#ifndef _CRING_BUFFER_CC_
#define _CRING_BUFFER_CC_ 1

#include "cubic_inc.h"
#include "CLock.cc"
#include <list>

using namespace std;

class CRingBuffer
{
public:
    typedef enum ErrCode
    {
        ERR_NO_ERROR,
        ERR_NOT_READY,
        ERR_BAD_PARRMETER,
        ERR_NOT_TIMEOUT,
        ERR_NOT_ENOUGH,
    } ErrCode;
private:
    unsigned char  *m_buf;
    const int m_size;
    const bool m_over_write;
    int       m_r_pos;
    int       m_w_pos;
    int       m_remain;

    CLock     m_lock;

public:
    CRingBuffer( int size, bool allow_over_write = false )
        : m_buf( NULL )
        , m_size( size )
        , m_over_write( allow_over_write )
        , m_r_pos( 0 )
        , m_w_pos( 0 )
        , m_remain( 0 )
        , m_lock()
    {
        m_buf = ( unsigned char * )malloc( size );

        if( m_buf )
        {
            memset( m_buf, 0, size );
        }
    }

    ~CRingBuffer()
    {
        FREE( m_buf );
    }

    inline int size()
    {
        return m_size;
    };

    inline int remain()
    {
        return m_remain;
    }

    // read only index
    template <class T>
    T touch( int idx ) const
    {
        RETNIF( m_buf == NULL, ( T )0 );
        T ret;
        int offset = sizeof( T ) * idx;
        RETNIF( sizeof( T ) != touch( offset,  &ret, sizeof( T ) ), ( T )0 );
        return ret;
    };

    // read only index
    int touch( int offset, void *buf, int size ) const
    {
        RETNIF( m_buf == NULL, 0 );
        RETNIF( size > m_remain, 0 );

        if( offset + size > m_remain )
        {
            size = m_remain - offset;
        }

        offset += m_r_pos;

        while( offset >= m_size )
        {
            offset -= m_size;
        }

        if( offset + size <= m_size )
        {
            memcpy( buf, m_buf + offset, size );
        }
        else
        {
            int first_size = m_size - offset;
            memcpy( buf, m_buf + offset, first_size );
            memcpy( ( unsigned char * )buf + first_size, m_buf, ( size - first_size ) );
        }

        return size;
    };

    void reset()
    {
        CLock::Auto auto_lock( m_lock );
        m_r_pos = 0;
        m_w_pos = 0;
        m_remain = 0;
    }

    ErrCode put( const void *buf, int size )
    {
        CLock::Auto auto_lock( m_lock );
        RETNIF( m_buf == NULL, ERR_NOT_READY );
        RETNIF( buf == NULL || size > m_size, ERR_BAD_PARRMETER );
        RETNIF( !m_over_write && m_size - m_remain < size, ERR_NOT_ENOUGH );

        if( m_w_pos + size <= m_size )
        {
            memcpy( m_buf + m_w_pos, buf, size );
            m_w_pos += size;
            m_remain += size;
        }
        else if ( size > m_size )
        {
            int first_size = m_size - m_w_pos;
            buf = ( unsigned char * )buf + size - m_size;
            memcpy( m_buf + m_w_pos, buf, first_size );
            memcpy( m_buf, ( unsigned char * )buf + first_size, ( m_size - first_size ) );
            m_w_pos = size - first_size;
            m_remain = m_size;
        }
        else
        {
            int first_size = m_size - m_w_pos;
            memcpy( m_buf + m_w_pos, buf, first_size );
            memcpy( m_buf, ( unsigned char * )buf + first_size, ( size - first_size ) );
            m_w_pos = size - first_size;
            m_remain += size;
        }

        if( m_remain >= m_size )
        {
            m_remain = m_size;
            m_r_pos = m_w_pos;
        }

        return ERR_NO_ERROR;
    }

    template <class T>
    ErrCode put( const T &data )
    {
        return put( &data, sizeof( T ) );
    }

    ErrCode get( void *buf, int size )
    {
        CLock::Auto auto_lock( m_lock );
        RETNIF( m_buf == NULL, ERR_NOT_READY );
        RETNIF( buf == NULL, ERR_BAD_PARRMETER );
        RETNIF( size <= 0, ERR_BAD_PARRMETER );
        RETNIF( m_size < size, ERR_NOT_ENOUGH );
        RETNIF( m_remain < size, ERR_NOT_ENOUGH );

        if( m_r_pos + size <= m_size )
        {
            memcpy( buf, m_buf + m_r_pos, size );
            m_r_pos += size;

            if( m_r_pos >= m_size )
            {
                m_r_pos -= m_size;
            }
        }
        else
        {
            int first_size = m_size - m_r_pos;
            memcpy( buf, m_buf + m_r_pos, first_size );
            memcpy( ( unsigned char * )buf + first_size, m_buf, ( size - first_size ) );
            m_r_pos = size - first_size;
        }

        m_remain -= size;
        return ERR_NO_ERROR;
    }

    template <class T>
    ErrCode get( T &data )
    {
        return get( &data, sizeof( T ) );
    }
};

#endif //_CRING_BUFFER_CC_