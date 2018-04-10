/**
 * @file ISoundDestination.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief sound destination interface
 * @detail sound destination interface
 */
#ifndef _SOUND_DESTINATION_CC_
#define _SOUND_DESTINATION_CC_ 1

#include "SoundDefs.h"

class ISoundDestination
{
public:
    virtual ~ISoundDestination() {};
    virtual int setFormat( int n_channel, int sample_rate, CubicSampleFmt sample_fmt ) = 0;
    virtual int putFrame( const void *buffer, unsigned int buffer_size ) = 0;
};

#endif //_SOUND_DESTINATION_CC_
