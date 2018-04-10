/**
 * @file SoundSourceDecoder.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief decoder sound source
 * @detail decoder sound source
 */
#ifndef _SOUND_SOURCE_DECODER_CC_
#define _SOUND_SOURCE_DECODER_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundSource.cc"
#include "CRingBuffer.cc"
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avstring.h"
#include "libavutil/threadmessage.h"
#include "libswresample/swresample.h"

#ifdef __cplusplus
}
#endif //__cplusplus


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundSourceDecoder"

using namespace std;

class IDecoderListener
{
public:
    virtual void onDecodeComplete( const string &url ) = 0;
};

class SoundSourceDecoder : public ISoundSource
{
private:
    CRingBuffer             *m_ring_buffer;
    string                  m_src_url;
    IDecoderListener        *m_listener;

    long                    m_output_channel_num;
    long                    m_output_sample_rate;
    AVSampleFormat          m_output_sample_format;
    AVCodecID               m_output_codec_id;
    AVFormatContext         *m_input_format_ctx;
    AVCodecContext          *m_input_codec_ctx;
    AVCodecContext          *m_output_codec_ctx;
    AVAudioFifo             *m_fifo_buffer;
    SwrContext              *m_swr_ctx;
    AVCodec                 *m_input_codec;
    AVCodec                 *m_output_codec;
    int                     m_input_stream_idx;
    int64_t                 m_pts;
    bool                    m_loop;


    static void ffmpeg_log_callback( void *avclass, int level, const char *fmt, va_list vl )
    {
        int prio = CUBIC_LOG_LEVEL_INFO;
        ( void )( avclass );

        if( level > av_log_get_level() )
        {
            return;
        }

        if( level <= AV_LOG_FATAL )
        {
            prio = CUBIC_LOG_LEVEL_ERROR;
        }
        else if( level <= AV_LOG_ERROR )
        {
            prio = CUBIC_LOG_LEVEL_ERROR;
        }
        else if( level <= AV_LOG_WARNING )
        {
            prio = CUBIC_LOG_LEVEL_DEBUG;
        }
        else if( level <= AV_LOG_INFO )
        {
            prio = CUBIC_LOG_LEVEL_INFO;
        }
        else if( level <= AV_LOG_VERBOSE )
        {
            prio = CUBIC_LOG_LEVEL_INFO;
        }
        else if( level <= AV_LOG_DEBUG )
        {
            prio = CUBIC_LOG_LEVEL_INFO;
        }
        else
        {
            prio = CUBIC_LOG_LEVEL_INFO;
        }

        CFramework::GetInstance().getLoger().logoutV( prio, CUBIC_LOG_TAG":ffmpeg:", fmt, vl );
    };

    bool setup()
    {
        int ret = 0;
        avcodec_register_all();
        av_register_all();
        avformat_network_init();
        // ************************** setup ffmpeg ******************************
        avcodec_register_all();
        av_register_all();
        avformat_network_init();
        // enable log of ffmpeg
        av_log_set_level( AV_LOG_INFO );
        av_log_set_callback( ffmpeg_log_callback );
        // ************************** setup input ******************************
        unsigned int            input_stream_idx = 0;
        // open input
        ret = avformat_open_input( &m_input_format_ctx, m_src_url.c_str(), NULL, NULL );
        RETNIF_LOGE( ret < 0, false, "SoundSource:avformat_open_input fail, ret=%d", ret );
        // find stream and find audio stream
        ret = avformat_find_stream_info( m_input_format_ctx, NULL );
        RETNIF_LOGE( ret < 0, false, "SoundSource:avformat_find_stream_info fail, ret=%d", ret );
        RETNIF_LOGE( m_input_format_ctx->nb_streams < 1, false, "SoundSource:no stream" );

        for( unsigned int i = 0; i < m_input_format_ctx->nb_streams; i++ )
        {
            if( m_input_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
            {
                input_stream_idx = i;
                break;
            }
        }

        LOGD( "SoundSource:Got input_stream_idx=%d", input_stream_idx );
        RETNIF_LOGE( input_stream_idx >= m_input_format_ctx->nb_streams, false, "SoundSource:no audio stream" );
        m_input_stream_idx = input_stream_idx;
        // setup decoder
        m_input_codec = avcodec_find_decoder( m_input_format_ctx->streams[input_stream_idx]->codecpar->codec_id );
        RETNIF_LOGE( m_input_codec == NULL, false, "SoundSource: no fitable codec found for input, require codec:%d", m_input_format_ctx->streams[input_stream_idx]->codecpar->codec_id );
        m_input_codec_ctx = avcodec_alloc_context3( m_input_codec );
        RETNIF_LOGE( m_input_codec_ctx == NULL, false, "SoundSource:alloc decode context failed" );
        ret = avcodec_parameters_to_context( m_input_codec_ctx, m_input_format_ctx->streams[input_stream_idx]->codecpar );
        RETNIF_LOGE( ret < 0, false, "SoundSource:setup codec parameters fail for input, ret=%d", ret );
        ret = avcodec_open2( m_input_codec_ctx, m_input_codec, NULL );
        RETNIF_LOGE( ret < 0, false, "SoundSource:open codec fail for input, ret=%d", ret );
        // ************************** setup output ******************************
        m_output_codec = avcodec_find_encoder( m_output_codec_id/*AV_CODEC_ID_PCM_S16LE*/ );
        RETNIF_LOGE( m_output_codec == NULL, false, "SoundSource:no fitable codec found for output" );
        m_output_codec_ctx = avcodec_alloc_context3( m_output_codec );
        RETNIF_LOGE( m_output_codec_ctx == NULL, false, "SoundSource:alloc decode context failed" );
        m_output_codec_ctx->channels       = m_output_channel_num;
        m_output_codec_ctx->channel_layout = av_get_default_channel_layout( m_output_channel_num );
        m_output_codec_ctx->sample_rate    = m_output_sample_rate;
        m_output_codec_ctx->sample_fmt     = m_output_sample_format;
        m_output_codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
        //m_output_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        LOGD( "SoundSource:0 m_output_codec_ctx->sample_fmt %d", m_output_codec_ctx->sample_fmt );
        LOGD( "SoundSource:1 m_output_codec_ctx->frame_size %d", m_output_codec_ctx->frame_size );
        ret = avcodec_open2( m_output_codec_ctx, m_output_codec, NULL );
        RETNIF_LOGE( ret < 0, false, "SoundSource:open codec fail for output, ret=%d", ret );
        LOGD( "SoundSource: m_output_codec_ctx->frame_size %d",  m_output_codec_ctx->frame_size );

        if( m_output_codec_ctx->frame_size == 0 )
        {
            m_output_codec_ctx->frame_size = m_output_sample_rate * m_output_channel_num * 10 / 1000; // 10ms
        }

        LOGD( "SoundSource: m_output_codec_ctx->frame_size: %d", m_output_codec_ctx->frame_size );
        LOGD( "SoundSource: m_output_codec_ctx->channels: %d",   m_output_codec_ctx->channels );
        LOGD( "SoundSource: m_input_codec_ctx->channels; %d",    m_input_codec_ctx->channels );
        // ************************* setup resampler ****************************
        m_swr_ctx = swr_alloc_set_opts(
                        NULL,
                        av_get_default_channel_layout( m_output_codec_ctx->channels ),
                        m_output_codec_ctx->sample_fmt,
                        m_output_codec_ctx->sample_rate,
                        av_get_default_channel_layout( m_input_codec_ctx->channels ),
                        m_input_codec_ctx->sample_fmt,
                        m_input_codec_ctx->sample_rate,
                        0,
                        NULL );
        RETNIF_LOGE( m_swr_ctx == NULL, false, "SoundSource:create SW resampler fail" );
        ret = swr_init( m_swr_ctx );
        RETNIF_LOGE( ret < 0, false, "SoundSource:init fail for SW resampler, ret:%d", ret );
        // *************************** FIFO buffer  *****************************
        m_fifo_buffer = av_audio_fifo_alloc( m_output_sample_format, m_output_channel_num, 1 );
        RETNIF_LOGE( m_fifo_buffer == NULL, false, "SoundSource:can not alloc FIFO buffer" );
        return true;
    };

    void release()
    {
        // ************************ release all ***********************
        LOGD( "SoundSource:releaseAll()" );

        if( m_swr_ctx != NULL )
        {
            swr_close( m_swr_ctx );
            swr_free( &m_swr_ctx );
        }

        if( m_input_format_ctx != NULL )
        {
            avformat_close_input( &m_input_format_ctx );
            avformat_free_context( m_input_format_ctx );
        }

        if( m_input_codec_ctx != NULL )
        {
            avcodec_close( m_input_codec_ctx );
            avcodec_free_context( &m_input_codec_ctx );
        }

        if( m_output_codec_ctx != NULL )
        {
            avcodec_close( m_output_codec_ctx );
            avcodec_free_context( &m_output_codec_ctx );
        }

        if( m_fifo_buffer != NULL )
        {
            av_audio_fifo_free( m_fifo_buffer );
            m_fifo_buffer = NULL;
        }
    };

    int readInputToDecode()
    {
        int ret;
        int success = -1;
        AVPacket input_packet;
        RETNIF( m_input_format_ctx == NULL, -2 );

        do
        {
            // prepare packet
            av_init_packet( &input_packet );

            // read from input
            while( ( ret = av_read_frame( m_input_format_ctx, &input_packet ) ) == AVERROR( EAGAIN ) );

            BREAKIF_LOGD( ret < 0, "SoundSource:read input failed, ret=%d", ret );
            // write to decoder
            ret = avcodec_send_packet( m_input_codec_ctx, &input_packet );
            BREAKIF_LOGD( ret < 0, "SoundSource:can not save packet, ret=%d", ret );
            success = 0;
        }
        while( 0 );

        av_packet_unref( &input_packet );
        return success;
    };

    // read media packet from input context
    int resampleFrame()
    {
        int ret;
        int success = -1;
        AVFrame *input_frame = NULL;
        uint8_t **covered_samples = NULL;
        RETNIF( m_input_codec_ctx == NULL, -2 );
        RETNIF( m_fifo_buffer == NULL, -3 );
        RETNIF( m_swr_ctx == NULL, -4 );

        do
        {
            // prepare frame
            input_frame  = av_frame_alloc();
            BREAKIF_LOGD( input_frame == NULL, "SoundSource:alloc input frame fail" );
            // read from decoder
            ret = avcodec_receive_frame( m_input_codec_ctx, input_frame );
            BREAKIF( ret < 0 );
            // calculate out sample size
            int n_out_sample_num = input_frame->nb_samples * m_output_codec_ctx->sample_rate / m_input_codec_ctx->sample_rate + 4;
            // prepare temp buffer for result frame
            covered_samples = ( uint8_t ** )calloc( m_output_codec_ctx->channels, sizeof( *covered_samples ) );
            BREAKIF_LOGD( covered_samples == NULL, "SoundSource:cannot alloc temp memory for covered_samples table" );
            memset( covered_samples, 0, m_output_codec_ctx->channels * sizeof( *covered_samples ) );
            ret = av_samples_alloc( covered_samples, NULL, m_output_codec_ctx->channels, n_out_sample_num, m_output_codec_ctx->sample_fmt, 0 );
            BREAKIF_LOGD( ret < 0, "SoundSource:cannot alloc temp memory for covered_samples, ret:%d", ret );
            // do resample
            ret = swr_convert( m_swr_ctx, covered_samples, n_out_sample_num, ( const uint8_t ** )input_frame->extended_data, input_frame->nb_samples );
            BREAKIF_LOGD( ret < 0, "SoundSource:convert frame fail, ret:%d", ret );
            // save result to fifo buffer
            ret = av_audio_fifo_write( m_fifo_buffer, ( void ** )covered_samples, ret );
            BREAKIF_LOGD( ret < 0, "SoundSource:write fifo buffer fail, ret:%d", ret );
            success = 0;
        }
        while( 0 );

        // release memory
        if( covered_samples != NULL )
        {
            if( covered_samples[0] != NULL )
            {
                av_free( covered_samples[0] );
            }

            free( covered_samples );
        }

        av_frame_unref( input_frame );
        av_frame_free( &input_frame );
        return success;
    };


    int flushToEncoder()
    {
        int ret;
        int success = -1;
        AVFrame *output_frame = NULL;
        RETNIF( m_output_codec_ctx == NULL, -2 );
        RETNIF( m_fifo_buffer == NULL, -3 );
        RETNIF( av_audio_fifo_size( m_fifo_buffer ) < m_output_codec_ctx->frame_size, -4 );

        do
        {
            output_frame = av_frame_alloc();
            BREAKIF_LOGD( output_frame == NULL, "SoundSource:alloc output frame fail" );
            output_frame->nb_samples     = FFMIN( av_audio_fifo_size( m_fifo_buffer ), m_output_codec_ctx->frame_size );
            output_frame->channel_layout = m_output_codec_ctx->channel_layout;
            output_frame->format         = m_output_codec_ctx->sample_fmt;
            output_frame->sample_rate    = m_output_codec_ctx->sample_rate;
            output_frame->channels       = m_output_codec_ctx->channels;
            ret = av_frame_get_buffer( output_frame, 0 );
            BREAKIF_LOGD( ret < 0, "SoundSource:get buffer fail for output frame, ret=%d", ret );
            ret = av_audio_fifo_read( m_fifo_buffer, ( void ** )output_frame->data, output_frame->nb_samples );
            BREAKIF_LOGD( ret != output_frame->nb_samples, "SoundSource:read fifo buffer error" );
            output_frame->pts = m_pts;
            m_pts += output_frame->nb_samples;
            ret = avcodec_send_frame( m_output_codec_ctx, output_frame );
            BREAKIF_LOGD( ret < 0, "SoundSource:send frame to encoder failed, ret=%d", ret );
            success = 0;
        }
        while( 0 );

        av_frame_unref( output_frame );
        av_frame_free( &output_frame );
        return success;
    };

    int saveToRingBuffer()
    {
        int ret;
        int success = -1;
        AVPacket output_packet;
        RETNIF( m_input_format_ctx == NULL, -2 );

        do
        {
            // prepare packet
            av_init_packet( &output_packet );
            // read from encoder
            ret = avcodec_receive_packet( m_output_codec_ctx, &output_packet );
            BREAKIF( ret < 0 );
            m_ring_buffer->put( output_packet.data, output_packet.size );
            success = 0;
        }
        while( 0 );

        av_packet_unref( &output_packet );
        return success;
    };

public:
    SoundSourceDecoder( const string &url, IDecoderListener *listener = NULL, bool loop = false )
        : m_ring_buffer( NULL )
        , m_src_url( url )
        , m_listener( listener )
        , m_output_channel_num( 0 )
        , m_output_sample_rate( 0 )
        , m_output_sample_format( AV_SAMPLE_FMT_S16 )
        , m_input_format_ctx( NULL )
        , m_input_codec_ctx( NULL )
        , m_output_codec_ctx( NULL )
        , m_fifo_buffer( NULL )
        , m_swr_ctx( NULL )
        , m_input_codec( NULL )
        , m_output_codec( NULL )
        , m_input_stream_idx( -1 )
        , m_pts( 0 )
        , m_loop( loop )
    {};

    static int64_t getMediaFileDuration( const string &file )
    {
        int ret = 0;
        int64_t duration = 0;
        AVFormatContext *input_format_ctx = NULL;
        avcodec_register_all();
        av_register_all();
        avformat_network_init();
        // open input
        ret = avformat_open_input( &input_format_ctx, file.c_str(), NULL, NULL );
        RETNIF_LOGE( ret < 0, duration, "SoundSource:avformat_open_input fail, ret=%d", ret );
        // find stream and find audio stream
        ret = avformat_find_stream_info( input_format_ctx, NULL );
        RETNIF_LOGE( ret < 0, duration, "SoundSource:avformat_find_stream_info fail, ret=%d", ret );
        duration = input_format_ctx->duration;
        // free context
        avformat_close_input( &input_format_ctx );
        avformat_free_context( input_format_ctx );
        return duration;
    }

    virtual ~SoundSourceDecoder()
    {
        release();
        DELETE( m_ring_buffer );
    }

    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt sample_fmt )
    {
        static const AVSampleFormat tab_sam_fmt[SAMPLE_UNKNOWN] =
        {
            AV_SAMPLE_FMT_U8,
            AV_SAMPLE_FMT_S16,
            AV_SAMPLE_FMT_S32,
            AV_SAMPLE_FMT_FLT,
            AV_SAMPLE_FMT_DBL
        };
        static const AVCodecID tab_codec_id[SAMPLE_UNKNOWN] =
        {
            AV_CODEC_ID_PCM_S8,
            AV_CODEC_ID_PCM_S16LE,
            AV_CODEC_ID_PCM_S32LE,
            AV_CODEC_ID_PCM_F32LE,
            AV_CODEC_ID_PCM_F64LE
        };
        m_output_channel_num = n_channel;
        m_output_sample_rate = sample_rate;
        m_output_sample_format = tab_sam_fmt[sample_fmt];
        m_output_codec_id = tab_codec_id[sample_fmt];
        RETNIF_LOGE(
            m_output_channel_num != 1 &&
            m_output_channel_num != 2,
            -1,
            "bad channel number ! %ld\n",
            m_output_channel_num );
        RETNIF_LOGE(
            m_output_sample_rate != 8000 &&
            m_output_sample_rate != 16000 &&
            m_output_sample_rate != 32000 &&
            m_output_sample_rate != 44100 &&
            m_output_sample_rate != 48000,
            -2,
            "bad sample rate ! %ld\n", m_output_sample_rate );
        release();
        int buffer_size = n_channel * sample_rate * CubicSampleSizeInByte[sample_fmt]; // 1 secound
        DELETE( m_ring_buffer );
        m_ring_buffer = new CRingBuffer( buffer_size );
        RETNIF( m_ring_buffer == NULL, -1 );
        return setup() ? 0 : -2;
    };

    virtual int getFrame( void *buffer, unsigned int buffer_size )
    {
        RETNIF( m_ring_buffer == NULL, -1 );

        if( buffer_size <= ( uint32_t )m_ring_buffer->remain() )
        {
            m_ring_buffer->get( buffer, buffer_size );
            return buffer_size;
        }

        while( this->saveToRingBuffer() == 0
                || this->flushToEncoder() == 0
                || this->resampleFrame() == 0
                || this->readInputToDecode() == 0 )
        {
            if( buffer_size <= ( uint32_t )m_ring_buffer->remain() )
            {
                m_ring_buffer->get( buffer, buffer_size );
                return buffer_size;
            }
        }

        // notify when last data complete
        if( m_ring_buffer->remain() <= 0 )
        {
            if( m_loop && m_input_format_ctx )
            {
                av_seek_frame( m_input_format_ctx, m_input_stream_idx, 0, AVSEEK_FLAG_ANY );
            }
            else if( m_listener != NULL )
            {
                m_listener->onDecodeComplete( m_src_url );
            }
        }

        // no more data, anyway return as we have
        buffer_size = m_ring_buffer->remain();
        RETNIF( buffer_size <= 0, 0 );
        RETNIF( CRingBuffer::ERR_NO_ERROR != m_ring_buffer->get( buffer, buffer_size ), 0 );
        return buffer_size;
    };

};

#endif //_SOUND_SOURCE_RING_BUFFER_CC_
