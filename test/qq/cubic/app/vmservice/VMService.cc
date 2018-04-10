/**
 * @file VMService.cc
 * @author lishujie
 * @version 1.0
 * @brief Light Service
 * @detail Light Service
 */


#include "cubic_inc.h"
#include "CFramework.cc"


#include "CAsyncRun.cc"
#include "CRemoteReport.cc"
#include "DownloadThread.cc"
#include "UploadThread.cc"
#include "CSafeQueue.cc"
#include "CLock.cc"

#include <iostream>
#include <time.h>

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "VMService"


#define CUBIC_VM_PATH_READ_LIST          "/data/vm_read_list"
#define CUBIC_VM_PATH_EXPIRED_LIST       "/data/vm_expired_list"



class VMService : public ICubicApp, public IDownloadThread, public IAbsTimer
{
private:
    static const int CUBIC_VM_READ_LIST_MAX = 10;
    static const int CUBIC_VM_EXPIRED_LIST_MAX = 128;


    CLock                   m_vm_tab_lock;
    list<string>            m_vm_list_fetching;
    list<string>            m_vm_list_unread;
    list<string>            m_vm_list_read;
    list<string>            m_vm_list_expired;

    int                     m_poll_vm_timer;

    inline string getLocalPath( const string &file_name )
    {
        ostringstream oss;
        oss << CUBIC_VOICE_MSG_CACHE << "/" << file_name;
        return oss.str();
    }

    // create new and make to fetching
    void addNewVMtoFetch( const string &remote_url )
    {
        LOGD( "addNewVMtoFetch url=%s", remote_url.c_str() );
        CLock::Auto lock( m_vm_tab_lock );
        string file_name = CUtil::getFileNameOfPath( remote_url );

        for( list<string>::iterator i = m_vm_list_fetching.begin(); i != m_vm_list_fetching.end(); i++ )
        {
            RETIF( file_name == ( *i ) );
        }

        for( list<string>::iterator i = m_vm_list_unread.begin(); i != m_vm_list_unread.end(); i++ )
        {
            RETIF( file_name == ( *i ) );
        }

        for( list<string>::iterator i = m_vm_list_read.begin(); i != m_vm_list_read.end(); i++ )
        {
            if( file_name == ( *i ) )
            {
                string local_path = getLocalPath( file_name );
                RETIF( access( local_path.c_str(), 0 ) == 0 );
            }
        }

        for( list<string>::iterator i = m_vm_list_expired.begin(); i != m_vm_list_expired.end(); i++ )
        {
            RETIF( file_name == ( *i ) );
        }

        DownloadThread::getInstance().addNewDownload( remote_url );
        m_vm_list_fetching.push_back( file_name );
    };

    // move fetching to unread
    void moveVMtoUnread( const string &local_path )
    {
        LOGD( "moveVMtoUnread local_path=%s", local_path.c_str() );
        CLock::Auto lock( m_vm_tab_lock );
        string file_name = CUtil::getFileNameOfPath( local_path );

        if( m_vm_list_fetching.front() != file_name )
        {
            LOGE( "moveVMtoUnread, not in list, remove it" );
            unlink( local_path.c_str() );
            return;
        }

        for( list<string>::iterator i = m_vm_list_read.begin(); i != m_vm_list_read.end(); i++ )
        {
            if( file_name == ( *i ) )
            {
                LOGI( "moveVMtoUnread, it's a read message, no notify" );
                m_vm_list_fetching.pop_front();
                return;
            }
        }

        m_vm_list_fetching.pop_front();
        m_vm_list_unread.push_back( file_name );
        notifyNewMessage( file_name );
        CubicStatSet( CUBIC_STAT_vm_unread, m_vm_list_unread.size() );
        CubicWakeupLockSet(CUBIC_WAKELOCK_ID_VM_UNREAD);
    }

    // remove from unread, so can redownload it when next polling
    void remobeVMfromUnread( const string &local_path )
    {
        LOGD( "remobeVMfromUnread local_path=%s", local_path.c_str() );
        CLock::Auto lock( m_vm_tab_lock );
        string file_name = CUtil::getFileNameOfPath( local_path );

        if( m_vm_list_unread.front() != file_name )
        {
            LOGE( "remobeVMfromUnread, not in list, remove it" );
            return;
        }

        unlink( local_path.c_str() );
        m_vm_list_unread.pop_front();
        if( m_vm_list_unread.size() == 0 )
        {
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_VM_UNREAD);
        }
    }

    // move unread to read, and will make oldest read expired
    void moveVMtoRead( const string &local_path )
    {
        LOGD( "moveVMtoRead local_path=%s", local_path.c_str() );
        CLock::Auto lock( m_vm_tab_lock );
        string file_name = CUtil::getFileNameOfPath( local_path );

        if( m_vm_list_unread.front() != file_name )
        {
            LOGE( "moveVMtoRead, not in list, remove it" );
            return;
        }

        m_vm_list_unread.pop_front();
        m_vm_list_read.push_back( file_name );

        while( m_vm_list_read.size() > CUBIC_VM_READ_LIST_MAX )
        {
            file_name = m_vm_list_read.front();
            unlink( file_name.c_str() );
            m_vm_list_read.pop_front();
            m_vm_list_expired.push_back( file_name );
        }

        CubicStatSet( CUBIC_STAT_vm_read, m_vm_list_read.size() );
        saveHistory();
        if( m_vm_list_unread.size() == 0 )
        {
            CubicWakeupLockClear(CUBIC_WAKELOCK_ID_VM_UNREAD);
        }
    }

    void loadHistory()
    {
        LOGD( "loadHistory entry" );
        CLock::Auto lock( m_vm_tab_lock );
        static const int BUF_SZ = 1024;
        char buf[BUF_SZ + 4];
        int fd = -1;
        // load expired list
        fd = open( CUBIC_VM_PATH_EXPIRED_LIST, O_RDONLY );

        if( fd > 0 )
        {
            m_vm_list_expired.clear();
            memset( buf, 0, BUF_SZ );

            while( CUtil::readLine( fd, buf, BUF_SZ ) > 0 )
            {
                m_vm_list_expired.push_back( string( buf ) );
                LOGD( "loadHistory, load expired list: %s", buf );
                memset( buf, 0, BUF_SZ );
            }

            close( fd );
        }

        // load read list
        fd = open( CUBIC_VM_PATH_READ_LIST, O_RDONLY );

        if( fd > 0 )
        {
            m_vm_list_read.clear();
            memset( buf, 0, BUF_SZ );

            while( CUtil::readLine( fd, buf, BUF_SZ ) > 0 )
            {
                m_vm_list_read.push_back( buf );
                LOGD( "loadHistory, load read list: %s", buf );
                memset( buf, 0, BUF_SZ );
            }

            close( fd );
        }
    };

    void saveHistory()
    {
        LOGD( "saveHistory entry" );
        CLock::Auto lock( m_vm_tab_lock );
        static const char NEW_LINE = '\n';
        int fd = -1;
        // save expired list
        fd = open( CUBIC_VM_PATH_EXPIRED_LIST, O_CREAT | O_WRONLY | O_TRUNC, 0666 );

        if( fd > 0 )
        {
            for( list<string>::iterator i = m_vm_list_expired.begin(); i != m_vm_list_expired.end(); i++ )
            {
                write( fd, ( *i ).c_str(), ( *i ).length() );
                write( fd, &NEW_LINE, 1 );
            }

            close( fd );
        }

        // save read list
        fd = open( CUBIC_VM_PATH_READ_LIST, O_CREAT | O_WRONLY | O_TRUNC, 0666 );

        if( fd > 0 )
        {
            for( list<string>::iterator i = m_vm_list_read.begin(); i != m_vm_list_read.end(); i++ )
            {
                write( fd, ( *i ).c_str(), ( *i ).length() );
                write( fd, &NEW_LINE, 1 );
            }

            close( fd );
        }
    };

    void notifyNewMessage( const string &file_name )
    {
        string local_path = getLocalPath( file_name );
        cubic_msg_last_unread_vm arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.path, local_path.c_str(), CUBIC_PATH_MAX );
        CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_LAST_UNREAD_VM, arg );
    };

    void notifyLeaveMessage( const string &file_name )
    {
        string local_path = getLocalPath( file_name );
        cubic_msg_lm arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.path, local_path.c_str(), CUBIC_PATH_MAX );
        CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_LM, arg );
    };

    void pollFromServer()
    {
        LOGD( "pollFromServer" );
        RETIF_LOGE( CubicCfgGetStr( CUBIC_CFG_push_group ).length() <= 0, "pollFromServer, no group yet" );
        // timer callback is running in standalone thread, just do report here
        vector<string> vm_list = CRemoteReport::getVMList( CUBIC_VM_READ_LIST_MAX );
        LOGD( "pollFromServer, got voice message: %d", vm_list.size() );

        for ( size_t i = 0; i < vm_list.size(); i ++ )
        {
            addNewVMtoFetch( vm_list[i] );
        }
    }

    void cancelPollVMTimer()
    {
        RETIF( m_poll_vm_timer < 0 );
        LOGD( "cancelPollVMTimer" );
        CubicKillAbsTimer( m_poll_vm_timer );
        m_poll_vm_timer = -1;
    };

    void setPollVMTimer()
    {
        cancelPollVMTimer();
        int period = CubicCfgGetI( CUBIC_CFG_light_polling_period );
        RETIF_LOGD( period <= 0, "setPollVMTimer polling is disabled: %d", period );
        m_poll_vm_timer = CubicSetAbsTimerInterval( period, this );
        LOGD( "setPollVMTimer id=%d", m_poll_vm_timer );
    };

public:
    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        curl_global_init( CURL_GLOBAL_ALL );
        DownloadThread::getInstance().registerUser( this );
        DownloadThread::getInstance().start();
        UploadThread::getInstance().start();
        loadHistory();
        setPollVMTimer();
        return true;
    };

    void onDeInit()
    {
        LOGD( "%s onDeInit", CUBIC_THIS_APP );
        DownloadThread::getInstance().stop();
        UploadThread::getInstance().stop();
        cancelPollVMTimer();
        saveHistory();
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMessage: %s, %d", str_src_app_name.c_str(), n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_VM_RECEIVED:
            LOGD( "onMessage: CUBIC_MSG_VM_RECEIVED" );
            {
                cubic_msg_vm_received *data = ( cubic_msg_vm_received * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_VM_RECEIVED argument missing !" );
                BREAKIF_LOGE( data->voice_message_url[0] == 0, "CUBIC_MSG_VM_RECEIVED url is empty !" );
                addNewVMtoFetch( data->voice_message_url );
            }
            break;

        case CUBIC_MSG_VM_POLL:
            LOGD( "onMessage: CUBIC_MSG_VM_POLL" );
            {
                CLock::Auto lock( m_vm_tab_lock );
                BREAKIF_LOGD( m_vm_list_unread.size() <= 0, "onMessage: no more unread" );
                notifyNewMessage( m_vm_list_unread.front() );
            }
            break;

        case CUBIC_MSG_VM_READ:
            LOGD( "onMessage: CUBIC_MSG_VM_READ" );
            {
                cubic_msg_vm_read *data = ( cubic_msg_vm_read * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_VM_RECEIVED argument missing !" );
                BREAKIF_LOGE( data->path[0] == 0, "CUBIC_MSG_VM_RECEIVED url is empty !" );

                if( data->error == 0 )
                {
                    moveVMtoRead( data->path );
                }
                else
                {
                    remobeVMfromUnread( data->path );
                }
            }
            break;

        case CUBIC_MSG_VM_GET_LM:
            LOGD( "onMessage: CUBIC_MSG_VM_GET_LM" );
            {
                cubic_msg_vm_get_lm *data = ( cubic_msg_vm_get_lm * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_VM_RECEIVED argument missing !" );
                CLock::Auto lock( m_vm_tab_lock );
                BREAKIF_LOGE( data->index < 0 || data->index >= ( int )m_vm_list_read.size(),
                              "onMessage, no valid leave message" );
                int idx = 0;

                for( list<string>::iterator i = m_vm_list_read.begin(); i != m_vm_list_read.end(); i++ )
                {
                    if( idx == data->index )
                    {
                        notifyLeaveMessage( *i );
                        break;
                    }

                    idx ++;
                };
            }
            break;

        case CUBIC_MSG_VM_SEND:
            LOGD( "onMessage: CUBIC_MSG_VM_SEND" );
            {
                cubic_msg_vm_send *data = ( cubic_msg_vm_send * )p_data;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_VM_RECEIVED argument missing !" );
                BREAKIF_LOGE( data->path[0] == 0, "CUBIC_MSG_VM_RECEIVED url is empty !" );
                UploadThread::getInstance().addNewUpload( data->path );
            }
            break;

        default:
            break;
        }

        return 0;
    };

    // interface for IDownloadThread
    virtual void downloadComplete( const string &local_path, int error )
    {
        if( error == 0 )
        {
            moveVMtoUnread( local_path );
        }
        else
        {
            unlink( local_path.c_str() );
        }
    };

    // interface for IAbsTimer
    virtual void onAbsTimer( int n_timer_id )
    {
        LOGD( "onAbsTimer: %d", n_timer_id );
        if( n_timer_id == m_poll_vm_timer )
        {
            pollFromServer();
        }
    };
};

IMPLEMENT_CUBIC_APP( VMService )

