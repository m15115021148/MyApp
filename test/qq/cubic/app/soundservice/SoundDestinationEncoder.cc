/**
 * @file SoundDestinationEncoder.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief encoder sound destination
 * @detail encoder sound destination
 */
#ifndef _SOUND_DESTINATION_ENCODER_CC_
#define _SOUND_DESTINATION_ENCODER_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundDestination.cc"
#include "CRingBuffer.cc"
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avstring.h"
#include "libavutil/threadmessage.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>


#ifdef __cplusplus
}
#endif //__cplusplus


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundDesticationEncoder"

using namespace std;

class SoundDestinationEncoder : public ISoundDestination
{
private:
    static const int        MS_PER_STEP = 10;
    string                  m_dst_url;

    long                    m_input_channel_num;
    long                    m_input_sample_rate;
    AVSampleFormat          m_input_sample_format;
    AVCodecID               m_input_codec_id;
    AVCodecContext          *m_input_codec_ctx;
    AVFormatContext         *m_output_format_ctx;
    AVCodecContext          *m_output_codec_ctx;
    AVFilterGraph           *m_filter_graph;
    AVFilterContext         *m_filter_src_ctx;
    AVFilterContext         *m_filter_sink_ctx;
    int64_t                 m_pts;


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
        //vfprintf(stderr, fmt, vl );
    };

    bool setup()
    {
        int ret = 0;
        LOGD( "setup when url=%s", m_dst_url.c_str() );
        char filename[PATH_MAX];
        av_strlcpy( filename, m_dst_url.c_str(), PATH_MAX );
        // ************************** setup ffmpeg ******************************
        avcodec_register_all();
        avfilter_register_all();
        av_register_all();
        avformat_network_init();
        // enable log of ffmpeg
        av_log_set_level( AV_LOG_INFO );
        av_log_set_callback( ffmpeg_log_callback );
        // ************************** setup input ******************************
        AVCodec *input_codec = avcodec_find_decoder( m_input_codec_id );
        RETNIF_LOGE( input_codec == NULL, false, "SoundDestination:can not found codec as id" );
        m_input_codec_ctx = avcodec_alloc_context3( input_codec );
        RETNIF_LOGE( m_input_codec_ctx == NULL, false, "SoundDestination:alloc context fail" );
        m_input_codec_ctx->channels = m_input_channel_num;
        m_input_codec_ctx->channel_layout = av_get_default_channel_layout( m_input_channel_num );
        m_input_codec_ctx->sample_fmt = m_input_sample_format;
        m_input_codec_ctx->sample_rate =  m_input_sample_rate;
        ret = avcodec_open2( m_input_codec_ctx, input_codec, NULL );
        RETNIF_LOGE( ret < 0, false, "SoundDestination: fail to open codec for input ret=%d", ret );
        LOGD( "m_input_codec_ctx->frame_size=%d", m_input_codec_ctx->frame_size );
        // ************************** setup output ******************************
        AVIOContext *output_io_context = NULL;
        ret = avio_open( &output_io_context, filename, AVIO_FLAG_WRITE );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:failed to avio_open output_context ret=%d", ret );
        AVOutputFormat *output_format = av_guess_format( NULL, filename, NULL );
        RETNIF_LOGE( output_format == NULL, false, "SoundDestination:failed to guess format" );
        m_output_format_ctx = avformat_alloc_context();
        RETNIF_LOGE( m_output_format_ctx == NULL, false, "SoundDestination:failed to alloc m_output_format_ctx" );
        m_output_format_ctx->pb = output_io_context;
        m_output_format_ctx->oformat = output_format;
        av_strlcpy( m_output_format_ctx->filename, filename, sizeof( m_output_format_ctx->filename ) );
        AVCodec *output_codec = avcodec_find_encoder( m_output_format_ctx->oformat->audio_codec );
        RETNIF_LOGE( output_codec == NULL, false, "SoundDestination:find codec fail for output" );
        AVStream *output_stream = avformat_new_stream( m_output_format_ctx, NULL );
        RETNIF_LOGE( output_stream == NULL, false, "SoundDestination:failed to avformat_new_stream" );
        m_output_codec_ctx = avcodec_alloc_context3( output_codec );
        RETNIF_LOGE( m_output_codec_ctx == NULL, false, "SoundDestination:find failed to alloc output_codec_ctx" );
        AVSampleFormat output_sample_fmt = AV_SAMPLE_FMT_NONE;

        for( const AVSampleFormat *p = output_codec->sample_fmts; *p != AV_SAMPLE_FMT_NONE; p++ )
        {
            if( *p == m_input_sample_format )
            {
                output_sample_fmt = *p;
                break;
            }
        }

        if( output_sample_fmt == AV_SAMPLE_FMT_NONE )
        {
            output_sample_fmt = output_codec->sample_fmts[0];
        }

        m_output_codec_ctx->channels          = m_input_channel_num;
        m_output_codec_ctx->channel_layout    = av_get_default_channel_layout( m_input_channel_num );
        m_output_codec_ctx->sample_fmt        = output_sample_fmt;
        m_output_codec_ctx->sample_rate       = m_input_sample_rate;
        //m_output_codec_ctx->bit_rate          = 96000;
        m_output_codec_ctx->time_base.num     = 1;
        m_output_codec_ctx->time_base.den     = m_output_codec_ctx->sample_rate;
        m_output_codec_ctx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;

        if ( m_output_format_ctx->oformat->flags & AVFMT_GLOBALHEADER )
        {
            m_output_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        ret = avcodec_open2( m_output_codec_ctx, output_codec, NULL );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:failed to avcodec_open2 ret=%d", ret );
        ret = avcodec_parameters_from_context( output_stream->codecpar, m_output_codec_ctx );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:failed to avcodec_parameters_from_context ret=%d", ret );
        av_dump_format( m_output_format_ctx, 0, filename, 1 );
        ret = avformat_write_header( m_output_format_ctx, NULL );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:failed to avformat_write_header ret=%d", ret );
        // *************************** init filter  *****************************
        m_filter_graph = avfilter_graph_alloc();
        RETNIF_LOGE( m_filter_graph == NULL, false, "SoundDestination:alloc avfilter_graph_alloc fail" );
        AVFilter *filter  = NULL;
        //------------------------ abuffer ------------------------
        filter = avfilter_get_by_name( "abuffer" );
        RETNIF_LOGE( filter == NULL, false, "SoundDestination:find failter fail, abuffer" );
        char src_args[1024];
        snprintf( src_args, sizeof( src_args ),
                  "sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
                  m_input_codec_ctx->sample_rate,
                  av_get_sample_fmt_name( m_input_codec_ctx->sample_fmt ),
                  m_input_codec_ctx->channel_layout );
        ret = avfilter_graph_create_filter( &m_filter_src_ctx, filter, "src", src_args, NULL, m_filter_graph );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:find create filter failed src ret=%d", ret );
        //------------------------ abuffersink ------------------------
        filter = avfilter_get_by_name( "abuffersink" );
        RETNIF_LOGE( filter == NULL, false, "SoundDestination:find failter fail, abuffersink" );
        ret = avfilter_graph_create_filter( &m_filter_sink_ctx, filter, "sink", NULL, NULL, m_filter_graph );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:find create filter failed sink ret=%d", ret );
        int out_sample_rates[] = { m_output_codec_ctx->sample_rate, -1 };
        ret = av_opt_set_int_list( m_filter_sink_ctx, "sample_rates", out_sample_rates, -1,  AV_OPT_SEARCH_CHILDREN );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:av_opt_set_int_list failed out_sample_rates ret=%d", ret );
        int64_t out_sample_fmts[] = { m_output_codec_ctx->sample_fmt, -1 };
        ret = av_opt_set_int_list( m_filter_sink_ctx, "sample_fmts", out_sample_fmts, -1,  AV_OPT_SEARCH_CHILDREN );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:av_opt_set_int_list failed sample_fmts ret=%d", ret );
        int64_t out_channel_layouts[] = { ( int64_t )m_output_codec_ctx->channel_layout, -1 };
        ret = av_opt_set_int_list( m_filter_sink_ctx, "channel_layouts", out_channel_layouts, -1,  AV_OPT_SEARCH_CHILDREN );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:av_opt_set_int_list failed out_channel_layouts ret=%d", ret );
        //------------------------ aresample ------------------------
        AVFilterContext *filter_resample_ctx = NULL;
        filter = avfilter_get_by_name( "aresample" );
        RETNIF_LOGE( filter == NULL, false, "SoundDestination:find failter fail, aresample" );
        ret = avfilter_graph_create_filter( &filter_resample_ctx, filter, "resample", NULL, NULL, m_filter_graph );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:find create filter failed resample ret=%d", ret );
        // ------------------------ link all ------------------------------
        ret = avfilter_link( m_filter_src_ctx, 0, filter_resample_ctx, 0 );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:faile to link m_filter_src_ctx to filter_resample_ctx ret=%d", ret );
        ret = avfilter_link( filter_resample_ctx, 0, m_filter_sink_ctx, 0 );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:faile to link filter_resample_ctx to m_filter_sink_ctx ret=%d", ret );
        ret = avfilter_graph_config( m_filter_graph, NULL );
        RETNIF_LOGE( ret != 0, false, "SoundDestination:faile to avfilter_graph_config ret=%d", ret );

        if( !( output_codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE ) )
        {
            av_buffersink_set_frame_size( m_filter_sink_ctx, m_output_codec_ctx->frame_size );
        }

        return true;
    };

    void release()
    {
        // ************************ release all ***********************
        LOGD( "SoundDestination:releaseAll()" );

        if( m_output_format_ctx != NULL )
        {
            if( m_output_format_ctx->oformat && m_output_format_ctx->pb )
            {
                avformat_flush( m_output_format_ctx );
                av_write_trailer( m_output_format_ctx );
                avio_closep( &m_output_format_ctx->pb );
            }

            avformat_free_context( m_output_format_ctx );
            m_output_format_ctx = NULL;
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

        if( m_filter_graph != NULL )
        {
            avfilter_graph_free( &m_filter_graph );
        }
    };

    int write_to_decoder( const void *data, int size )
    {
        int ret = 0;
        uint8_t *buf = NULL;
        AVPacket *pkt = NULL;

        do
        {
            buf = ( uint8_t * )av_malloc( size );
            ret = AVERROR( ENOMEM );
            BREAKIF_LOGD( buf == NULL, "write_to_decoder: can not alloc data" );
            AVPacket *pkt = av_packet_alloc();
            ret = AVERROR( ENOMEM );
            BREAKIF_LOGD( pkt == NULL, "write_to_decoder: can not alloc pkt" );
            memcpy( buf, data, size );
            ret = av_packet_from_data( pkt, buf, size );
            BREAKIF_LOGD( ret < 0, "write_to_decoder: failed to av_packet_from_data" );
            buf = NULL; // the buffer will referenced by AVPacket, no need free by us
            pkt->pts = m_pts;
            m_pts += size / m_input_channel_num / CUBIC_SAMPLE_SIZE_IN_BYTE;
            ret = avcodec_send_packet( m_input_codec_ctx, pkt );
            BREAKIF_LOGD( ret < 0, "write_to_decoder: failed to avcodec_send_packet" );
            ret = 0;
        }
        while( 0 );

        if( buf != NULL )
        {
            av_free( buf );
        }

        av_packet_free( &pkt );
        return ret;
    };

    int decode_to_filter()
    {
        int ret = 0;
        AVFrame *frame = av_frame_alloc();
        RETNIF_LOGE( frame == NULL, AVERROR( ENOMEM ), "filter_to_encode: can not alloc frame" );

        do
        {
            ret = avcodec_receive_frame( m_input_codec_ctx, frame );
            BREAKIF( ret < 0 );
            ret = av_buffersrc_add_frame_flags( m_filter_src_ctx, frame, AV_BUFFERSRC_FLAG_PUSH );
            BREAKIF_LOGD( ret < 0, "filter_to_encode: failed to av_buffersrc_add_frame_flags" );
            ret = 0;
        }
        while( 0 );

        av_frame_free( &frame );
        return ret;
    };

    int filter_to_encode()
    {
        int ret = 0;
        AVFrame *frame = av_frame_alloc();
        RETNIF_LOGE( frame == NULL, AVERROR( ENOMEM ), "filter_to_encode: can not alloc frame" );

        do
        {
            ret = av_buffersink_get_frame_flags( m_filter_sink_ctx, frame, 0 );
            BREAKIF( ret < 0 );
            ret = avcodec_send_frame( m_output_codec_ctx, frame );
            BREAKIF_LOGD( ret < 0, "filter_to_encode: failed to avcodec_send_frame" );
            ret = 0;
        }
        while( 0 );

        av_frame_free( &frame );
        return ret;
    };

    int flush_to_file()
    {
        int ret = 0;
        AVPacket *pkt = av_packet_alloc();
        RETNIF_LOGE( pkt == NULL, AVERROR( ENOMEM ), "flush_to_file: can not alloc pkt" );

        do
        {
            ret = avcodec_receive_packet( m_output_codec_ctx, pkt );
            BREAKIF( ret < 0 );
            ret = av_interleaved_write_frame( m_output_format_ctx, pkt );
            BREAKIF_LOGD( ret < 0, "flush_to_file: failed to av_interleaved_write_frame" );
            ret = 0;
        }
        while( 0 );

        av_packet_free( &pkt );
        return ret;
    };


public:
    SoundDestinationEncoder( const string &url )
        : m_dst_url( url )
        , m_input_channel_num( 0 )
        , m_input_sample_rate( 0 )
        , m_input_sample_format( AV_SAMPLE_FMT_S16 )
        , m_input_codec_id( AV_CODEC_ID_PCM_S16LE )
        , m_input_codec_ctx( NULL )
        , m_output_format_ctx( NULL )
        , m_output_codec_ctx( NULL )
        , m_filter_graph( NULL )
        , m_filter_src_ctx( NULL )
        , m_filter_sink_ctx( NULL )
        , m_pts( 0 )
    {};

    virtual ~SoundDestinationEncoder()
    {
        release();
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
        m_input_channel_num = n_channel;
        m_input_sample_rate = sample_rate;
        m_input_sample_format = tab_sam_fmt[sample_fmt];
        m_input_codec_id = tab_codec_id[sample_fmt];
        RETNIF_LOGE(
            m_input_channel_num != 1 &&
            m_input_channel_num != 2,
            -1,
            "bad channel number ! %ld\n",
            m_input_channel_num );
        RETNIF_LOGE(
            m_input_sample_rate != 8000 &&
            m_input_sample_rate != 16000 &&
            m_input_sample_rate != 32000 &&
            m_input_sample_rate != 44100 &&
            m_input_sample_rate != 48000,
            -2,
            "bad sample rate ! %ld\n",
            m_input_sample_rate );
        release();
        RETNIF( !setup(), -3 );
        return 0;
    };

    virtual int x_putFrame( const void *buffer, unsigned int buffer_size )
    {
        int ret = 0;
        AVPacket *pkt;
        uint8_t *data = ( uint8_t * )av_malloc( buffer_size );
        RETNIF_LOGE( ret != 0, -1, "can not alloc data" );
        pkt = av_packet_alloc();

        if( pkt == NULL )
        {
            LOGE( "can not alloc packet" );
            av_free( data );
            return -2;
        }

        memcpy( data, buffer, buffer_size );
        ret = av_packet_from_data( pkt, data, buffer_size );

        if( ret != 0 )
        {
            LOGE( "putFrame create packet failed" );
            av_free( data );
            return -2;
        }

        pkt->pts = m_pts;
        m_pts += buffer_size / m_input_channel_num / 2;
        ret = avcodec_send_packet( m_input_codec_ctx, pkt );
        av_packet_free( &pkt );

        if( ret != 0 )
        {
            LOGE( "putFrame decode failed ret=%d", ret );
            return -3;
        }

        AVFrame *input_frame = av_frame_alloc();
        AVFrame *filt_frame = av_frame_alloc();
        RETNIF_LOGE( input_frame == NULL || filt_frame == NULL, -4, "putFrame can not alloc input_frame" );
        ret = avcodec_receive_frame( m_input_codec_ctx, input_frame );

        if( ret != 0 )
        {
            LOGE( "putFrame can not avcodec_receive_frame ret=%d", ret );
            av_frame_free( &input_frame );
            av_frame_free( &filt_frame );
            return -5;
        }

        ret = av_buffersrc_add_frame_flags( m_filter_src_ctx, input_frame, /*AV_BUFFERSRC_FLAG_KEEP_REF*/AV_BUFFERSRC_FLAG_PUSH );

        if( ret != 0 )
        {
            //LOGE("putFrame can not send frame to filter ret=%d", ret );
            av_frame_free( &input_frame );
            av_frame_free( &filt_frame );
            return -6;
        }

        pkt = av_packet_alloc();

        while( 1 )
        {
            ret = av_buffersink_get_frame_flags( m_filter_sink_ctx, filt_frame, 0 );
            BREAKIF ( ret < 0 );
            ret = avcodec_send_frame( m_output_codec_ctx, filt_frame );
            av_frame_unref( filt_frame );

            while( 0 == avcodec_receive_packet( m_output_codec_ctx, pkt ) )
            {
                av_interleaved_write_frame( m_output_format_ctx, pkt );
                av_packet_unref( pkt );
            }
        }

        av_packet_free( &pkt );
        av_frame_free( &input_frame );
        av_frame_free( &filt_frame );
        return buffer_size;
    };


    virtual int putFrame( const void *buffer, unsigned int buffer_size )
    {
        RETNIF( 0 != write_to_decoder( buffer, ( int )buffer_size ), 0 );

        while( 0 == flush_to_file() ||
                0 == filter_to_encode() ||
                0 == decode_to_filter()
             );

        return buffer_size;
    };
};

#endif //_SOUND_SOURCE_RING_BUFFER_CC_

