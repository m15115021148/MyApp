/**
 * @file SoundDestinationEncOgg.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief ogg encoder sound destination
 * @detail ogg encoder sound destination
 */
#ifndef _SOUND_DESTINATION_ENC_OGG_CC_
#define _SOUND_DESTINATION_ENC_OGG_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundDestination.cc"
#include "CRingBuffer.cc"
#include <iostream>

extern "C" {

#include <vorbis/vorbisenc.h>

}


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundDesticationEncoder"

using namespace std;

class SoundDestinationEncOgg : public ISoundDestination
{
private:
    static const int        MS_PER_STEP = 10;
    string                  m_dst_url;
    int                     m_input_channel_num;
    int                     m_input_sample_rate;
    int                     m_input_frame_size;
    CubicSampleFmt          m_input_sample_fmt;
    FILE                   *m_output_file;


    ogg_stream_state        m_ogg_stream_stat;
    vorbis_info             m_vorbis_info;
    vorbis_comment          m_vorbis_comment;
    vorbis_dsp_state        m_vorbis_dsp_state;
    vorbis_block            m_vorbis_block;

    bool setup()
    {
        int ret = 0;
        LOGD( "setup when url=%s", m_dst_url.c_str() );
        m_output_file = fopen( m_dst_url.c_str(), "wb+" );
        RETNIF_LOGE( !m_output_file, false, "setup fail: can not open file !" );
        /********** Encode setup ************/
        vorbis_info_init( &m_vorbis_info );
        ret = vorbis_encode_init_vbr( &m_vorbis_info, m_input_channel_num, m_input_sample_rate, 0.5f );
        RETNIF_LOGE( ret != 0, false, "setup fail: vorbis_encode_init_vbr error=%d", ret );
        /* add a comment */
        vorbis_comment_init( &m_vorbis_comment );
        vorbis_comment_add_tag( &m_vorbis_comment, "ENCODER", "encoder_example.c" );
        /* set up the analysis state and auxiliary encoding storage */
        ret = vorbis_analysis_init( &m_vorbis_dsp_state, &m_vorbis_info );
        RETNIF_LOGE( ret != 0, false, "setup fail: vorbis_analysis_init error=%d", ret );
        ret = vorbis_block_init( &m_vorbis_dsp_state, &m_vorbis_block );
        RETNIF_LOGE( ret != 0, false, "setup fail: vorbis_block_init error=%d", ret );
        /* set up our packet->stream encoder */
        /* pick a random serial number; that way we can more likely build
        chained streams just by concatenation */
        srand( time( NULL ) );
        ret = ogg_stream_init( &m_ogg_stream_stat, rand() );
        RETNIF_LOGE( ret != 0, false, "setup fail, to init " );
        /* Vorbis streams begin with three headers; the initial header (with
        most of the codec setup parameters) which is mandated by the Ogg
        bitstream spec.  The second header holds any comment fields.  The
        third header holds the bitstream codebook.  We merely need to
        make the headers, then pass them to libvorbis one at a time;
        libvorbis handles the additional Ogg bitstream constraints */
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;
        vorbis_analysis_headerout( &m_vorbis_dsp_state, &m_vorbis_comment, &header, &header_comm, &header_code );
        ogg_stream_packetin( &m_ogg_stream_stat, &header ); // automatically placed in its own page
        ogg_stream_packetin( &m_ogg_stream_stat, &header_comm );
        ogg_stream_packetin( &m_ogg_stream_stat, &header_code );
        /* This ensures the actual
        * audio data will start on a new page, as per spec
        */
        ogg_page oggpage;
        int result = ogg_stream_flush( &m_ogg_stream_stat, &oggpage );
        RETNIF( result == 0, true );
        ret = fwrite( oggpage.header, 1, oggpage.header_len, m_output_file );
        RETNIF_LOGE( !m_output_file, false, "setup fail: write page header !" );
        ret = fwrite( oggpage.body, 1, oggpage.body_len, m_output_file );
        RETNIF_LOGE( !m_output_file, false, "setup fail: write page body !" );
        return true;
    };

    void release()
    {
        // ************************ release all ***********************
        LOGD( "SoundDestination:releaseAll()" );
        ogg_stream_clear( &m_ogg_stream_stat );
        vorbis_block_clear( &m_vorbis_block );
        vorbis_dsp_clear( &m_vorbis_dsp_state );
        vorbis_comment_clear( &m_vorbis_comment );
        vorbis_info_clear( &m_vorbis_info );

        if( m_output_file )
        {
            fclose( m_output_file );
        }
    };



public:
    SoundDestinationEncOgg( const string &url )
        : m_dst_url( url )
        , m_input_channel_num( 0 )
        , m_input_sample_rate( 0 )
        , m_input_frame_size( 0 )
        , m_input_sample_fmt( SAMPLE_16_BIT )
        , m_output_file( NULL )
        , m_ogg_stream_stat()
        , m_vorbis_info()
        , m_vorbis_comment()
        , m_vorbis_dsp_state()
        , m_vorbis_block()
    {};

    virtual ~SoundDestinationEncOgg()
    {
        release();
    }

    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt sample_fmt )
    {
        m_input_channel_num = n_channel;
        m_input_sample_rate = sample_rate;
        m_input_sample_fmt = sample_fmt;
        m_input_frame_size = CubicSampleSizeInByte[sample_fmt] * n_channel;
        RETNIF_LOGE(
            m_input_channel_num != 1 &&
            m_input_channel_num != 2,
            -1,
            "bad channel number ! %d\n",
            m_input_channel_num );
        RETNIF_LOGE(
            m_input_sample_rate != 8000 &&
            m_input_sample_rate != 16000 &&
            m_input_sample_rate != 32000 &&
            m_input_sample_rate != 44100 &&
            m_input_sample_rate != 48000,
            -2,
            "bad sample rate ! %d\n",
            m_input_sample_rate );
        RETNIF_LOGE(
            m_input_sample_fmt != SAMPLE_16_BIT,
            -3,
            "NOT support sample format! %d\n",
            m_input_sample_fmt );
        LOGD( "setFormat m_input_frame_size=%d", m_input_frame_size );
        release();
        return setup() ? 0 : -4;
        //return -4;
    };

    virtual int putFrame( const void *buffer, unsigned int buffer_size )
    {
        RETNIF( buffer == NULL || buffer_size == 0, 0 );
        RETNIF_LOGE(
            m_input_sample_fmt != SAMPLE_16_BIT,
            -2,
            "putFrame failed, NOT support sample format! %d\n",
            m_input_sample_fmt );
        /* data to encode */
        int frame_num = buffer_size / m_input_frame_size;
        char *data = ( char * )buffer;
        /* expose the buffer to submit data */
        float **dsp_buf = vorbis_analysis_buffer( &m_vorbis_dsp_state, frame_num );
        RETNIF_LOGE( dsp_buf == NULL, -1, "putFrame failed: to get dsp buffer" );

        /* uninterleave samples */
        for( int i = 0, j = 0; i < frame_num && j < ( int )buffer_size - 2; i++ )
        {
            for( int ch = 0; ch < m_input_channel_num; ch++, j += 2 )
            {
                dsp_buf[ch][i] = ( ( data[j + 1] << 8 ) | ( 0x00ff & ( int )data[j] ) ) / 32768.f;
            }
        }

        /* tell the library how much we actually submitted */
        vorbis_analysis_wrote( &m_vorbis_dsp_state, frame_num );

        /* vorbis does some data preanalysis, then divvies up blocks for
        more involved (potentially parallel) processing.  Get a single
        block for encoding now */
        while( vorbis_analysis_blockout( &m_vorbis_dsp_state, &m_vorbis_block ) == 1 )
        {
            ogg_packet oggpacket;
            /* analysis, assume we want to use bitrate management */
            vorbis_analysis( &m_vorbis_block, NULL );
            vorbis_bitrate_addblock( &m_vorbis_block );

            while( vorbis_bitrate_flushpacket( &m_vorbis_dsp_state, &oggpacket ) )
            {
                /* weld the packet into the bitstream */
                ogg_stream_packetin( &m_ogg_stream_stat, &oggpacket );

                /* write out pages (if any) */
                while( 1 )
                {
                    ogg_page oggpage;
                    int result = ogg_stream_pageout( &m_ogg_stream_stat, &oggpage );
                    BREAKIF( result == 0 );
                    fwrite( oggpage.header, 1, oggpage.header_len, m_output_file );
                    fwrite( oggpage.body, 1, oggpage.body_len, m_output_file );
                    /* this could be set above, but for illustrative purposes, I do
                     it here (to show that vorbis does know where the stream ends) */
                    BREAKIF( ogg_page_eos( &oggpage ) );
                }
            }
        }

        return buffer_size;
    };
};

#endif //_SOUND_SOURCE_RING_BUFFER_CC_


