/**
 * @file CValuePackage.cc
 * @author  lishujie@landforce
 * @version 1.0
 * @brief Cubic Value Package, tool to push key-value pairs into package, and get key-value pairs from package
 * @detail Cubic Value Package, tool to push key-value pairs into package, and get key-value pairs from package
 */
#ifndef _CVALUE_PACKAGE_CC_
#define _CVALUE_PACKAGE_CC_ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;



class CValuePackage
{
public:
    class IForeach
    {
    public:
        virtual void onValue( const string &key, const string &val ) = 0;
    };

private:
    static const int INIT_BUF_SIZE = 1024;

    unsigned char *m_buf;
    unsigned int   m_buf_size;
    unsigned int   m_data_size;
    unsigned int   m_num;

    inline unsigned char *next( unsigned char *p )
    {
        while( *p++ );

        return p;
    };

    bool expand( unsigned int require_size )
    {
        if( m_buf_size >= require_size )
        {
            return true;
        }

        unsigned int buf_sz = m_buf_size;

        if( buf_sz == 0 || m_buf == NULL )
        {
            buf_sz = INIT_BUF_SIZE;
        };

        // calcutate new size
        while( buf_sz < require_size )
        {
            buf_sz <<= 1;  // twice of before
        }

        if( m_buf )
        {
            // realloc memory
            m_buf = ( unsigned char * )realloc( m_buf, buf_sz );
        }
        else
        {
            // realloc memory
            m_buf = ( unsigned char * )malloc( buf_sz );
        }

        if( !m_buf )
        {
            perror( "Malloc data buffer" );
            return false;
        }

        // zero new memory
        memset( m_buf + m_buf_size, 0, buf_sz - m_buf_size );
        m_buf_size = buf_sz;
        return true;
    };

public:
    CValuePackage()
        : m_buf( NULL )
        , m_buf_size( 0 )
        , m_data_size( 0 )
        , m_num( 0 )
    {
        expand( INIT_BUF_SIZE );
    };

    CValuePackage( const CValuePackage &data )
        : m_buf( NULL )
        , m_buf_size( 0 )
        , m_data_size( 0 )
        , m_num( 0 )
    {
        fromPackage( data );
    };

    CValuePackage( const void *data )
        : m_buf( NULL )
        , m_buf_size( 0 )
        , m_data_size( 0 )
        , m_num( 0 )
    {
        fromData( data );
    };

    ~CValuePackage()
    {
        free( m_buf );
    };

    inline operator void *() const
    {
        return ( void * )m_buf;
    };

    CValuePackage &operator = ( const CValuePackage &data )
    {
        fromPackage( data );
        return *this;
    };

    string operator [] ( const string key )
    {
        return get( key );
    };

    string operator [] ( const int idx )
    {
        return get( idx );
    };

    inline int size()
    {
        return m_data_size;
    };

    void set( const string key, const string val )
    {
        if( !m_buf || key.length() <= 0 )
        {
            return;
        }

        // check size enough or not
        // item: key+'\0'+val+'\0'
        if( !expand( m_data_size + key.length() + val.length() + 2 ) )
        {
            return;
        }

        // save data
        unsigned char *dst = m_buf + m_data_size;
        strncpy( ( char * )dst, key.c_str(), key.length() );
        dst += key.length();
        *dst++ = 0;
        strncpy( ( char * )dst, val.c_str(), val.length() );
        dst += val.length();
        *dst++ = 0;
        m_data_size = dst - m_buf;
        m_num ++;
    };

    void del( const string key )
    {
        unsigned char *item = NULL;

        // find
        for( unsigned char *p = m_buf; ( unsigned int )p - ( unsigned int )m_buf < m_data_size; p = next( next( p ) ) )
        {
            if( key == ( char * )p )
            {
                item = p;
                break;
            }
        }

        if( !item )
        {
            return;
        }

        // delete item, move item behind forward
        int size_of_item = ( next( next( item ) ) - item );
        int size_of_tail = m_data_size - size_of_item - ( item - m_buf );
        memcpy( item, item + size_of_item, size_of_tail );
        memset( item + size_of_tail, 0, size_of_item );
        m_data_size -= size_of_item;
        m_num --;
    };

    void del( const int idx )
    {
        unsigned char *item = NULL;
        int i = 0;

        for( unsigned char *p = m_buf; ( unsigned int )p - ( unsigned int )m_buf < m_data_size; p = next( next( p ) ) )
        {
            if( i == idx )
            {
                item = p;
                break;
            }

            i ++;
        }

        if( !item )
        {
            return;
        }

        // delete item, move item behind forward
        int size_of_item = ( next( next( item ) ) - item );
        int size_of_tail = m_data_size - size_of_item - ( item - m_buf );
        memcpy( item, item + size_of_item, size_of_tail );
        memset( item + size_of_tail, 0, size_of_item );
        m_data_size -= size_of_item;
        m_num --;
    };

    string get( const string key )
    {
        string val;

        if( m_num <= 0 )
        {
            return val;
        }

        for( unsigned char *p = m_buf; ( unsigned int )p - ( unsigned int )m_buf < m_data_size; p = next( next( p ) ) )
        {
            if( key == ( char * )p )
            {
                val = ( char * )next( p );
                break;
            }
        }

        return val;
    };

    string get( const int idx )
    {
        string val;

        if( idx < 0 || m_num <= 0 )
        {
            return val;
        }

        int i = 0;

        for( unsigned char *p = m_buf; ( unsigned int )p - ( unsigned int )m_buf < m_data_size; p = next( next( p ) ) )
        {
            if( i == idx )
            {
                val = ( char * )next( p );
                break;
            }

            i ++;
        }

        return val;
    };

    inline void *getData()
    {
        return ( void * )m_buf;
    };

    void fromPackage( const CValuePackage &data )
    {
        if( !expand( data.m_buf_size ) )
        {
            return;
        }

        m_data_size = data.m_data_size;
        m_num = data.m_num;
        memset( m_buf, 0, m_buf_size );
        memcpy( m_buf, data.m_buf, m_buf_size );
        return;
    };

    bool fromData( const void *data, int size = -1 )
    {
        int num = 0;
        unsigned char *p = ( unsigned char * )data;

        while( *p || ( size > 0 && p - ( unsigned char * )data < size ) )
        {
            num ++;
            p = next( next( p ) ); // skip to next value
        }

        if( num <= 0 )
        {
            return false;
        }

        // calc size
        int data_sz = ( p - ( unsigned char * )data );

        if( !expand( data_sz ) )
        {
            return false;
        }

        // clone buffer data
        memset( m_buf, 0, m_buf_size );
        memcpy( m_buf, data, data_sz );
        // setup attribute
        m_num = num;
        m_data_size = data_sz;
        return true;
    };

    bool exist( const string key )
    {
        if( m_num <= 0 )
        {
            return false;
        }

        for( unsigned char *p = m_buf; ( unsigned int )p - ( unsigned int )m_buf < m_data_size; p = next( next( p ) ) )
        {
            if( key == ( char * )p )
            {
                return true;
            }
        }

        return false;
    };

    inline int num()
    {
        return m_num;
    };

    void foreach( IForeach &handle )
    {
        if( m_num <= 0 )
        {
            return;
        }

        string key;
        string val;

        for( unsigned char *p = m_buf; ( unsigned int )p - ( unsigned int )m_buf < m_data_size; p = next( next( p ) ) )
        {
            key = ( char * )p;
            val = ( char * )next( p );
            handle.onValue( key, val );
        }
    };

    void empty()
    {
        memset( m_buf, 0, m_buf_size );
        m_data_size = 0;
        m_num = 0;
    };
};

#endif //_CVALUE_PACKAGE_CC_
