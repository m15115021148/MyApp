/**
 * @file SoundRecorder.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief sound recorder
 * @detail sound recorder
 */
#ifndef _SOUND_RECORDER_CC_
#define _SOUND_RECORDER_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundDestination.cc"
#include "SoundDevice.cc"
#include "CUtil.cc"
#include "CThread.cc"
#include <stdint.h>
#include <unistd.h>
#include <iostream>

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "sound_recorder"

class SoundRecorder : protected CThread, public SoundDevice
{
public:
    typedef enum RecordPath
    {
        PATH_NONE = 0,
        PATH_EAR,
        PATH_MIC,
    } RecordPath;

private:
    ISoundDestination  *m_sounddestination;
    struct pcm         *m_pcm_device;
    int                 m_curr_vol;
    bool                m_mute;
    uint8_t            *m_buf;
    RecordPath          m_curr_path;

    static int set_params( struct pcm *pcm )
    {
        struct snd_pcm_hw_params *params;
        struct snd_pcm_sw_params *sparams;
        params = ( struct snd_pcm_hw_params * ) calloc( 1, sizeof( struct snd_pcm_hw_params ) );
        RETNIF_LOGE( !params, -ENOMEM, "set_mixer( :Failed to allocate ALSA hardware parameters!" );
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
        RETNIF_LOGE ( !sparams, ENOMEM, "set_mixer( :Failed to allocate ALSA software parameters!" )
        // Get the current software parameters
        sparams->tstamp_mode = SNDRV_PCM_TSTAMP_NONE;
        sparams->period_step = 1;

        if ( pcm->flags & PCM_MONO )
        {
            sparams->avail_min = pcm->period_size / 2;
            sparams->xfer_align = pcm->period_size / 2;
        }
        else if ( pcm->flags & PCM_QUAD )
        {
            sparams->avail_min = pcm->period_size / 8;
            sparams->xfer_align = pcm->period_size / 8;
        }
        else if ( pcm->flags & PCM_5POINT1 )
        {
            sparams->avail_min = pcm->period_size / 12;
            sparams->xfer_align = pcm->period_size / 12;
        }
        else
        {
            sparams->avail_min = pcm->period_size / 4;
            sparams->xfer_align = pcm->period_size / 4;
        }

        sparams->start_threshold = 1;
        sparams->stop_threshold = INT_MAX;
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

#if _CUBIC_USE_UCMGR_
    void releasePath( RecordPath path )
    {
        switch( m_curr_path )
        {
        case PATH_MIC:
            ucmgr_set( "_disdev", "Line" );
            break;

        case PATH_EAR:
            ucmgr_set( "_disdev", "Headset" );
            break;

        default:
            break;
        }
    }

    void setupPath( RecordPath path )
    {
        switch( path )
        {
        case PATH_MIC:
            ucmgr_set( "_enadev", "Line" );
            break;

        case PATH_EAR:
            ucmgr_set( "_enadev", "Headset" );
            break;

        default:
            break;
        }
    }
#else //_CUBIC_USE_UCMGR_
    void releasePath( RecordPath path )
    {
        switch( m_curr_path )
        {
        case PATH_MIC:
            set_mixer( "AIF1_CAP Mixer SLIM TX7", "1", NULL );
            set_mixer( "SLIM TX7 MUX", "ZERO", NULL );
            set_mixer( "DEC8 MUX", "ZERO", NULL );
            set_mixer( "MultiMedia1 Mixer PRI_MI2S_TX", "0", NULL );
            break;

        case PATH_EAR:
            set_mixer( "AIF1_CAP Mixer SLIM TX7", "0", NULL );
            set_mixer( "SLIM TX7 MUX", "ZERO", NULL );
            set_mixer( "DEC5 MUX", "ZERO", NULL );
            set_mixer( "MultiMedia1 Mixer PRI_MI2S_TX", "0", NULL );
            break;

        default:
            break;
        }
    }

    void setupPath( RecordPath path )
    {
        switch( path )
        {
        case PATH_MIC:
            set_mixer( "AIF1_CAP Mixer SLIM TX7", "1", NULL );
            set_mixer( "MI2S_TX Channels", "One", NULL );
            set_mixer( "SLIM TX7 MUX", "DEC8", NULL );
            set_mixer( "DEC8 MUX", "ADC5", NULL );
            set_mixer( "MultiMedia1 Mixer PRI_MI2S_TX", "1", NULL );
            break;

        case PATH_EAR:
            set_mixer( "AIF1_CAP Mixer SLIM TX7", "1", NULL );
            set_mixer( "MI2S_TX Channels", "One", NULL );
            set_mixer( "SLIM TX7 MUX", "DEC5", NULL );
            set_mixer( "DEC5 MUX", "ADC2", NULL );
            set_mixer( "MultiMedia1 Mixer PRI_MI2S_TX", "1", NULL );
            break;

        default:
            break;
        }
    }
#endif //_CUBIC_USE_UCMGR_

    SoundRecorder()
        : m_sounddestination( NULL )
        , m_pcm_device( NULL )
        , m_curr_vol( 80 )
        , m_mute( false )
        , m_buf( NULL )
        , m_curr_path( PATH_NONE )
    {
#if _CUBIC_USE_UCMGR_
        ucmgr_set( "_enamod", "Capture Music" );
#endif //_CUBIC_USE_UCMGR_
        setPath( PATH_MIC );
        setVolume( m_curr_vol );
    };


public:
    virtual ~SoundRecorder()
    {
        stop();
        releasePath( m_curr_path );
#if _CUBIC_USE_UCMGR_
        ucmgr_set( "_enamod", "Capture Music" );
#endif //_CUBIC_USE_UCMGR_
    };

    void setVolume( int vol )
    {
        RETIF( vol < 0 || vol > 100 );
        char str_vol[6] = {0};
        snprintf( str_vol, 6, "%d%%", vol );
        set_mixer( "DEC8 Volume", str_vol, NULL );
        set_mixer( "ADC5 Volume", str_vol, NULL );
        set_mixer( "DEC5 Volume", str_vol, NULL );
        set_mixer( "ADC2 Volume", str_vol, NULL );
    };

    void setPath( RecordPath path )
    {
        LOGD( "setPath: %d when %d", ( int )path, ( int )m_curr_path );
        RETIF( path == m_curr_path );
        releasePath( m_curr_path );
        usleep( 100000 );
        setupPath( path );
        m_curr_path = path;

        switch( path )
        {
        case PATH_EAR:
            CubicStatSet( CUBIC_STAT_snd_record_path, "earset" );
            break;

        case PATH_MIC:
        default:
            CubicStatSet( CUBIC_STAT_snd_record_path, "body" );
            break;
        }
    };

    static SoundRecorder &getInstance()
    {
        static SoundRecorder instance;
        return instance;
    }

    inline RecordPath getCurrentPath()
    {
        return m_curr_path;
    }

    inline bool isRunning()
    {
        return CThread::isRunning() || ( m_sounddestination != NULL );
    }

    int start( ISoundDestination *sounddestination )
    {
        LOGD( "start" );
        // start play
        int flags = 0;
        flags |= PCM_IN;

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
            BREAKIF_LOGE ( set_params( m_pcm_device ), "Arec:Failed in set_params" );
            BREAKIF_LOGE ( pcm_prepare( m_pcm_device ), "Arec:Failed in pcm_prepare, error:%s", strerror( errno ) );
            // LOGE( "Arec:pcm_prepare=%d", pcm_prepare(m_pcm_device) );
            m_buf = ( uint8_t * )malloc( m_pcm_device->period_size );
            BREAKIF_LOGE( m_buf == NULL, "Can not alloc mem for buffer" );
            BREAKIF_LOGE( sounddestination == NULL, "start fail, sounddestination is empty !" );
            BREAKIF_LOGE( sounddestination->setFormat(
                              CUBIC_DEF_CHANNEL_NUM,
                              CUBIC_DEF_SAMPLE_RATE,
                              CUBIC_DEF_SAMPLE_FMT ) < 0,
                          "start fail, sounddestination set failed !" );
            m_sounddestination = sounddestination;
        }
        while ( 0 );

        if( m_sounddestination == NULL )
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
        m_sounddestination = NULL;
        LOGD( "stoped" );
    };

    virtual void onStart( void *user )
    {
        UNUSED_ARG( user );
        CubicStatSet( CUBIC_STAT_snd_record, ( int )1 );
    };

    virtual void onStop( void *user )
    {
        UNUSED_ARG( user );
        CubicStatSet( CUBIC_STAT_snd_record, ( int )0 );
    };

    virtual RunRet run( void *user )
    {
        RETNIF_LOGE( m_sounddestination == NULL, RUN_END, "No Sound destinition available !" );
        RETNIF_LOGE( m_pcm_device == NULL, RUN_END, "No Sound device available !" );
        RETNIF_LOGE( m_buf == NULL, RUN_END, "No buffer available !" );
        unsigned int size = m_pcm_device->period_size;
        int ret = pcm_read( m_pcm_device, m_buf, size );

        if( 0 > ret )
        {
            usleep( 1000 );
            return RUN_CONTINUE;
        }

        //memset( m_buf, 0, size ); // THIRCHINA TEST CODE
        m_sounddestination->putFrame( m_buf, size );
        //static FILE* sfp = fopen("/tmp/test_rec", "wb+");
        //if(sfp) fwrite(m_buf, size, 1, sfp);
        return RUN_CONTINUE;
    };
};

#endif //_SOUND_RECORDER_CC_
