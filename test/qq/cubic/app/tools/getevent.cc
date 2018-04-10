
#include "cubic_inc.h"

#include "CLogger.cc"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include <linux/input.h>

#include <signal.h>
#include <iostream>

using namespace std;


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "getevent"


static int s_stop_flag = 0;

static void main_quit( int sig )
{
    switch( sig )
    {
    case SIGINT:
    case SIGHUP:
    case SIGABRT:
    case SIGTERM:
    case SIGSTOP:
        s_stop_flag = 1;
        break;

    case SIGCHLD:
        break;
    }
};

int main( int argc, const char *argv[] )
{
    CLogger log( CUBIC_LOG_TAG );
    // setup signal handle
    signal( SIGINT,  main_quit );
    signal( SIGHUP,  main_quit );
    signal( SIGABRT, main_quit );
    signal( SIGTERM, main_quit );
    signal( SIGSTOP, main_quit );
    signal( SIGCHLD, main_quit );

    if( argc != 2 )
    {
        printf( "usage: getevent <file-path>\n\n" );
        return -1;
    }

    int m_event_fd = open( argv[1], O_RDONLY );

    if( m_event_fd <= 0 )
    {
        printf( "open event file failed, %s\n", argv[1] );
        return -2;
    }

    while( s_stop_flag == 0 )
    {
        struct input_event evt;
        int evt_size = sizeof( evt );
        int     ret = 0;
        fd_set  fds = {0};
        struct timeval timeout = {0, 10000};
        // ********************************* read event ************************
        // init fds
        FD_ZERO( &fds );
        FD_SET( m_event_fd, &fds );
        // select
        ret = select( m_event_fd + 1, &fds, 0, 0, &timeout );
        CONTINUEIF( ret < 0 );
        CONTINUEIF( ret == 0 );
        CONTINUEIF( !FD_ISSET( m_event_fd, &fds ) );
        CONTINUEIF( read( m_event_fd, &evt, evt_size ) != evt_size );
        printf( "EVENT read event type[%d] code[%d] value[%d], time:%ld.%ld\n",
                evt.type, evt.code, evt.value, evt.time.tv_sec, evt.time.tv_usec );
    };

    close( m_event_fd );

    return 0;
};

