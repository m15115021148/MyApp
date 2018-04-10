/**
 * @file SoundService.cc
 * @author lishujie
 * @version 1.0
 * @brief Light Service
 * @detail Light Service
 */

#include "CFramework.cc"
#include "cubic_inc.h"
#include <iostream>

#include "CAsyncRun.cc"
#include "SoundPlayer.cc"
#include "SoundSourceDecoder.cc"
#include "SoundSourceSip.cc"
#include "SoundRecorder.cc"
#include "SoundDestinationEncoder.cc"
#include "SoundDestinationEncOgg.cc"
#include "SoundDestinationSip.cc"


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundService"



class SoundService : public ICubicApp, public IDecoderListener
{
private:
    ISoundSource               *m_suond_source;
    ISoundDestination          *m_sound_destination;
    string                      m_sound_destination_url;

    void stopPlay()
    {
        SoundPlayer::getInstance().stop();
        DELETE( m_suond_source );
    };

    bool playAudio( const string &url, bool loop = false )
    {
        if( SoundPlayer::getInstance().isRunning() )
        {
            cubic_msg_play_done arg;
            memset( &arg, 0, sizeof( cubic_msg_play_done ) );
            arg.complete = 0;
            arg.error = 0;
            strncpy( arg.path, url.c_str(), CUBIC_PATH_MAX );
            CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_PLAY_DONE, arg );
        }

        stopPlay();
        m_suond_source = new SoundSourceDecoder( url, this, loop );
        RETNIF_LOGE( m_suond_source == NULL, false, "playAudio failed to create SoundSourceDecoder !" );
        RETNIF_LOGE( !SoundPlayer::getInstance().start( m_suond_source ), false, "playAudio failed to start play !" );
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_SOUND_PLAY);
        return true;
    };

    void stopRecord()
    {
        SoundRecorder::getInstance().stop();
        DELETE( m_sound_destination );
    };

    bool recordAudio( const string &url )
    {
        stopRecord();
        m_sound_destination = new SoundDestinationEncoder( url );
        //m_sound_destination = new SoundDestinationEncOgg(url);
        RETNIF_LOGE( m_sound_destination == NULL, false, "recordAudio failed to create SoundDestinationEncoder !" );
        RETNIF_LOGE( !SoundRecorder::getInstance().start( m_sound_destination ), false, "recordAudio failed to start record !" );
        m_sound_destination_url = url;
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_SOUND_RECORD);
        return true;
    };

    bool setupSipSound()
    {
        LOGD( "setupSipSound" );
        stopRecord();
        stopPlay();
        m_suond_source = new SoundSourceSip();
        RETNIF_LOGE( m_suond_source == NULL, false, "setupSipSound failed to create SoundSourceSip !" );
        RETNIF_LOGE( !SoundPlayer::getInstance().start( m_suond_source ), false, "setupSipSound failed to start play !" );
        m_sound_destination = new SoundDestinationSip();
        RETNIF_LOGE( m_sound_destination == NULL, false, "setupSipSound failed to create SoundDestinationSip !" );
        RETNIF_LOGE( !SoundRecorder::getInstance().start( m_sound_destination ), false, "setupSipSound failed to start record !" );
        return true;
    };

    void reportPlayDone( const string &url, int complete, int error )
    {
        cubic_msg_play_done arg;
        memset( &arg, 0, sizeof( cubic_msg_play_done ) );
        arg.complete = 1;
        arg.error = 0;
        strncpy( arg.path, url.c_str(), CUBIC_PATH_MAX );
        CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_PLAY_DONE, arg );
        CubicWakeupLockClear(CUBIC_WAKELOCK_ID_SOUND_PLAY);
    }

    void reportRecordDone( const string &url, int complete, int error )
    {
        cubic_msg_record_done arg;
        memset( &arg, 0, sizeof( cubic_msg_record_done ) );
        arg.complete = 1;
        arg.error = 0;
        strncpy( arg.path, url.c_str(), CUBIC_PATH_MAX );
        CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_RECORD_DONE, arg );
        CubicWakeupLockClear(CUBIC_WAKELOCK_ID_SOUND_RECORD);
    }

public:
    SoundService()
        : m_suond_source( NULL )
        , m_sound_destination( NULL )
        , m_sound_destination_url()
    {
    };

    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        CubicStatSet( CUBIC_STAT_snd_play, ( int )0 );
        CubicStatSet( CUBIC_STAT_snd_record, ( int )0 );
        return true;
    };

    void onDeInit()
    {
        LOGD( "%s onDeInit", CUBIC_THIS_APP );
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMessage: %s, %d", str_src_app_name.c_str(), n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_SOUND_PLAY_START:
            LOGD( "onMessage: CUBIC_MSG_SOUND_PLAY_START" );
            {
                cubic_msg_sound_play_start *data = ( cubic_msg_sound_play_start * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_SOUND_PLAY_START argument missing !" );

                if( data->path[0] == 0 || strcmp( data->path, "off" ) == 0 )
                {
                    stopPlay();
                    break;
                }

                if( !playAudio( data->path, ( data->loop == 1 ) ) )
                {
                    reportPlayDone( data->path, 0, 1 );
                }
            }
            break;

        case CUBIC_MSG_SOUND_PLAY_STOP:
            LOGD( "onMessage: CUBIC_MSG_SOUND_PLAY_STOP" );
            stopPlay();
            break;

        case CUBIC_MSG_SOUND_RECORD_START:
            LOGD( "onMessage: CUBIC_MSG_SOUND_RECORD_START" );
            {
                cubic_msg_sound_record_start *data = ( cubic_msg_sound_record_start * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_SOUND_RECORD_START argument missing !" );

                if( data->path[0] == 0 || strcmp( data->path, "off" ) == 0 )
                {
                    stopRecord();
                    break;
                }

                if( !recordAudio( data->path ) )
                {
                    reportRecordDone( data->path, 0, 1 );
                }
            }
            break;

        case CUBIC_MSG_SOUND_RECORD_STOP:
            LOGD( "onMessage: CUBIC_MSG_SOUND_RECORD_STOP" );
            BREAKIF( m_sound_destination == NULL );
            stopRecord();
            reportRecordDone( m_sound_destination_url, 1, 0 );
            break;

        case CUBIC_MSG_SOUND_SIP_START:
            LOGD( "onMessage: CUBIC_MSG_SOUND_SIP_START" );
            setupSipSound();
            break;

        case CUBIC_MSG_SOUND_SIP_STOP:
            LOGD( "onMessage: CUBIC_MSG_SOUND_SIP_STOP" );
            stopPlay();
            stopRecord();
            break;

        case CUBIC_MSG_SOUND_ROUTE_PLAY_SPEAKER:
            LOGD( "onMessage: CUBIC_MSG_SOUND_ROUTE_PLAY_SPEAKER" );
            BREAKIF( SoundPlayer::getInstance().getCurrentPath() == SoundPlayer::PATH_SPEAKER );
            SoundPlayer::getInstance().setPath( SoundPlayer::PATH_SPEAKER );
            break;

        case CUBIC_MSG_SOUND_ROUTE_PLAY_EARPHONE:
            LOGD( "onMessage: CUBIC_MSG_SOUND_ROUTE_PLAY_EARPHONE" );
            BREAKIF( SoundPlayer::getInstance().getCurrentPath() == SoundPlayer::PATH_EAR );
            SoundPlayer::getInstance().setPath( SoundPlayer::PATH_EAR );
            break;

        case CUBIC_MSG_SOUND_ROUTE_PLAY_BOTH:
            LOGD( "onMessage: CUBIC_MSG_SOUND_ROUTE_PLAY_BOTH" );
            SoundPlayer::getInstance().setPath( SoundPlayer::PATH_BOTH );
            break;

        case CUBIC_MSG_SOUND_ROUTE_REC_MIC:
            LOGD( "onMessage: CUBIC_MSG_SOUND_ROUTE_REC_MIC" );
            SoundRecorder::getInstance().setPath( SoundRecorder::PATH_MIC );
            break;

        case CUBIC_MSG_SOUND_ROUTE_REC_EARPHONE:
            LOGD( "onMessage: CUBIC_MSG_SOUND_ROUTE_REC_EARPHONE" );
            SoundRecorder::getInstance().setPath( SoundRecorder::PATH_EAR );
            break;

        case CUBIC_MSG_SOUND_VOL_UP:
            LOGD( "onMessage: CUBIC_MSG_SOUND_VOL_UP" );

            if( SoundPlayer::getInstance().setVolume( SoundPlayer::VOL_UP ) == 1 )
            {
                CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            }

            break;

        case CUBIC_MSG_SOUND_VOL_DOWN:
            LOGD( "onMessage: CUBIC_MSG_SOUND_VOL_DOWN" );

            if( SoundPlayer::getInstance().setVolume( SoundPlayer::VOL_DOWN ) == 1 )
            {
                CubicPost( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_DO_VIBRATE );
            }

            break;

        case CUBIC_MSG_SOUND_VOL_MUTE_OR_RESUME:
            LOGD( "onMessage: CUBIC_MSG_SOUND_VOL_MUTE_OR_RESUME" );
            SoundPlayer::getInstance().setVolume( SoundPlayer::VOL_MUTE_OR_RESUME );
            break;

        default:
            break;
        }

        return 0;
    };

    // interface for IDecoderListener
    virtual void onDecodeComplete( const string &url )
    {
        LOGD( "onDecodeComplete: %s", url.c_str() );
        CubicPost( CUBIC_THIS_APP, CUBIC_MSG_SOUND_PLAY_STOP );
        reportPlayDone( url, 1, 0 );
    };
};

IMPLEMENT_CUBIC_APP( SoundService )

