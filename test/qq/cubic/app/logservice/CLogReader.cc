/**
 * @file CLogReader.cc
 * @author shujie.li
 * @version 1.0
 * @brief Cubic Log Reader, read log from file log
 * @detail Cubic Log Reader, read log from file log
 */

#include "cubic_inc.h"
#include "CLogService.h"
#include "CLogFile.cc"
#include "CConfig.cc"
#include <iostream>

using namespace std;

#define CUBIC_TAGS_MAX 32
static const char *s_cubic_tags_table[][CUBIC_TAGS_MAX] =
{
    //key,          tag 1, tag 2....., NULL
    {"communicate", "BleInterface", "SipService", NULL},
    {"system",      "watchdog", "CoreApp", "LocationService", "NetworkService", "BatService", "Ublox", NULL},
    {NULL},
};


class ReadLog : public CLogFile::LogRead
{
private:
    int             m_limit_num;
    const char    **m_limit_service;
    const char    **m_limit_tags;

    void printlog( const LogMessage &log )
    {
        char lev = 'W';

        switch( log.n_level )
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

        printf( "[%08lu.%03lu] [%c] [%s] [%s]:%s\n",
                log.time_stamp.tv_sec,
                ( log.time_stamp.tv_usec / 1000 ),
                lev,
                log.str_appname,
                log.str_tag,
                log.str_data );
    };

public:
    ReadLog( int limit_num, const char **tags_table )
        : m_limit_num( limit_num - 1 )
        , m_limit_tags( tags_table )
    {};

    virtual bool onLog( const TLogMessage &log )
    {
        RETNIF( m_limit_num < 0, false );

        // unlimit
        if( m_limit_tags == NULL )
        {
            printlog( log );
            m_limit_num--;
            return true;
        }

        // find if match any tag in table
        for( int i = 0; m_limit_tags[i] != NULL; i++ )
        {
            if( strcmp( m_limit_tags[i], log.str_appname ) == 0 )
            {
                printlog( log );
                m_limit_num--;
                return true;
            }
        }

        return true;
    };
};


enum
{
    ARG_PROG = 0,
    ARG_LIMIT,
    ARG_TAG,
    ARG_MAX
};

int main( int argc, const char *argv[] )
{
    int n_limit = 0x7FFFFFFF; // max int
    const char **limit_tags = NULL;

    if( argc > ARG_LIMIT )
    {
        n_limit = strtod( argv[ARG_LIMIT], NULL );
    }

    if( argc > ARG_TAG )
    {
        for( int i = 0; s_cubic_tags_table[i][0] != NULL; i++ )
        {
            if( strcmp( s_cubic_tags_table[i][0], argv[ARG_TAG] ) == 0 )
            {
                limit_tags = &( s_cubic_tags_table[i][1] );
                break;
            }
        }
    }

    ReadLog reader( n_limit, limit_tags );
    CConfig config( CUBIC_CONFIG_PATH );
    string path = config.get( CUBIC_CFG_log_file_path, ( string )CUBIC_LOG_DEF_PATH );
    CLogFile log_file( path.c_str() );
    log_file.read( &reader );
    return 0;
};

