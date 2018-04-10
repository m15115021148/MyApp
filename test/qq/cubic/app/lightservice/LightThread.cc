/**
 * @file LightThread.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief light effect thread
 * @detail light effect thread, to run each light in each step
 */
#ifndef _LIGHT_THREAD_CC_
#define _LIGHT_THREAD_CC_ 1

#include "cubic_inc.h"
#include "light_defs.h"
#include "CThread.cc"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "lightthread"

using namespace std;


class LightThread : public CThread
{
private:
    const int STEP_LEN; // us
    uint8_t **m_light_tab;
    int m_fd_tab[CUBIC_LIGHT_NUM];
    int m_total_step;
    int m_curr_step;


    bool alloc_light_tab( int num )
    {
        m_light_tab = ( uint8_t ** )malloc( num * sizeof( uint8_t * ) );
        RETNIF( !m_light_tab, false );
        memset( m_light_tab, 0, num * sizeof( uint8_t * ) );

        for( int i = 0; i < num; i++ )
        {
            m_light_tab[i] = ( uint8_t * )malloc( CUBIC_LIGHT_NUM * sizeof( uint8_t ) );
            RETNIF( !m_light_tab[i], false );
            memset( m_light_tab[i], 0, CUBIC_LIGHT_NUM * sizeof( uint8_t ) );
        }

        m_total_step = num;
        return true;
    };

    void release_light_tab()
    {
        if( m_light_tab )
        {
            for( int i = 0; i < m_total_step; i++ )
            {
                FREE( m_light_tab[i] );
            }

            free( m_light_tab );
            m_light_tab = NULL;
        }
    }

    void setup_device()
    {
        LOGD( "setup_device" );

        for( int i = 0; i < CUBIC_LIGHT_NUM; i++ )
        {
            CONTINUEIF( CUBIC_LIGHT_PATH[i] == NULL );
            m_fd_tab[i] = open( CUBIC_LIGHT_PATH[i], O_RDWR );

            if( m_fd_tab[i] <= 0 )
            {
                perror( "open file failed!" );
                LOGE( "can not open light file: %s", CUBIC_LIGHT_PATH[i] );
                continue;
            }
        }
    }

    void release_device()
    {
        char buf[2] = {'0', 0};

        for( int i = 0; i < CUBIC_LIGHT_NUM; i++ )
        {
            if( m_fd_tab[i] > 0 )
            {
                write( m_fd_tab[i], buf, 2 );
                close( m_fd_tab[i] );
                m_fd_tab[i] = -1;
            }
        }
    }

public:
    LightThread( const CubicLightKey light_define[CUBIC_LIGHT_NUM][CUBIC_LIGHT_KEY_MAX], int step_len = 20000 )
        : STEP_LEN( step_len )
        , m_light_tab( NULL )
        , m_fd_tab()
        , m_total_step( 0 )
        , m_curr_step( 0 )
    {
        int max_length = 0;
        LOGI( "LightThread  new thread" );

        for( int i = 0; i < CUBIC_LIGHT_NUM; i++ )
        {
            for( int j = 0; j < CUBIC_LIGHT_KEY_MAX; j++ )
            {
                if( light_define[i][j].moment > max_length )
                {
                    max_length = light_define[i][j].moment;
                }
            }
        }

        RETIF_LOGE( max_length <= 0, "light define is not valid" );
        RETIF_LOGE( !alloc_light_tab( max_length * 1000 / STEP_LEN ), "alloc memory failed !" );
        RETIF_LOGE( m_light_tab == NULL, "light table alloc failed !" );
        LOGD( "LightThread max_length=%d, m_total_step=%d", max_length, m_total_step );

        for( int light_id = 0; light_id < CUBIC_LIGHT_NUM; light_id++ )
        {
            for( int key = 1; key < CUBIC_LIGHT_KEY_MAX; key++ )
            {
                BREAKIF( light_define[light_id][key].brightness <= CUBIC_LIGHT_BRIGHTNESS_INVALID );
                BREAKIF( light_define[light_id][key].moment <= light_define[light_id][key - 1].moment );
                int step_num = ( light_define[light_id][key].moment - light_define[light_id][key - 1].moment ) * 1000 / STEP_LEN;
                double step_len = ( double )( light_define[light_id][key].brightness - light_define[light_id][key - 1].brightness ) / step_num;
                ;
                double v = light_define[light_id][key - 1].brightness;
                int base = light_define[light_id][key - 1].moment * 1000 / STEP_LEN;

                for( int pos = 0; pos < step_num; pos++, v += step_len )
                {
                    if( v > CUBIC_LIGHT_BRIGHTNESS_MAX )
                    {
                        v = CUBIC_LIGHT_BRIGHTNESS_MAX;
                    }

                    m_light_tab[base + pos][light_id] = ( uint8_t )v;
                }
            }
        }

        setup_device();
    };


    virtual ~LightThread()
    {
        LOGI( "LightThread  release thread" );
        this->stop();
        release_light_tab();
        release_device();
    };

    virtual RunRet run( void *user )
    {
        UNUSED_ARG( user );
        RETNIF( !m_light_tab || m_total_step <= 0, CThread::RUN_END );

        if( m_curr_step >= m_total_step || m_curr_step < 0 )
        {
            m_curr_step = 0;
        }

        char buf[4 + 4] = {0};
        int len = 0;

        for( int i = 0; i < CUBIC_LIGHT_NUM; i++ )
        {
            CONTINUEIF( m_fd_tab[i] <= 0 );
            len = snprintf( buf, 4, "%d", m_light_tab[m_curr_step][i] );
            write( m_fd_tab[i], buf, len );
        }

        m_curr_step ++;
        usleep( STEP_LEN );
        return CThread::RUN_CONTINUE;
    };

    void runOneRound()
    {
        while( !needAbort() &&
                m_curr_step < m_total_step &&
                run( NULL ) == CThread::RUN_CONTINUE );
    };
};

#endif //_LIGHT_THREAD_CC_
