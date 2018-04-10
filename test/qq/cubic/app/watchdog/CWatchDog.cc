/**
 * @file CWatchDog.cc
 * @author Shujie.Li
 * @version 1.0
 * @brief watch dog service to setup and watch every one
 * @detail watch dog service to setup and watch every one
 */

#include "cubic_inc.h"
#include "CCoreWatch.h"
#include "CThread.cc"
#include "CConfig.cc"
#include "CLogger.cc"
#include "cubic_inc.h"
#include <iostream>
#include <map>
#include <vector>
#include <sys/types.h>
#include <sys/wait.h>


using namespace std;

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "watchdog"


#ifdef LOGD
#undef LOGD
#endif //LOGD

#ifdef LOGE
#undef LOGE
#endif //LOGE

#ifdef LOGI
#undef LOGI
#endif //LOGI


#define LOGE(...) \
    m_logger.logout( CUBIC_LOG_LEVEL_ERROR,   CUBIC_LOG_TAG, __VA_ARGS__)

#define LOGD(...) \
    m_logger.logout( CUBIC_LOG_LEVEL_DEBUG,   CUBIC_LOG_TAG, __VA_ARGS__)

#define LOGI(...) \
    m_logger.logout( CUBIC_LOG_LEVEL_INFO,   CUBIC_LOG_TAG, __VA_ARGS__)


class WatchDog : public CThread
{
private:
    typedef enum ErrCode
    {
        ERR_NO_ERROR = 0,
        ERR_CREATE_FAIL = -1,
        ERR_SET_OPTION_FAIL = -2,
        ERR_BIND_ADDR_FAIL = -3,
        ERR_LISTENER_ALREADY_START = -4,
        ERR_CREATE_LISTEN_THREAD_FAIL = -5,
        ERR_SOCKET_NOT_READY = -6,
        ERR_WAIT_SOCKET_FAIL = -7,
        ERR_TIMEOUT = -8,
        ERR_DATA_RECV_FAIL = -9,
    } EErrCode;


    static const int INVALID_SOCKET = -1;
    static const int RECV_TIMEOUT = 1000; // ms
    static const int WATCH_KILL_TIMEOUT = 1000;


    int m_socket;
    string m_sock_addr;
    CLogger m_logger;


    //****************** watch table *******************
    typedef struct WatchItem
    {
        string name;
        string command;
        enum
        {
            ONECE = 0,
            INIT,
            ETERNAL,
        } type;
        pid_t pid;
        int left_time;
    } WatchItem;
    map<string, WatchItem> m_watch_tab;
    map<pid_t, string> m_watch_pid_tab;
    int m_watch_timeout;


    EErrCode initSocket()
    {
        const unsigned int ASYN_TRUE = 1;
        int ret = 0;

        // ------------ create socket --------------
        if( m_socket == INVALID_SOCKET )
        {
            m_socket = socket( AF_UNIX, SOCK_DGRAM, 0 );
        }

        if( m_socket == INVALID_SOCKET )
        {
            perror( "Create Socket" );
            return ERR_CREATE_FAIL;
        }

        // ------------ set to async ----------------
        ret = ioctl( m_socket, FIONBIO, &ASYN_TRUE );

        if( ret < 0 )
        {
            perror( "Set ASYNC Socket" );
            return ERR_SET_OPTION_FAIL;
        }

        // ------------ bind address ----------------
        struct sockaddr_un t_addr;
        t_addr.sun_family = AF_UNIX;
        strncpy( t_addr.sun_path, m_sock_addr.c_str(), sizeof( t_addr.sun_path ) );
        unlink( m_sock_addr.c_str() );
        ret = bind( m_socket, ( struct sockaddr * )( &t_addr ), ( socklen_t )sizeof( struct sockaddr_un ) );

        if( ret < 0 )
        {
            perror( "Bind Address" );
            return ERR_BIND_ADDR_FAIL;
        }

        return ERR_NO_ERROR;
    };

    void deinitSocket()
    {
        if( m_socket == INVALID_SOCKET )
        {
            return;
        }

        close( m_socket );
        m_socket = INVALID_SOCKET;
        unlink( m_sock_addr.c_str() );
    };


    WatchDog()
        : m_socket( INVALID_SOCKET )
        , m_sock_addr( CORE_WATCH_SERV_PATH )
        , m_logger( "WatchDog", 4 )
        , m_watch_timeout( 10 )
    {};


public:
    static WatchDog &GetInstance()
    {
        static WatchDog instance;
        return instance;
    };

    ~WatchDog()
    {};

    bool init()
    {
        LOGE( "%s onInit", CUBIC_LOG_TAG );
        CConfig cfg( CUBIC_CONFIG_PATH );
        // setup watch socket
        RETNIF_LOGE( ERR_NO_ERROR != initSocket(), false, "onInit failed to setup socket !" );
        m_watch_timeout = cfg.get( CUBIC_CFG_watch_timeout, ( int )10 );
        // load config table
        vector<string> init_table = cfg.enumSub( CUBIC_CFG_watch_init );
        vector<string> name_table;
        LOGD( "init_table size:%d, timeout:%d", init_table.size(), m_watch_timeout );

        // init watch table
        for( size_t i = 0; i < init_table.size(); i++ )
        {
            WatchItem item;
            string type = cfg.getv( CUBIC_CFG_watch_init_type, ( string )"null", init_table[i].c_str() );

            if( type == "eternal" )
            {
                item.type = WatchItem::ETERNAL;
            }
            else if( type == "init" )
            {
                item.type = WatchItem::INIT;
            }
            else if( type == "onece" )
            {
                item.type = WatchItem::ONECE;
            }
            else
            {
                LOGE( "error: unknown type %s for %s", type.c_str(), item.name.c_str() );
                continue;
            }

            item.name = cfg.getv( CUBIC_CFG_watch_init_name, ( string )"null", init_table[i].c_str() );
            item.command = cfg.getv( CUBIC_CFG_watch_init_command, ( string )"null", init_table[i].c_str() );
            item.pid = -1;
            item.left_time = m_watch_timeout;
            m_watch_tab[item.name] = item;
            name_table.push_back( item.name ); // to keep the sort
            LOGI( "init_table %d, name[%s], type[%s], command:%s", i, item.name.c_str(), type.c_str(), item.command.c_str() );
        }

        // begin all apps
        for( size_t i = 0; i < name_table.size(); i++ )
        {
            string name = name_table[i];
            pid_t pid = CUtil::execel_as_child( m_watch_tab[name].command );
            LOGE( "progress:%s start, pid:%d", name.c_str(), pid );
            CONTINUEIF( pid <= 0 );

            // wait for init job done
            if( m_watch_tab[ name ].type == WatchItem::INIT )
            {
                waitpid( pid, 0, 0 );
            }

            m_watch_tab[ name ].pid = pid;
            m_watch_pid_tab[pid] = name;
        }

        return this->start();
    };

    void deinit()
    {
        LOGE( "%s onDeInit", CUBIC_LOG_TAG );
        deinitSocket();
        return;
    };

    virtual void onChildDead( pid_t pid )
    {
        RETIF( m_watch_pid_tab.find( pid ) == m_watch_pid_tab.end() );
        string name = m_watch_pid_tab[pid];
        m_watch_pid_tab.erase( pid );
        WatchItem i = m_watch_tab[name];
        LOGE( "PID[%d] is ended, name is:%s command:%s", i.pid, i.name.c_str(), i.command.c_str() );

        if( m_watch_tab[name].type == WatchItem::ETERNAL )
        {
            pid = CUtil::execel_as_child( m_watch_tab[name].command );

            if( pid <= 0 )
            {
                perror( "start child faild" );
                LOGE( "start service fail, name:%s", name.c_str() );
                m_watch_tab[name].pid = -1;
                return;
            }

            m_watch_tab[name].pid = pid;
            m_watch_tab[name].left_time = m_watch_timeout;
            m_watch_pid_tab[ pid ] = name;
            LOGE( "progress:%s restarted, new pid:%d", name.c_str(), pid );
        }
    };

    virtual int tick()
    {
        pid_t pid = -1;

        // if any child die, finish it, and restart if necessary
        while( ( pid = waitpid( 0, NULL, WNOHANG ) ) > 0 )
        {
            onChildDead( pid );
        };

        // restart no response service
        map<string, WatchItem>::iterator iter;

        for( iter = m_watch_tab.begin(); iter != m_watch_tab.end(); iter++ )
        {
            CONTINUEIF( iter->second.type != WatchItem::ETERNAL );
            iter->second.left_time --;
            CONTINUEIF( iter->second.left_time > 0 );
            LOGE( "progress:%s is timeout, known as pid:%d", iter->second.name.c_str(), iter->second.pid );

            if( iter->second.pid > 0  )
            {
                pid_t pid = iter->second.pid;
                iter->second.pid = -1;
                m_watch_pid_tab.erase( pid );
                CUtil::kill_with_pid( pid, WATCH_KILL_TIMEOUT );
            }

            pid = CUtil::execel_as_child( iter->second.command );

            if( pid < 0 )
            {
                perror( "start child faild" );
                LOGE( "start service fail, name:%s", iter->second.name.c_str() );
                continue;
            }

            iter->second.pid = pid;
            iter->second.left_time = m_watch_timeout;
            m_watch_pid_tab[ pid ] = iter->second.name;
            LOGE( "progress:%s restarted, new pid:%d", iter->second.name.c_str(), pid );
        }

        return 0;
    };

    virtual RunRet run( void *user )
    {
        char buf[CUBIC_APP_NAME_MAX + 4];
        memset( buf, 0, sizeof( buf ) );
        RETNIF( CUtil::waitAndRecv( m_socket, RECV_TIMEOUT, NULL, buf, CUBIC_APP_NAME_MAX ) <= 0, RUN_CONTINUE );
        string name = buf;
        RETNIF( m_watch_tab.find( name ) == m_watch_tab.end(), RUN_CONTINUE );
        m_watch_tab[name].left_time = m_watch_timeout;
        return CThread::RUN_CONTINUE;
    };
};


static int sn_stop_flag = 0;

static void main_quit( int sig )
{
    switch( sig )
    {
    case SIGINT:
    case SIGHUP:
    case SIGABRT:
    case SIGTERM:
    case SIGSTOP:
        sn_stop_flag = 1;
        break;

    case SIGCHLD:
    {
        pid_t pid;
        pid = waitpid( -1, NULL, WNOHANG );

        if( pid != -1 && errno != 10 )
        {
            WatchDog::GetInstance().onChildDead( pid );
        }
    }
    break;
    }
};

int main( int argc, const char *argv[] )
{
    // setup signal handle
    signal( SIGINT,  main_quit );
    signal( SIGHUP,  main_quit );
    signal( SIGABRT, main_quit );
    signal( SIGTERM, main_quit );
    signal( SIGSTOP, main_quit );
    signal( SIGCHLD, main_quit );

    // setup frameork instance
    if( !WatchDog::GetInstance().init() )
    {
        printf( "init failed !\n" );
        //LOGE("Fail to init app: %s", sstr_cubic_appName );
        return -1;
    }

    // start message pump
    while( sn_stop_flag == 0 )
    {
        sleep( 1 );
        //usleep(1000000);
        WatchDog::GetInstance().tick();
    };

    // deinit framwork
    WatchDog::GetInstance().deinit();

    return 0;
};


