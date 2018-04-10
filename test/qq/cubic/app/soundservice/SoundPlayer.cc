/**
 * @file SoundPlayer.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief sound player
 * @detail sound player
 */
#ifndef _SOUND_PLAYER_CC_
#define _SOUND_PLAYER_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundSource.cc"
#include "SoundDevice.cc"
#include "CUtil.cc"
#include "CThread.cc"
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <iostream>

extern "C" {
#include "alsa-intf/alsa_audio.h"
#include "alsa-intf/alsa_ucm.h"
}


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "sound_player"

#define CUBIC_EARPHONE_STAT_PATH        "/sys/bus/platform/drivers/tomtom_codec/hph_cur_state"

using namespace std;



class SoundPlayer : protected CThread, public SoundDevice
{
public:
    typedef enum PlayPath
    {
        PATH_NONE = 0,
        PATH_EAR,
        PATH_SPEAKER,
        PATH_BOTH,
    } PlayPath;

    typedef enum VolumeSet
    {
        VOL_UP = 1,
        VOL_DOWN,
        VOL_MUTE_OR_RESUME,
    } VolumeSet;

private:
    ISoundSource    *m_soundsrouce;
    struct pcm      *m_pcm_device;
    uint8_t         *m_buf;
    int              m_curr_vol;
    bool             m_mute;
    PlayPath         m_curr_path;



    static int set_params( struct pcm *pcm )
    {
        struct snd_pcm_hw_params *params;
        struct snd_pcm_sw_params *sparams;
        int channels;

        if( pcm->flags & PCM_MONO )
        {
            channels = 1;
        }
        else if( pcm->flags & PCM_QUAD )
        {
            channels = 4;
        }
        else if( pcm->flags & PCM_5POINT1 )
        {
            channels = 6;
        }
        else if( pcm->flags & PCM_7POINT1 )
        {
            channels = 8;
        }
        else
        {
            channels = 2;
        }

        params = ( struct snd_pcm_hw_params * ) calloc( 1, sizeof( struct snd_pcm_hw_params ) );
        RETNIF_LOGE( !params, -ENOMEM, "Aplay:Failed to allocate ALSA hardware parameters!" );
        param_init( params );
        param_set_mask( params, SNDRV_PCM_HW_PARAM_ACCESS,
                        ( pcm->flags & PCM_MMAP ) ? SNDRV_PCM_ACCESS_MMAP_INTERLEAVED : SNDRV_PCM_ACCESS_RW_INTERLEAVED );
        param_set_mask( params, SNDRV_PCM_HW_PARAM_FORMAT, pcm->format );
        param_set_mask( params, SNDRV_PCM_HW_PARAM_SUBFORMAT,
                        SNDRV_PCM_SUBFORMAT_STD );
        //param_set_min(params, SNDRV_PCM_HW_PARAM_PERIOD_BYTES, CUBIC_DEF_SAMPLE_RATE*CUBIC_SAMPLE_SIZE_IN_BYTE*CUBIC_DEF_CHANNEL_NUM*10/1000);
        param_set_min( params, SNDRV_PCM_HW_PARAM_PERIOD_TIME, 10 );
        param_set_int( params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS, 16 );
        param_set_int( params, SNDRV_PCM_HW_PARAM_FRAME_BITS,
                       pcm->channels * 16 );
        param_set_int( params, SNDRV_PCM_HW_PARAM_CHANNELS,
                       pcm->channels );
        param_set_int( params, SNDRV_PCM_HW_PARAM_RATE, pcm->rate );
        param_set_hw_refine( pcm, params );

        if( param_set_hw_params( pcm, params ) )
        {
            FREE( params );
            LOGE( "Can not set hw params, error:%s", strerror( errno ) );
            return -1;
        }

        pcm->buffer_size = pcm_buffer_size( params );
        pcm->period_size = pcm_period_size( params );
        pcm->period_cnt = pcm->buffer_size / pcm->period_size;
        LOGD ( "period_cnt = %d\n", pcm->period_cnt );
        LOGD ( "period_size = %d\n", pcm->period_size );
        LOGD ( "buffer_size = %d\n", pcm->buffer_size );
        sparams = ( struct snd_pcm_sw_params * ) calloc( 1, sizeof( struct snd_pcm_sw_params ) );
        RETNIF_LOGE ( !sparams, ENOMEM, "Aplay:Failed to allocate ALSA software parameters!" )
        // Get the current software parameters
        sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
        sparams->period_step = 1;
        sparams->avail_min = pcm->period_size / ( channels * 2 ) ;
        sparams->start_threshold =  pcm->period_size / ( channels * 2 ) ;
        sparams->stop_threshold =  pcm->buffer_size ;
        sparams->xfer_align =  pcm->period_size / ( channels * 2 ) ; /* needed for old kernels */
        sparams->silence_size = 0;
        sparams->silence_threshold = 0;

        if( param_set_sw_params( pcm, sparams ) )
        {
            FREE( sparams );
            LOGE( "Can not set sw params, error:%s", strerror( errno ) );
            return -1;
        }

        LOGD ( "sparams->avail_min= %lu", sparams->avail_min );
        LOGD ( "sparams->start_threshold= %lu", sparams->start_threshold );
        LOGD ( "sparams->stop_threshold= %lu", sparams->stop_threshold );
        LOGD ( "sparams->xfer_align= %lu", sparams->xfer_align );
        LOGD ( "sparams->boundary= %lu", sparams->boundary );
        return 0;
    }

    void set_volume( int vol )
    {
        RETIF( vol < 0 || vol > 100 );
        LOGD( "setVolume: %d", vol );
        char str_vol[6] = {0};
        snprintf( str_vol, 6, "%d%%", vol );
        set_mixer( "HPHL Volume", str_vol, NULL );
        set_mixer( "HPHR Volume", str_vol, NULL );
        set_mixer( "RX1 Digital Volume", str_vol, NULL );
        set_mixer( "RX2 Digital Volume", str_vol, NULL );
        set_mixer( "RX7 Digital Volume", str_vol, NULL );
        CubicStatSet( CUBIC_STAT_snd_play_vol, vol );
    };

#if _CUBIC_USE_UCMGR_
    void releasePath( PlayPath path )
    {
        switch( path )
        {
        case PATH_SPEAKER:
            ucmgr_set( "_disdev", "Speaker" );
            break;

        case PATH_EAR:
            ucmgr_set( "_disdev", "Headphones" );
            break;

        case PATH_BOTH:
            ucmgr_set( "_disdev", "Speaker" );
            ucmgr_set( "_disdev", "Headphones" );
            break;

        default:
            break;
        }
    }

    void setupPath( PlayPath path )
    {
        switch( path )
        {
        case PATH_SPEAKER:
            LOGD( "setPath to PATH_SPEAKER" );
            ucmgr_set( "_enadev", "Speaker" );
            break;

        case PATH_EAR:
            LOGD( "setPath to PATH_EAR" );
            ucmgr_set( "_enadev", "Headphones" );
            break;

        case PATH_BOTH:
            LOGD( "setPath to PATH_BOTH" );
            ucmgr_set( "_enadev", "Speaker" );
            ucmgr_set( "_enadev", "Headphones" );
            break;

        default:
            break;
        }
    }
#else // _CUBIC_USE_UCMGR_
    void releasePath( PlayPath path )
    {
        switch( path )
        {
        case PATH_SPEAKER:
            set_mixer( "SLIM RX1 MUX", "ZERO", NULL );
            set_mixer( "RX7 MIX1 INP1", "ZERO", NULL );
            set_mixer( "COMP0 Switch", "0", NULL );
            break;

        case PATH_EAR:
            set_mixer( "SLIM RX2 MUX", "ZERO", NULL );
            set_mixer( "RX1 MIX1 INP1", "ZERO", NULL );
            set_mixer( "RX2 MIX1 INP1", "ZERO", NULL );
            set_mixer( "HPHL DAC Switch", "0", NULL );
            set_mixer( "CLASS_H_DSM MUX", "ZERO", NULL );
            break;

        case PATH_BOTH:
            set_mixer( "SLIM RX1 MUX", "ZERO", NULL );
            set_mixer( "SLIM RX2 MUX", "ZERO", NULL );
            set_mixer( "RX1 MIX1 INP1", "ZERO", NULL );
            set_mixer( "RX2 MIX1 INP1", "ZERO", NULL );
            set_mixer( "RX7 MIX1 INP1", "ZERO", NULL );
            set_mixer( "HPHL DAC Switch", "0", NULL );
            set_mixer( "COMP0 Switch", "0", NULL );
            set_mixer( "CLASS_H_DSM MUX", "ZERO", NULL );
            break;

        default:
            break;
        }
    }

    void setupPath( PlayPath path )
    {
        switch( path )
        {
        case PATH_SPEAKER:
            LOGD( "setPath to PATH_SPEAKER" );
            set_mixer( "PRI_MI2S_RX Audio Mixer MultiMedia1", "1", NULL );
            set_mixer( "MI2S_RX Channels", "One", NULL );
            set_mixer( "SLIM RX1 MUX", "AIF1_PB", NULL );
            set_mixer( "RX7 MIX1 INP1", "RX1", NULL );
            set_mixer( "COMP0 Switch", "1", NULL );
            break;

        case PATH_EAR:
            LOGD( "setPath to PATH_EAR" );
            set_mixer( "PRI_MI2S_RX Audio Mixer MultiMedia1", "1", NULL );
            set_mixer( "MI2S_RX Channels", "Two", NULL );
            set_mixer( "SLIM RX1 MUX", "AIF1_PB", NULL );
            set_mixer( "SLIM RX2 MUX", "AIF1_PB", NULL );
            set_mixer( "RX1 MIX1 INP1", "RX1", NULL );
            set_mixer( "RX2 MIX1 INP1", "RX2", NULL );
            set_mixer( "HPHL DAC Switch", "1", NULL );
            set_mixer( "CLASS_H_DSM MUX", "DSM_HPHL_RX1", NULL );
            break;

        case PATH_BOTH:
            LOGD( "setPath to PATH_BOTH" );
            set_mixer( "PRI_MI2S_RX Audio Mixer MultiMedia1", "1", NULL );
            set_mixer( "MI2S_RX Channels", "Two", NULL );
            set_mixer( "SLIM RX1 MUX", "AIF1_PB", NULL );
            set_mixer( "SLIM RX2 MUX", "AIF1_PB", NULL );
            set_mixer( "RX7 MIX1 INP1", "RX1", NULL );
            set_mixer( "RX1 MIX1 INP1", "RX1", NULL );
            set_mixer( "RX2 MIX1 INP1", "RX2", NULL );
            set_mixer( "HPHL DAC Switch", "1", NULL );
            set_mixer( "COMP0 Switch", "1", NULL );
            set_mixer( "CLASS_H_DSM MUX", "DSM_HPHL_RX1", NULL );
            break;

        default:
            break;
        }
    }
#endif //_CUBIC_USE_UCMGR_

    SoundPlayer()
        : m_soundsrouce( NULL )
        , m_pcm_device( NULL )
        , m_buf( NULL )
        , m_curr_vol( 72 )
        , m_mute( false )
        , m_curr_path( PATH_NONE )
    {
        static const int BUF_SZ = 16;
        char buf[BUF_SZ + 4] = {0};
        int r = CUtil::ReadFile( CUBIC_EARPHONE_STAT_PATH, buf, BUF_SZ );

        if( r > 0 )
        {
            r = strtol( buf, NULL, 10 );
        }

#if _CUBIC_USE_UCMGR_
        ucmgr_set( "_enamod", "Play Music" );
#endif //_CUBIC_USE_UCMGR_

        if( r > 0 )
        {
            setPath( PATH_EAR );
        }
        else
        {
            setPath( PATH_SPEAKER );
        }

        set_volume( m_curr_vol );
    };

public:
    virtual ~SoundPlayer()
    {
        stop();
        releasePath( m_curr_path );
#if _CUBIC_USE_UCMGR_
        ucmgr_set( "_dismod", "Play Music" );
#endif //_CUBIC_USE_UCMGR_
    };

    static SoundPlayer &getInstance()
    {
        static SoundPlayer instance;
        return instance;
    };

    void setPath( PlayPath path )
    {
        LOGD( "setPath: %d when %d", ( int )path, ( int )m_curr_path );
        RETIF( path == m_curr_path );
        set_volume( 0 );
        releasePath( m_curr_path );
        usleep( 100000 );
        setupPath( path );
        set_volume( m_curr_vol );
        m_curr_path = path;

        switch( path )
        {
        case PATH_EAR:
            CubicStatSet( CUBIC_STAT_snd_play_path, "earset" );
            break;

        case PATH_BOTH:
            CubicStatSet( CUBIC_STAT_snd_play_path, "both" );
            break;

        case PATH_SPEAKER:
        default:
            CubicStatSet( CUBIC_STAT_snd_play_path, "body" );
            break;
        }
    };

    int setVolume( VolumeSet volset )
    {
        const int levels[] = {56, 64, 72};
        const int LEVEL_MAX = sizeof( levels ) / sizeof( int );
        int next_level = m_curr_vol;

        if( volset == VOL_MUTE_OR_RESUME )
        {
            LOGD( "setVolume VOL_MUTE_OR_RESUME ==> %s", ( m_mute ? "resume" : "mute" ) );
            set_volume( m_mute ? m_curr_vol : 0 );
            m_mute = !m_mute;
            return 0;
        }

        switch( volset )
        {
        case VOL_UP:
            for( int i = 0; i < LEVEL_MAX; i++ )
            {
                if( levels[i] > m_curr_vol )
                {
                    next_level = levels[i];
                    break;
                }
            }

            break;

        case VOL_DOWN:
            for( int i = LEVEL_MAX - 1; i >= 0; i-- )
            {
                if( levels[i] < m_curr_vol )
                {
                    next_level = levels[i];
                    break;
                }
            }

            break;

        default:
            break;
        }

        if( m_curr_vol == next_level )
        {
            return 1;
        }

        set_volume( next_level );
        m_curr_vol = next_level;
        return 0;
    };

    bool isMuted()
    {
        return m_mute;
    };

    bool start( ISoundSource *soundsource )
    {
        LOGD( "start" );
        // start play
        int flags = 0;
        flags |= PCM_OUT;

        switch ( CUBIC_DEF_CHANNEL_NUM )
        {
        case 1:
            flags |= PCM_MONO;
            break;

        case 4:
            flags |= PCM_QUAD;
            break;

        case 6:
            flags |= PCM_5POINT1;
            break;

        case 8:
            flags |= PCM_7POINT1;
            break;

        default:
            flags |= PCM_STEREO;
            break;
        };

        flags |= DEBUG_ON;

        int format = 0;

        switch( CUBIC_DEF_SAMPLE_FMT )
        {
        case SAMPLE_8_BIT:
            format = SNDRV_PCM_FORMAT_S8;
            break;

        default:
        case SAMPLE_16_BIT:
            format = SNDRV_PCM_FORMAT_S16_LE;
            break;

        case SAMPLE_32_BIT:
            format = SNDRV_PCM_FORMAT_S32_LE;
            break;

        case SAMPLE_32_BIT_FLOAT:
            format = SNDRV_PCM_FORMAT_FLOAT_LE;
            break;

        case SAMPLE_64_BIT_FLOAT:
            format = SNDRV_PCM_FORMAT_FLOAT64_LE;
            break;
        };

        do
        {
            char device[64] = "hw:0,0";
            m_pcm_device = pcm_open( flags, device );
            BREAKIF_LOGE( m_pcm_device == NULL || m_pcm_device < 0 || m_pcm_device->fd < 0, "Can not open PCM devices !" );
            BREAKIF_LOGE( m_pcm_device == NULL || m_pcm_device < 0 || m_pcm_device->fd < 0, "start fail, no available stream !" );
            BREAKIF_LOGE ( !pcm_ready( m_pcm_device ), "PCM is not ready" );
            m_pcm_device->channels = CUBIC_DEF_CHANNEL_NUM;
            m_pcm_device->rate = CUBIC_DEF_SAMPLE_RATE;
            m_pcm_device->flags = flags;
            m_pcm_device->format = format;
            BREAKIF_LOGE ( set_params( m_pcm_device ), "Aplay:Failed in set_params" );
            BREAKIF_LOGE ( pcm_prepare( m_pcm_device ), "Aplay:Failed in pcm_prepare, error:%s", strerror( errno ) );
            // BREAKIF_LOGE( "Aplay:pcm_prepare=%d", pcm_prepare(m_pcm_device) );
            m_buf = ( uint8_t * )malloc( m_pcm_device->period_size );
            BREAKIF_LOGE( m_buf == NULL, "Can not alloc mem for buffer" );
            BREAKIF_LOGE( soundsource == NULL, "start fail, soundsource is empty !" );
            BREAKIF_LOGE( soundsource->setFormat(
                              CUBIC_DEF_CHANNEL_NUM,
                              CUBIC_DEF_SAMPLE_RATE,
                              CUBIC_DEF_SAMPLE_FMT ) < 0,
                          "start fail, set failed !" );
            m_soundsrouce = soundsource;
        }
        while ( 0 );

        if( m_soundsrouce == NULL )
        {
            if( m_pcm_device != NULL )
            {
                pcm_close( m_pcm_device );
            }

            m_pcm_device = NULL;
            FREE( m_buf );
            return false;
        }

        return CThread::start();
    };

    inline PlayPath getCurrentPath()
    {
        return m_curr_path;
    }

    inline bool isRunning()
    {
        return CThread::isRunning() || ( m_soundsrouce != NULL );
    }

    void stop()
    {
        LOGD( "stop" );
        CThread::stop();

        if( m_pcm_device != NULL )
        {
            pcm_close( m_pcm_device );
        }

        m_pcm_device = NULL;
        FREE( m_buf );
        m_soundsrouce = NULL;
        LOGD( "stoped" );
    };

    virtual void onStart( void *user )
    {
        UNUSED_ARG( user );
        CubicStatSet( CUBIC_STAT_snd_play, ( int )1 );
    };

    virtual void onStop( void *user )
    {
        UNUSED_ARG( user );
        CubicStatSet( CUBIC_STAT_snd_play, ( int )0 );
    };

    virtual RunRet run( void *user )
    {
        RETNIF_LOGE( m_soundsrouce == NULL, RUN_END, "No Sound source available !" );
        RETNIF_LOGE( m_pcm_device == NULL, RUN_END, "No Sound device available !" );
        RETNIF_LOGE( m_buf == NULL, RUN_END, "No buffer available !" );
        unsigned int size = m_pcm_device->period_size;
        int ret = m_soundsrouce->getFrame( m_buf, size );

        if( ret <= 0 )
        {
            usleep( 1000 );
            return RUN_CONTINUE;
        }

        pcm_write( m_pcm_device, m_buf, ret );
        return RUN_CONTINUE;
    };

};

#endif //_SOUND_PLAYER_CC_
