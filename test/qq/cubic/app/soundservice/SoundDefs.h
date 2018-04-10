/**
 * @file SoundDefs.h
 * @author Shujie.Li
 * @version 1.0
 * @brief sound definition
 * @detail sound definition
 */
#ifndef _SOUND_DEFS_H_
#define _SOUND_DEFS_H_ 1


#include "cubic_const.h"

typedef enum CubicSampleFmt
{
    SAMPLE_8_BIT = 0,
    SAMPLE_16_BIT,
    SAMPLE_32_BIT,
    SAMPLE_32_BIT_FLOAT,
    SAMPLE_64_BIT_FLOAT,
    SAMPLE_UNKNOWN,
} CubicSampleFmt;

static const int CubicSampleSizeInByte[SAMPLE_UNKNOWN] = {1, 2, 4, 4, 8};
const static int CUBIC_DEF_SAMPLE_RATE = 16000;
const static int CUBIC_DEF_CHANNEL_NUM = 1;
const static int CUBIC_SAMPLE_SIZE_IN_BYTE = 2;
const static CubicSampleFmt CUBIC_DEF_SAMPLE_FMT = SAMPLE_16_BIT;

#define CUBIC_UCMGR_NAME "snd_soc_msm_Tomtom_I2S"



#endif //_SOUND_DEFS_H_
