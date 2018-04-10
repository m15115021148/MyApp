/**
 * @file cubic_log.h
 * @brief log marcro definition for all cubic
 * @detail log marcro definition for all cubic
 */

#ifndef _CUBIC_LOG_H_
#define _CUBIC_LOG_H_ 1


#ifndef CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "default"
#endif //CUBIC_LOG_TAG

#define CUBIC_LOG_DEF_PATH                  "/tmp/cubic.log.dat"

#define CUBIC_LOG_LEVEL_ERROR  1
#define CUBIC_LOG_LEVEL_DEBUG  2
#define CUBIC_LOG_LEVEL_INFO  3

#define CubicLogE(...) \
    CFramework::GetInstance().getLoger().logout( CUBIC_LOG_LEVEL_ERROR,   CUBIC_LOG_TAG, __VA_ARGS__)

#define CubicLogD(...)  \
    CFramework::GetInstance().getLoger().logout( CUBIC_LOG_LEVEL_DEBUG,   CUBIC_LOG_TAG, __VA_ARGS__)

#define CubicLogI(...)  \
    CFramework::GetInstance().getLoger().logout( CUBIC_LOG_LEVEL_INFO,   CUBIC_LOG_TAG, __VA_ARGS__)

#define LOGE CubicLogE
#define LOGD CubicLogD
#define LOGI CubicLogI

#define RETIF_LOGE( condition, ... ) \
    if( condition ){ \
        LOGE(__VA_ARGS__); \
        return; \
    }

#define RETIF_LOGD( condition, ... ) \
    if( condition ){ \
        LOGD(__VA_ARGS__); \
        return; \
    }

#define RETNIF_LOGE( condition, n, ... ) \
    if( condition ){ \
        LOGE(__VA_ARGS__); \
        return n; \
    }

#define RETNIF_LOGD( condition, n, ... ) \
    if( condition ){ \
        LOGD(__VA_ARGS__); \
        return n; \
    }

#define BREAKIF_LOGE( condition, ... ) \
    if( condition ){ \
        LOGE(__VA_ARGS__); \
        break; \
    }

#define BREAKIF_LOGD( condition, ... ) \
    if( condition ){ \
        LOGD(__VA_ARGS__); \
        break; \
    }


#endif //_CUBIC_LOG_H_
