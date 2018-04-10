/**
 * @file CoreApp.cc
 * @author gang.tian
 * @version 1.0
 * @brief Demo App,test the cubic framework function
 * @detail Demo App,test function and the demo application
 */

#include "CFramework.cc"
#include "cubic_inc.h"
#include <iostream>

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "demo"

class CDemo : public ICubicApp
{
private:
    class CConfigTest : public IListenConfig
    {
    public:
        virtual void onConfigChange( const string &str_key )
        {
            cout << "hello,world" << endl;
            cout << "+++ onConfigChange: " << str_key
                 << "    change to" << CubicCfgGetStr( str_key ) << endl;
        };
    };

    class CTestTimer : public ITimer
    {
    private:
        string m_name;
        int m_idx;
    public:
        CTestTimer( const string &name ) : m_name( name ), m_idx( 0 )
        {
        };

        void onTimer( int n_timer_id )
        {
            cout << "Timer   " << m_name << "\n"
                 << "TimerID " << n_timer_id << "\n"
                 << "Idx     " << m_idx++ << endl;
        };
    };

public:
    bool ConfigTest()
    {
        string test = "hello";
        string test_config = "rw.test.config";
        string test_back;
        CConfig::EErrCode err = CConfig::ERR_NO_ERROR;
        cout << "start set config:" << test << endl;
        CubicCfgSet( "rw.test.config", "hello" );
        CubicCfgCommit( "rw.test.config" );

        if( err != CConfig::ERR_NO_ERROR )
        {
            printf( "CubicSetConfig error,err:%d\n", err );
        }
        else
        {
            printf( "CubicSetConfig %s ok\n", test.c_str() );
        }

        test_back = CubicCfgGetStr( test_config );
        cout << "get config:" << test_back << endl;

        if( test_back == "test" )
        {
            cout << "config test ok" << endl;
        }

        CConfigTest test_listen;
        CubicCfgSetListen( test_config, &test_listen );
        CubicCfgSet( test_config, "test" );
        CubicCfgCommit( test_config );
        CubicCfgUnsetListen( test_config );
        return true;
    };

    bool TestLog()
    {
        CubicLogE( "hello,world,i love you" );
        return true;
    };

    bool TestPostMessager()
    {
        CubicPostReq( "CCore", 1, "nihao" );
        return true;
    };

    bool TestSendRequest()
    {
        char buff[100] = {0};
        CubicSendReq( "CCore", 1, "nihao", buff );
        return true;
    };

    bool TestShareState()
    {
        string key = "test";
        string value = "1";
        string result;

        //CubicSetState(key,value);
        //CubicGetState(key,result);
        if( result == value )
        {
            cout << "state test ok" << endl;
        }

        return true;
    };

    bool TestTimer()
    {
        CTestTimer timer0( "timer0" );
        CTestTimer timer1( "timer1" );
        int n_timer_id0 = CubicSetTimer( 1000, &timer0 );
        int n_timer_id1 = CubicSetTimerInterval( 2000, &timer1 );
        sleep( 5 );
        CubicKillTimer( n_timer_id0 );
        CubicKillTimer( n_timer_id1 );
        return true;
    };

    CDemo()
    {
        ConfigTest();
    };

    ~CDemo()
    {
    };

    bool onInit()
    {
        cout << "onInit" << endl;
        return true;
    };

    void onDeInit()
    {
        cout << "onDeInit" << endl;
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        cout << "onMessage: str_src_app_name" << str_src_app_name << "\n"
             << "           n_msg_id" << n_msg_id << "\n"
             << "           p_data" << ( unsigned long )p_data << "\n"
             << endl;
        return 0;
    };
};

IMPLEMENT_CUBIC_APP( CDemo )

