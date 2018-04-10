/**
 * @file SoundDevice.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief sound common definition, and functions
 * @detail sound common definition, and functions
 */
#ifndef _SOUND_COMMON_CC_
#define _SOUND_COMMON_CC_ 1


#include "cubic_const.h"
#include "cubic_inc.h"
#include "SoundDefs.h"

extern "C" {
#include "alsa-intf/alsa_audio.h"
#include "alsa-intf/alsa_ucm.h"
#include "acdbloader/acdb-loader.h"
}

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundDevice"

#define CVD_VERSION_MIXER_CTL           "CVD Version"
#define MAX_CVD_VERSION_STRING_SIZE     100

#define _CUBIC_USE_UCMGR_ 1


class SoundDevice
{
protected:
    static snd_use_case_mgr_t *s_uc_mgr;
    static int s_uc_mgr_open_count;

    SoundDevice()
    {
#if _CUBIC_USE_UCMGR_
        load_acdb();
        ucmgr_open();
#endif //_CUBIC_USE_UCMGR_
    };

    virtual ~SoundDevice()
    {
#if _CUBIC_USE_UCMGR_
        ucmgr_close();
#endif //_CUBIC_USE_UCMGR_
    };

    static void get_cvd_version( char *cvd_version )
    {
        int ret = 0;
        int count;
        struct mixer_ctl *ctl;
        struct mixer *mixer;
        const char *device = "/dev/snd/controlC0";
        mixer = mixer_open( device );

        if ( !mixer )
        {
            fprintf( stderr, "fail to open mixer\n" );
            return;
        }

        ctl = mixer_get_ctl_by_name( mixer, CVD_VERSION_MIXER_CTL );

        if ( !ctl )
        {
            fprintf( stderr, "fail to get mixer ctl\n" );
            goto done;
        }

        mixer_ctl_update( ctl );
        count = mixer_ctl_get_num_values( ctl );

        if( count > MAX_CVD_VERSION_STRING_SIZE )
        {
            count = MAX_CVD_VERSION_STRING_SIZE;
        }

        ret = mixer_ctl_get_array( ctl, cvd_version, count );

        if ( ret != 0 )
        {
            fprintf( stderr, "fail to get mixer_ctl_get_array\n" );
            goto done;
        }

done:
        mixer_close( mixer );
        return;
    }

    static void load_acdb()
    {
        char *cvd_version = NULL;
        cvd_version = ( char * )calloc( 1, MAX_CVD_VERSION_STRING_SIZE );

        if ( !cvd_version )
        {
            fprintf( stderr, "fail to allocate cvd_version\n" );
        }
        else
        {
            get_cvd_version( cvd_version );
        }

        if ( ( acdb_loader_init_v2( NULL, cvd_version, 0 ) ) < 0 )
        {
            fprintf( stderr, "Failed to initialize ACDB\n" );
        }

        if ( cvd_version )
        {
            free( cvd_version );
        }
    }


    static struct mixer_ctl *get_ctl( struct mixer *mixer, const char *name )
    {
        char *p;
        unsigned idx = 0;

        if ( isdigit( name[0] ) )
        {
            return mixer_get_nth_control( mixer, atoi( name ) - 1 );
        }

        struct mixer_ctl *ret = NULL;

        char *name_cp = strdup( name );

        RETNIF( name_cp == NULL, NULL );

        p = strrchr( name_cp, '#' );

        if ( p )
        {
            *p++ = 0;
            idx = atoi( p );
        }

        ret = mixer_get_control( mixer, name_cp, idx );
        free( name_cp );
        return ret;
    };

    static int set_mixer( const char *name, ... )
    {
        static const char  *MIXSER_DEVICE_PATH = "/dev/snd/controlC0";
        static const int    MAX_ARG = 128;
        char               *argv[MAX_ARG];
        int                 argc = 0;
        va_list             parm_list;
        va_start( parm_list, name );

        for( argc = 0; argc < MAX_ARG; argc++ )
        {
            argv[argc] = va_arg( parm_list, char * );
            BREAKIF( argv[argc] == NULL );
        }

        va_end( parm_list );
        struct mixer *mixer;
        struct mixer_ctl *ctl;
        unsigned value;
        int r;
        mixer = mixer_open( MIXSER_DEVICE_PATH );
        RETNIF_LOGE( !mixer, -1, "oops: %s: %d", strerror( errno ), __LINE__ );
        ctl = get_ctl( mixer, name );

        if ( !ctl )
        {
            LOGE( "can't find control" );
            mixer_close( mixer );
            return -2;
        }

        if ( argc )
        {
            if ( isdigit( argv[0][0] ) )
            {
                r = mixer_ctl_set_value( ctl, argc, argv );
            }
            else
            {
                r = mixer_ctl_select( ctl, argv[0] );
            }

            if ( r )
            {
                LOGE( "oops: %s: %d", strerror( errno ), __LINE__ );
            }
        }
        else
        {
            mixer_ctl_get( ctl, &value );
        }

        mixer_close( mixer );
        return 0;
    };

    static void ucmgr_close()
    {
        RETIF( s_uc_mgr_open_count <= 0 );
        s_uc_mgr_open_count --;
        RETIF_LOGE( s_uc_mgr_open_count > 0, "ucmgr_close, open count decrease to %d", s_uc_mgr_open_count );

        if( s_uc_mgr )
        {
            snd_use_case_mgr_close( s_uc_mgr );
        }

        s_uc_mgr = NULL;
        LOGE( "ucmgr_close, sound card closed" );
    };

    static bool ucmgr_open()
    {
        if( s_uc_mgr != NULL )
        {
            s_uc_mgr_open_count++;
            return true;
        }

        int ret = snd_use_case_mgr_open( &s_uc_mgr, CUBIC_UCMGR_NAME );
        RETNIF_LOGE( ret < 0, false, "ucmgr_open, failed to open sound card: %d", ret );
        s_uc_mgr_open_count++;
        LOGE( "ucmgr_open, sound card opened" );
        ucmgr_set( "_verb", "HiFiX" );
        return true;
    };

    static int ucmgr_set( const char *id, const char *val )
    {
        RETNIF_LOGE( s_uc_mgr == NULL, -1, "uc manager not opened !" );
        int ret = snd_use_case_set( s_uc_mgr, id, val );
        RETNIF_LOGE( ret < 0, -2, "failed set parm to sound card: %d, id:%s val:%s", ret, id, val );
        return ret;
    };
};

snd_use_case_mgr_t *SoundDevice::s_uc_mgr = NULL;
int SoundDevice::s_uc_mgr_open_count = 0;

#endif //_SOUND_COMMON_CC_
