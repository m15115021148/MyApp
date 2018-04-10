
#include "cubic_inc.h"

#include "CConfig.cc"
#include "CMessager.cc"
#include "CLogger.cc"
#include "CShareStatus.cc"

#include <signal.h>
#include <iostream>

using namespace std;


#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "cmmd"



int main( int argc, const char *argv[] )
{
    CConfig cfg( CUBIC_CONFIG_PATH );
    CShareStatus stat( CUBIC_LOG_TAG );
    CMessager msg( CUBIC_LOG_TAG );
    CLogger log( CUBIC_LOG_TAG );
    cout << "Now: " << CUtil::getTimeString() << endl;

    if( argc == 3 && strcmp( argv[1], "getcfg" ) == 0 )
    {
        string result = cfg.get( argv[2], ( string )"null" );
        cout << result << endl;
        return 0;
    }

    if( argc == 4 && strcmp( argv[1], "setcfg" ) == 0 )
    {
        int ret = cfg.set( argv[2], argv[3] );
        cout << "ret:" << ret << endl;
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "getstat" ) == 0 )
    {
        string result = stat.get<string>( argv[2], "null" );
        cout << result << endl;
        return 0;
    }

    if( argc == 4 && strcmp( argv[1], "setstat" ) == 0 )
    {
        int ret = stat.set( argv[2], argv[3] );
        cout << "ret:" << ret << endl;
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_sim_ready" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_SIM_READY, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_sim_error" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_SIM_ERROR, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_net_connected" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_NETWORK_CONNECTED, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_net_lost" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_NETWORK_LOST, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_reg_success" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_REGISTER_SUCCESS, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_reg_lost" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_REGISTER_LOST, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_gps_renew" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_RENEW, ( int )0 );
        return 0;
    }

    if( argc == 4 && strcmp( argv[1], "v_gps_fence" ) == 0 )
    {
        cubic_msg_gps_fence_evt arg;
        arg.index = CStringTool::fromString<int>( argv[2] );
        arg.is_in = ( string( argv[3] ) == "in" ) ? 1 : 0;
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_FENCE_EVT, arg );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_gps_lost" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_GPS_LOST, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_gps_on" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_ENABLE, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_gps_off" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_DISABLE, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_gps_start" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_HOT_START, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_gps_stop" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_GPS_SERVICE, CUBIC_MSG_GPS_HOT_STOP, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_bat_low" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_BATTERY_LOW, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_bat_out" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_BATTERY_OUT, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_bat_normal" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_BATTERY_NORMAL, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_chg_in" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_IN, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_chg_out" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_OUT, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_chg_full" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_CHARGER_FULL, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_call_recv" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_CALL_RECEIVED, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_call_acpt" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_CALL_ACCEPTED, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_call_hang" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_CALL_HANGUPED, ( int )0 );
        return 0;
    }

    if( argc == 6 && strcmp( argv[1], "v_vm_recv" ) == 0 )
    {
        cubic_msg_vm_received arg;
        strncpy( arg.group_uuid, argv[2], CUBIC_UUID_LEN_MAX );
        strncpy( arg.sender_uuid, argv[3], CUBIC_UUID_LEN_MAX );
        strncpy( arg.created, argv[4], CUBIC_TIME_LEN_MAX );
        strncpy( arg.voice_message_url, argv[5], CUBIC_URL_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_VM_RECEIVED, arg );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_keep_track" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_KEEP_TRACKING, ( int )0 );
        return 0;
    }

    if( argc == 4 && strcmp( argv[1], "v_join_group" ) == 0 )
    {
        cubic_msg_join_group arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.group_uuid, argv[2], CUBIC_UUID_LEN_MAX );
        strncpy( arg.token, argv[3], CUBIC_UUID_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_JOIN_GROUP, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_upload_vm" ) == 0 )
    {
        cubic_msg_test_upload_vm arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.file_path, argv[2], CUBIC_URL_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_UPLOAD_VM, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_download_vm" ) == 0 )
    {
        cubic_msg_test_upload_vm arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.file_path, argv[2], CUBIC_URL_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_DOWNLOAD_VM, arg );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_shutdown" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_POWER_OFF, ( int )0 );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_play_sound" ) == 0 )
    {
        cubic_msg_test_play_sound arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.file_path, argv[2], CUBIC_URL_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_PLAY_SOUND, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_play_loop" ) == 0 )
    {
        cubic_msg_sound_play_start arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.path, argv[2], CUBIC_URL_LEN_MAX );
        arg.loop = 1;
        msg.postRequest( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_PLAY_START, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_play" ) == 0 )
    {
        cubic_msg_sound_play_start arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.path, argv[2], CUBIC_URL_LEN_MAX );
        arg.loop = 0;
        msg.postRequest( CUBIC_APP_NAME_SND_SERVICE, CUBIC_MSG_SOUND_PLAY_START, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_record_sound" ) == 0 )
    {
        cubic_msg_test_record_sound arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.file_path, argv[2], CUBIC_URL_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_RECORD_SOUND, arg );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_rpt_env" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_ENV, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_rpt_shock" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_SHOCK, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_rpt_bat" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_BAT, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_rpt_track" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_TRACK, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_rpt_help" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_HELP, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_ota_check" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_OTA_SERVICE, CUBIC_MSG_TEST_UPDATEFIRMWARE, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_ota_update" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_OTA_SERVICE, CUBIC_MSG_OTA_UPGRADEFIRMWARE, ( int )0 );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_ble" ) == 0 )
    {
        cubic_msg_test_ble arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.param, argv[2], CUBIC_BLE_PARAM_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_TEST, arg );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_ble_on" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_ENABLE, ( int )0  );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_ble_off" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_BLE_INTERFACE, CUBIC_MSG_BLE_DISABLE, ( int )0  );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_net_on" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_NET_SERVICE, CUBIC_MSG_NET_CONNECT, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_net_off" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_NET_SERVICE, CUBIC_MSG_NET_DISCONNECT, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_test_restore" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_RESET_FACTORY, ( int )0 );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_pin_verify" ) == 0 )
    {
        cubic_msg_net_pin_verify arg;
        memset( &arg, 0, sizeof( arg ) );
        strncpy( arg.pin_puk_code, argv[2], CUBIC_PIN_PUK_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_NET_SERVICE, CUBIC_MSG_NET_PIN_VERIFY, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_test_light" ) == 0 )
    {
        cubic_msg_light_test_light arg;
        arg.light = ( int )strtol( argv[2], NULL, 10 );
        msg.postRequest( CUBIC_APP_NAME_LIT_SERVICE, CUBIC_MSG_LIGHT_TEST_LIGHT, arg );
        return 0;
    }

    if( argc == 3 && strcmp( argv[1], "v_test_dial" ) == 0 )
    {
        cubic_msg_test_dial arg;
        strncpy( arg.number, argv[2], CUBIC_UUID_LEN_MAX );
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_DIAL, arg );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_test_poll" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_POLL, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_apn" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_NET_SERVICE, CUBIC_MSG_NET_SET_APN, ( int )0 );
        return 0;
    }

    if( argc == 2 && strcmp( argv[1], "v_test_mic" ) == 0 )
    {
        msg.postRequest( CUBIC_APP_NAME_CORE, CUBIC_MSG_TEST_MIC, ( int )0 );
        return 0;
    }


    cout << "unsupport command !" << endl;
    return 0;
};

