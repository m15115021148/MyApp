/**
 * @file DebugHelper.cc
 * @author shujie.li
 * @version 1.0
 * @brief Cubic DebugHelper, ultility debug methods
 * @detail Cubic DebugHelper, ultility debug methods
 */
#ifndef _DEBUG_HELPER_CC_
#define _DEBUG_HELPER_CC_ 1

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>

// print log info to std error, use it just like printf
// it will cantain file name, line number, function name, automatically
#define QCT_PRINT_LOG(...)  \
    fprintf( stderr, "-=QCT=- %s: %d (%s):", __FILE__, __LINE__, __FUNCTION__); \
    fprintf( stderr, __VA_ARGS__ ); \
    fprintf( stderr, "\n" )

// print back trace to stderr, 'num' to set max depth
// notice: if want to see the function name in backtrace output,
//         please add '-rdynamic' switch to ld flag for gnu compiler
#define QCT_TRACE_BACK( num ) \
    { \
        void** buf = 0; \
        char** msg = 0; \
        int n = 0; \
        int i = 0; \
        buf = (void**)malloc( sizeof(void*) * num ); \
        if( buf != 0 ){ \
            n = backtrace( buf, num ); \
            msg = backtrace_symbols( buf, n ); \
            if (msg != NULL) {\
                fprintf( stderr, "-=QCT=- %s: %d (%s)==== BACKTRACE =====\n", __FILE__, __LINE__, __FUNCTION__); \
                for( i = 0; i < n; i ++ ) fprintf( stderr, "==> %s\n", msg[i] ); \
                fprintf( stderr, "\n" ); \
                free(msg); \
            }\
            free(buf);\
        } \
    }

#endif //_DEBUG_HELPER_CC_
