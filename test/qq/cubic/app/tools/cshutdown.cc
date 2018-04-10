/**
 * @file: cshutdown.cc
 * @auth: thirchina
 * @desc: a shutdown tool to power off system or reboot
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <errno.h>
#include <syscall.h>
#include <unistd.h>
#include <linux/reboot.h>

using namespace std;


#define LOGOUT(fmt, ...)  \
    fprintf( stderr, fmt"\n", ##__VA_ARGS__ );

enum
{
    ARG_PROG = 0,
    ARG_ACT,
    ARG_TIMEOUT,
    ARG_TOTAL
};

enum
{
    ACT_SHUT = 0,
    ACT_REBOOT,
    ACT_SHUT_FORCE,
    ACT_REBOOT_FORCE,
    ACT_TOTAL
};

typedef struct ShutdownOption
{
    int act;
    int timeout;
} ShutdownOption;

typedef int ( *pfn_action )( const ShutdownOption *opt );

static int parsing_args_to_options( const char *argv[], ShutdownOption *option );
static int action_shutdown( const ShutdownOption *opt );
static int action_reboot( const ShutdownOption *opt );
static int action_force_shutdown( const ShutdownOption *opt );
static int action_force_reboot( const ShutdownOption *opt );


void usage( const char *name )
{
    LOGOUT( "Usage: %s <action> <time>\n"
            "    action    can be \"s\", \"r\", \"fs\", \"fr\"\n"
            "              \"s\" : shutdown system after <time> seconds\n"
            "              \"r\" : reboot system after <time> seconds\n"
            "              \"fs\": shutdown system now, if <time> reach, force power off\n"
            "              \"fr\": reboot system now, if <time> reach, force reboot\n"
            "    time      seconds value, meanning depend on <action>\n"
            "              zero \"0\" mean imediately\n", name );
};


int main( int argc, const char *argv[] )
{
    static const pfn_action action_table[] =
    {
        action_shutdown,       // ACT_SHUT
        action_reboot,         // ACT_REBOOT
        action_force_shutdown, // ACT_SHUT_FORCE
        action_force_reboot,   // ACT_REBOOT_FORCE
    };
    ShutdownOption option;

    if( argc != ARG_TOTAL )
    {
        usage( argv[ARG_PROG] );
        return -1;
    }

    option.act = ACT_TOTAL;
    option.timeout = 0;

    if( 0 > parsing_args_to_options( argv, &option ) )
    {
        return -1;
    }

    // fork and use child to run action
    pid_t pid = fork();

    if( pid < 0 )
    {
        LOGOUT( "fork child failed: %s", strerror( errno ) );
        return -1;
    }

    // for child, fork agin to grandchild
    if( pid == 0 )
    {
        pid_t pidd = fork();

        if( pidd < 0 )
        {
            LOGOUT( "fork grandchild failed: %s", strerror( errno ) );
            return -1;
        }

        // run task in grandchild
        if( pidd == 0 )
        {
            LOGOUT( "run task in grandchild" );
            return action_table[option.act]( &option );
        }

        // child can exit now
        return 0;
    }

    // for parent exit
    return 0;
};


static int parsing_args_to_options( const char *argv[], ShutdownOption *option )
{
    // parsing arguments
    if( strcmp( argv[ARG_ACT], "s" ) == 0 )
    {
        option->act = ACT_SHUT;
    }

    if( strcmp( argv[ARG_ACT], "r" ) == 0 )
    {
        option->act = ACT_REBOOT;
    }

    if( strcmp( argv[ARG_ACT], "fs" ) == 0 )
    {
        option->act = ACT_SHUT_FORCE;
    }

    if( strcmp( argv[ARG_ACT], "fr" ) == 0 )
    {
        option->act = ACT_REBOOT_FORCE;
    }

    option->timeout = strtol( argv[ARG_TIMEOUT], NULL, 10 );

    // check arguments
    if( option->act == ACT_TOTAL || option->timeout < 0 )
    {
        usage( argv[ARG_PROG] );
        return -1;
    }

    return 0;
};

static int action_shutdown( const ShutdownOption *opt )
{
    //system( "echo 1 > \"/sys/class/leds/aw9523_led_set_green/brightness\"" );
    if( opt->timeout > 0 )
    {
        sleep( opt->timeout );
    }

    return system( "shutdown -hP now" );
};

static int action_reboot( const ShutdownOption *opt )
{
    //system( "echo 1 > \"/sys/class/leds/aw9523_led_set_blue/brightness\"" );
    if( opt->timeout > 0 )
    {
        sleep( opt->timeout );
    }

    return system( "reboot" );
};

static int action_force_shutdown( const ShutdownOption *opt )
{
    system( "shutdown -hP now" );

    //system( "echo 1 > \"/sys/class/leds/aw9523_led_set_white/brightness\"" );
    if( opt->timeout > 0 )
    {
        sleep( opt->timeout );
    }

    // syscall directly
    return syscall( SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, NULL );
};

static int action_force_reboot( const ShutdownOption *opt )
{
    system( "reboot" );

    //system( "echo 1 > \"/sys/class/leds/aw9523_led_set_red/brightness\"" );
    if( opt->timeout > 0 )
    {
        sleep( opt->timeout );
    }

    // syscall directly
    return syscall( SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART, NULL );
};

