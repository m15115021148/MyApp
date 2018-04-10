/**
 * @file SoundSourceRingBuffer.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief ring buffer sound source
 * @detail ring buffer sound source
 */
#ifndef _SOUND_SOURCE_RING_BUFFER_CC_
#define _SOUND_SOURCE_RING_BUFFER_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundSource.cc"
#include "CRingBuffer.cc"


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundSourceRingBuffer"

class SoundSourceRingBuffer : public ISoundSource
{
private:
    CRingBuffer *m_ring_buffer;

public:
    SoundSourceRingBuffer()
        : m_ring_buffer( NULL )
    {};

    virtual ~SoundSourceRingBuffer()
    {
        DELETE( m_ring_buffer );
    }

    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt sample_fmt )
    {
        int buffer_size = n_channel * sample_rate * CubicSampleSizeInByte[sample_fmt]; // 1 secound
        DELETE( m_ring_buffer );
        m_ring_buffer = new CRingBuffer( buffer_size, true );
        RETNIF_LOGE( m_ring_buffer == NULL, -1, "setFormat failed, can not alloc ringbuffer" );
        return 0;
    };

    virtual int getFrame( void *buffer, unsigned int buffer_size )
    {
        RETNIF( m_ring_buffer == NULL, -1 );
        RETNIF( CRingBuffer::ERR_NO_ERROR != m_ring_buffer->get( buffer, buffer_size ), 0 );
        return buffer_size;
    };

    int putFrame( void *buffer, unsigned int buffer_size )
    {
        RETNIF( m_ring_buffer == NULL, -1 );
        RETNIF( CRingBuffer::ERR_NO_ERROR != m_ring_buffer->put( buffer, buffer_size ), 0 );
        return buffer_size;
    };
};

#endif //_SOUND_SOURCE_RING_BUFFER_CC_
