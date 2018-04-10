#ifndef __OTA_SERVICE_CC__
#define __OTA_SERVICE_CC__ 1

/**
 * @file OTAService.cc
 * @author lele.zhou
 * @version 1.0
 * @brief Update Firmware App, impletement OTAService function.
 * @detail Update Firmware App, impletement OTAService function.
 */

#include "CFramework.cc"
#include "cubic_inc.h"
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include "document.h"
#include "allocators.h"
#include "stringbuffer.h"
#include "JsonInterface.cc"
#include "CRemoteReport.cc"
#include "CStringTool.cc"



using namespace std;
using namespace rapidjson;

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG               "OTAService"
#include "CAsyncRun.cc"

//#define CUBIC_JSON_URL                "http://116.247.69.94:7000/mamot.json"
#define CUBIC_JSON_FILE             "/cache/mamot.json"
#define CUBIC_DOWNLOAD_FILE_PATH    "/cache/"
#define CUBIC_SYSTEM_INFO_DIR_PATH  "/etc/cfg/ro"
#define CUBIC_START_TIMER_INTERVAL 5000
#define CUBIC_LOOP_TIMER_INTERVAL  1000*3600*12
#define CUBIC_RETRY_MAX                 3
#define CUBIC_CMD_BUFFER_LEN_MAX        1024

class OTAService : public ICubicApp, public ITimer
{
private:
    int         m_start_update_timer;
    int         m_loop_update_timer;
    string      m_new_version_num;
    string      m_new_version_date;
    string      m_new_version_info;

    void cancelStartUpdateTimer()
    {
        RETIF( m_start_update_timer < 0 );
        LOGI( "cancelStartUpdateTimer" );
        CubicKillTimer( m_start_update_timer );
        m_start_update_timer = -1;
    };

    void setStartUpdateTimer()
    {
        cancelStartUpdateTimer();
        m_start_update_timer = CubicSetTimerInterval( CUBIC_START_TIMER_INTERVAL, this );
        LOGI( "setStartUpdateTimer id=%d", m_start_update_timer );
    };

    void cancelLoopUpdateTimer()
    {
        RETIF( m_loop_update_timer < 0 );
        LOGI( "cancelLoopUpdateTimer" );
        CubicKillTimer( m_loop_update_timer );
        m_loop_update_timer = -1;
    };

    void setLoopUpdateTimer()
    {
        cancelLoopUpdateTimer();
        m_loop_update_timer = CubicSetTimerInterval( CUBIC_LOOP_TIMER_INTERVAL, this );
        LOGI( "setLoopUpdateTimer id=%d", m_loop_update_timer );
    };

    virtual void onTimer( int n_timer_id )
    {
        LOGI( "onTimer: %d\tm_start_update_timer: %d", n_timer_id, m_start_update_timer );

        if( n_timer_id == m_start_update_timer )
        {
            int netStat4 = CubicStatGetI( CUBIC_STAT_net_wanstat_v4 );
            int netStat6 = CubicStatGetI( CUBIC_STAT_net_wanstat_v6 );
            LOGI( "netStat4: %d\tnetStat6: %d\n", netStat4, netStat6 );

            if( ( netStat4 == 0x03 ) || ( netStat6 == 0x09 ) )
            {
                cancelStartUpdateTimer();
                cancelLoopUpdateTimer();
                setLoopUpdateTimer();
                updateVersion();
            }
        }

        if( n_timer_id == m_loop_update_timer )
        {
            updateVersion();
        }
    };

    static int getDocument( const string &file, Document &doc )
    {
        string src;
        JsonInterface::getJsonDataFromFile( file.c_str(), src );
        doc.Parse<0>( src.c_str() );
        RETNIF_LOGE( doc.HasParseError(), -1, "\nError(offset %u): %u\n", ( unsigned )doc.GetErrorOffset(), doc.GetParseError() );
        return 0;
    };

    static int parseJson( const string &file, const string &localVer, Value &obj )
    {
        unsigned int i = 0;
        int fullVerNum = -1;
        int bestMatch = -1;
        Document doc;
        int ret = 0;
        ret = getDocument( file, doc );
        RETNIF_LOGE( ret != 0, ret, "parsejson error, get Document error." );
        RETNIF_LOGE( !doc.HasMember( "ver" ), -1, "parseJson error, no \"ver\"" );
        RETNIF_LOGE( !doc.HasMember( "date" ), -1, "parseJson error, no \"date\"" );
        RETNIF_LOGE( !doc.HasMember( "info" ), -1, "parseJson error, no \"info\"" );
        RETNIF_LOGE( !doc.HasMember( "package" ), -1, "parseJson error, no \"package\"" );
        RETNIF_LOGE( doc["ver"].GetString() == localVer, 1, "parseJson already last version" );
        Value &infoArray = doc["package"];
        RETNIF_LOGE( !infoArray.IsArray(), -1, "parseJson error, \"package\" not array" );

        for ( i = 0; i < infoArray.Size(); i++ )
        {
            const Value &object = infoArray[i];
            string num = object["from"].GetString();

            if( num == "all" )
            {
                fullVerNum = i;
            }

            if( num == localVer )
            {
                bestMatch = i;
                break;
            }
        }

        if( bestMatch == -1 )
        {
            bestMatch = fullVerNum;
        }

        RETNIF_LOGE( bestMatch == -1, -1, "parseJson error, there is no match package !" );
        RETNIF_LOGE( !infoArray[bestMatch].HasMember( "md5" ), -1, "parseJson error, no \"md5\"" );
        RETNIF_LOGE( !infoArray[bestMatch].HasMember( "size" ), -1, "parseJson error, no \"size\"" );
        RETNIF_LOGE( !infoArray[bestMatch].HasMember( "path" ), -1, "parseJson error, no \"path\"" );
        obj = infoArray[bestMatch];
        return 0;
    };

    static string getLocalVer()
    {
        string apVer = CubicCfgGetStr( CUBIC_CFG_version_num );
        size_t size  = apVer.rfind( '.' );
        return apVer.substr( size + 1 );
    };

    static string downloadPackageInfo()
    {
        string fname;
        string jsonpath = CubicCfgGetStr( CUBIC_CFG_update_jsonpath );

        if( access( CUBIC_JSON_FILE, 0 ) == 0 )
        {
            unlink( CUBIC_JSON_FILE );
        }

        return CRemoteReport::downloadOTAFile( jsonpath );
    }

    int updateVersion()
    {
        string fname;
        string lver;
        int ret = -1;
        lver = getLocalVer();
        fname = downloadPackageInfo();
        RETNIF_LOGE( fname.length() <= 0,  -1, "updateVersion, failed when download version info json" );
        Document doc;
        ret = getDocument( fname, doc );
        RETNIF_LOGE( ret != 0, ret, "get Document error." );
        RETNIF_LOGE( !doc.HasMember( "ver" ), -1, "parseJson error, no \"ver\"" );
        RETNIF_LOGE( !doc.HasMember( "date" ), -1, "parseJson error, no \"date\"" );
        RETNIF_LOGE( !doc.HasMember( "info" ), -1, "parseJson error, no \"info\"" );
        m_new_version_date = doc["date"].GetString();
        m_new_version_num = doc["ver"].GetString();
        m_new_version_info = doc["info"].GetString();
        cubic_msg_update arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.version, m_new_version_num.c_str(), CUBIC_VERNO_LEN_MAX );
        strncpy( arg.date, m_new_version_date.c_str(), CUBIC_TIME_LEN_MAX );
        strncpy( arg.info, m_new_version_info.c_str(), CUBIC_INFO_LEN_MAX );
        LOGE( "arg.version :<%s>\targ.date:<%s>\targ.info:<%s>\n", arg.version, arg.date, arg.info );
        ret = CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_NEW_FIRMWARE_NOTIFY, arg );
        ret = CubicPostReq( CUBIC_APP_NAME_CORE, CUBIC_MSG_NEW_FIRMWARE_NOTIFY, arg );
        LOGE( " post msg CUBIC_MSG_NEW_FIRMWARE_NOTIFY result:<%d>\n", ret );
        return 0;
    };

    static void do_OTA_Service( void *arg = NULL )
    {
        UNUSED_ARG( arg );
        int i = 0;

        do
        {
            if( doOTAService() == true )
            {
                break;
            }

            //retry update mamot.json
            string fname = downloadPackageInfo();
            BREAKIF_LOGE( fname.length() <= 0, "do_OTA_Service, failed when download version info json" );
            i++;
        }
        while( i < CUBIC_RETRY_MAX );

        CubicStatSet( CUBIC_STAT_package_upgrade_stat, 0 );  //upgrade end.

        if( i >= CUBIC_RETRY_MAX )
        {
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_FLASH_OPERATION_FAIL_LIGHT );
        }
    };

    static bool checkFile( const char *dirname, const char *cmp_file )
    {
        DIR *dir;
        struct dirent *entry;
        char path[PATH_MAX];
        dir = opendir( dirname );
        LOGE( "OTAService %s,%d start dirname:<%s>\tcmp_file:<%s>", __FUNCTION__, __LINE__, dirname, cmp_file );

        if ( dir == NULL )
        {
            perror( "open dir failed" );
            return false;
        }

        while ( ( entry = readdir( dir ) ) != NULL )
        {
            if( ( entry->d_name[0] == '.' && entry->d_name[1] == 0 ) ||
                    ( entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == 0 ) )
            {
                continue;
            }

            snprintf( path, ( size_t ) PATH_MAX, "%s/%s", dirname, entry->d_name );
            LOGE( "OTAService %s,%d path:<%s>", __FUNCTION__, __LINE__, path );

            if( strcmp( path, cmp_file ) == 0 )
            {
                return true;
            }
        }

        return false;
    };

    static bool doOTAService()
    {
        int ret;
        char cmd_buf[CUBIC_CMD_BUFFER_LEN_MAX + 4] = {0};
        LOGD( "doOTAService" );
        string curr_ver = getLocalVer();
        // if there is no new firmware return
        RETNIF_LOGE( access( CUBIC_JSON_FILE, 0 ) < 0, false, "doOTAService, No version info file !" );
        // choose package as json, if no match package return
        Value obj;
        ret = parseJson( CUBIC_JSON_FILE, curr_ver, obj );
        RETNIF_LOGE( ret < 0, false, "doOTAService, abort when no available package" );
        RETNIF( ret > 0, true );
        string package_md5 = obj["md5"].GetString();
        uint32_t package_size = obj["size"].GetUint();
        string package_path = obj["path"].GetString();
        // download package
        RETNIF_LOGE( package_path.length() <= 0, false, "doOTAService, bad path" );
        LOGD( "doOTAService, start file download: %s", package_path.c_str() );
        string fname = CUBIC_OTA_CACHE"/";
        fname += CUtil::getFileNameOfPath( package_path );

        if( !checkFile( CUBIC_OTA_CACHE, fname.c_str() ) )
        {
            CUtil::syscall( "rm /cache/*.zip", NULL, 0 );
        }

        string connectTimeout = CubicCfgGetStr( CUBIC_CFG_update_connect_timeout );
        string transmitTime = CubicCfgGetStr( CUBIC_CFG_update_max_transmit_time );
        string totalFileSize = CubicCfgGetStr( CUBIC_CFG_update_max_file_size );
        string ota_cmmd = "curl -C - -o " + fname + " " + package_path + " --connect-timeout " + connectTimeout + " -m " + transmitTime + " --max-filesize " + totalFileSize;
        LOGD( "doOTAService, ota command: %s", ota_cmmd.c_str() );
        CUtil::syscall( ota_cmmd.c_str(), cmd_buf, CUBIC_CMD_BUFFER_LEN_MAX );
        LOGD( "doOTAService, ota cmmd result:%s", cmd_buf );
        // check size and md5
        RETNIF_LOGE( package_size != CUtil::getFileSize( fname ), false, "doOTAService, file size not match" );
        string md5 = CUtil::getMd5( fname );
        RETNIF_LOGE( package_md5 != md5, false, "doOTAService, file check sum not match(md5:%s != target:%s)", md5.c_str(), package_md5.c_str() );
        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_FLASH_NEW_FIRMWARE_DOWNLOAD_LIGHT );
        // prepare recovery command
        CUtil::removeDir( "/cache/recovery" );
        RETNIF_LOGE( !CUtil::mkdirAndParrent( "/cache/recovery", S_IRWXU | S_IRWXG | S_IROTH ), false, "doOTAService, failed to create folder /cache/recovery" );
        string cmd = "--update_package=";
        cmd += fname;
        ret = CUtil::WriteFile( "/cache/recovery/command", cmd.c_str(), cmd.length() );
        RETNIF_LOGE( ret != ( int )cmd.length(), false, "doOTAService, failed to write recovery command" );
        // do update
        LOGD( "doOTAService, now reboot to apply new package" );
        unlink( CUBIC_JSON_FILE );
        CUtil::removeDir( CUBIC_SYSTEM_INFO_DIR_PATH );
        sleep( 1 );
        sync();
        CUtil::syscall( "sys_reboot recovery", NULL, 0 );
        return true;
    };

public:
    OTAService()
        : m_start_update_timer( -1 )
        , m_loop_update_timer( -1 )
        , m_new_version_num()
        , m_new_version_date()
        , m_new_version_info()
    {};

    ~OTAService()
    {
    };

    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );
        curl_global_init( CURL_GLOBAL_ALL );
        CubicStatSet( CUBIC_STAT_package_upgrade_stat, 0 );
        setStartUpdateTimer();
        return true;
    };

    void onDeInit()
    {
        cout << "onDeInit" << endl;
        cancelStartUpdateTimer();
        cancelLoopUpdateTimer();
        CubicStatSet( CUBIC_STAT_package_upgrade_stat, 0 );
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        cout << "onMessage: str_src_app_name" << str_src_app_name << "\n"
             << "           n_msg_id" << n_msg_id << "\n"
             << "           p_data" << ( unsigned long )p_data << "\n"
             << endl;

        switch( n_msg_id )
        {
        case CUBIC_MSG_OTA_UPGRADEFIRMWARE:
            // for( int retry = 0; !doOTAService() && retry < CUBIC_RETRY_MAX; retry++ );
            BREAKIF_LOGE( CubicStatGetI( CUBIC_STAT_package_upgrade_stat ) == 1,
                          "Upgrading firmware, not need upgrad once again." )
            CubicStatSet( CUBIC_STAT_package_upgrade_stat, 1 );
            CAsyncRun<>::async_run( do_OTA_Service );
            break;

        case CUBIC_MSG_TEST_UPDATEFIRMWARE:
            BREAKIF_LOGE( CubicStatGetI( CUBIC_STAT_net_connected ) == 0,
                          "network not connected, current network !" )
            updateVersion();
            break;

        default:
            break;
        };

        return 0;
    };
};

IMPLEMENT_CUBIC_APP( OTAService )
#endif
