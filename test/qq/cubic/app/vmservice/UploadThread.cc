/**
 * @file UploadThread.cc
 * @author lishujie
 * @version 1.0
 * @brief VM Service, upload thread
 * @detail VM Service, upload thread
 */
#ifndef _UOLOAD_THREAD_CC_
#define _UOLOAD_THREAD_CC_ 1

#include "cubic_inc.h"
#include "CThread.cc"
#include "CSafeQueue.cc"
#include "CRemoteReport.cc"

#include "../soundservice/SoundSourceDecoder.cc"

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "UploadThread"

#define CUBIC_VM_MAX_UPLOAD_RETRY    30

class UploadThread : public CThread
{
private:
    CSafeQueue<string>  m_upload_list;
    int                 m_uploading;

    UploadThread() : m_upload_list(), m_uploading( 0 ) {};
public:
    virtual ~UploadThread()
    {};

    static UploadThread &getInstance()
    {
        static UploadThread instance;
        return instance;
    };

    inline int waitSize()
    {
        return m_upload_list.size();
    }

    void addNewUpload(   const string &local_path )
    {
        LOGD( "addNewUpload: %s", local_path.c_str() );
        RETIF_LOGD( m_upload_list.exist( local_path ), "addNewUpload: already exist!" );
        m_upload_list.push( local_path );
        m_uploading++;
        CubicStatSet( CUBIC_STAT_vm_uploading, m_uploading );
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_VM_UPLOAD);
    };

    virtual RunRet run( void *user )
    {
        UNUSED_ARG( user );

        if( CubicStatGetI( CUBIC_STAT_net_connected ) == 0 )
        {
            sleep( 1 );
            return RUN_CONTINUE;
        };

        int success = 0;

        string local_path;

        RETNIF( CSafeQueue<string>::ERR_NO_ERROR != m_upload_list.pop( local_path, 100 ), RUN_CONTINUE );

        for( int i = 0; i < CUBIC_VM_MAX_UPLOAD_RETRY; i++ )
        {
            if( 0 == CRemoteReport::uploadVoiceMessage( local_path,
                    SoundSourceDecoder::getMediaFileDuration( local_path ) ) )
            {
                success = 1;
                break;
            }

            sleep( 1 );
        }

        if( success != 1 )
        {
            LOGE( "failed to upload message, max retry reached: %s", local_path.c_str() );
        }

        LOGD( "do_upload done" );
        unlink( local_path.c_str() );
        m_uploading--;
        CubicStatSet( CUBIC_STAT_vm_uploading, m_uploading );
        if( m_uploading == 0 )
        {
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_VM_UPLOAD);
        }
        return RUN_CONTINUE;
    };
};


#endif //_UOLOAD_THREAD_CC_
