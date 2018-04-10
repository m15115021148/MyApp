/**
 * @file SoundDestinationSip.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief ring buffer sound source
 * @detail ring buffer sound source
 */
#ifndef _SOUND_DESTINATION_SIP_CC_
#define _SOUND_DESTINATION_SIP_CC_ 1

#include "cubic_inc.h"
#include "SoundDefs.h"
#include "ISoundDestination.cc"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SoundDestinationSip"

class SoundDestinationSip : public ISoundDestination
{
private:
    int             m_fd_vioce_pipe;

public:
    SoundDestinationSip()
        : m_fd_vioce_pipe( -1 )
    {
        m_fd_vioce_pipe = open( CUBIC_SIP_VOICE_PIPE_SINK, O_RDWR | O_NONBLOCK );
        RETIF_LOGE( m_fd_vioce_pipe <= 0, "can not open voice pipe" );
    };

    virtual ~SoundDestinationSip()
    {
        if( m_fd_vioce_pipe > 0 )
        {
            close( m_fd_vioce_pipe );
            m_fd_vioce_pipe = -1;
        }
    }

    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt sample_fmt )
    {
        return 0;
    };

    virtual int putFrame( const void *buffer, unsigned int buffer_size )
    {
        RETNIF( m_fd_vioce_pipe < 0, -1 );
        return write( m_fd_vioce_pipe, buffer, buffer_size );
    };
};

#endif //_SOUND_DESTINATION_SIP_CC_

