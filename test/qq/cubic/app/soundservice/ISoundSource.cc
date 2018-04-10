/**
 * @file ISoundSource.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief sound source interface
 * @detail sound source interface
 */
#ifndef _SOUND_SOURCE_CC_
#define _SOUND_SOURCE_CC_ 1


class ISoundSource
{
public:
    virtual ~ISoundSource() {};
    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt CubicSampleFmt ) = 0;
    virtual int getFrame( void *buffer, unsigned int buffer_size ) = 0;
};

#endif //_SOUND_SOURCE_CC_
