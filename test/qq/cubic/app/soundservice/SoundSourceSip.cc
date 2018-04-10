/**
 * @file SoundSourceSip.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief ring buffer sound source
 * @detail ring buffer sound source
 */
#ifndef _SOUND_SOURCE_SIP_CC_
#define _SOUND_SOURCE_SIP_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundSource.cc"
#include "CRingBuffer.cc"
#include <fcntl.h>


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundSourceSip"

class SoundSourceSip : public ISoundSource
{
private:
    int             m_fd_vioce_pipe;
    int             m_frame_size;
    CRingBuffer    *mp_ring_buffer;

public:
    SoundSourceSip()
        : m_fd_vioce_pipe( -1 )
        , m_frame_size( 0 )
        , mp_ring_buffer( NULL )
    {
        m_fd_vioce_pipe = open( CUBIC_SIP_VOICE_PIPE_SRC, O_RDWR | O_NONBLOCK );
        RETIF_LOGE( m_fd_vioce_pipe <= 0, "can not open voice pipe" );
    };

    virtual ~SoundSourceSip()
    {
        if( m_fd_vioce_pipe > 0 )
        {
            close( m_fd_vioce_pipe );
            m_fd_vioce_pipe = -1;
        }

        DELETE( mp_ring_buffer );
    }

    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt sample_fmt )
    {
        int frame_size = n_channel * sample_rate * CubicSampleSizeInByte[sample_fmt] * 20 / 1000;
        LOGD( "setFormat" );
        RETNIF( frame_size == m_frame_size, 0 );
        DELETE( mp_ring_buffer );
        m_frame_size = frame_size;
        mp_ring_buffer = new CRingBuffer( m_frame_size * 10 );
        RETNIF_LOGE( mp_ring_buffer == NULL, -1, "setFormat: can not alloc ring buffer" );
        LOGD( "setFormat done" );
        return 0;
    };

    virtual int getFrame( void *buffer, unsigned int buffer_size )
    {
        RETNIF( m_fd_vioce_pipe < 0 || mp_ring_buffer == NULL, -1 );
        static const int BUF_SZ = 1024;
        char buf[BUF_SZ + 4];
        int n = 0;

        while( ( n = read( m_fd_vioce_pipe, buf, BUF_SZ ) ) > 0 )
        {
            mp_ring_buffer->put( buf, n );
        }

        RETNIF( mp_ring_buffer->remain() < ( int )buffer_size, 0 );
        mp_ring_buffer->get( buffer, buffer_size );
        return buffer_size;
    };
};

#endif //_SOUND_SOURCE_SIP_CC_
