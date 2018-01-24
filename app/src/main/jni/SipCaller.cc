/**
 * file: SipCaller.cc
 * author: thirchina
 * brif: Caller who use pjsua interface
 */
#ifndef _SIP_CALLER_CC_
#define _SIP_CALLER_CC_ 1

#include "cubic_inc.h"

#include "CStatMachine.cc"
#include "CThread.cc"
#include "CStringTool.cc"
#include "UpnpWrapper.cc"
#include "StunWrapper.cc"

#include <rapidjson/document.h>
#include <pjsua.h>
#include <pjsip/sip_msg.h>



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>




#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "SipCaller"


using namespace std;

#define _USE_SOUND_ARRAY 0

#if _USE_SOUND_ARRAY
#include "linked-16k-16b-mono.c"
#endif //_USE_SOUND_ARRAY

class ISipUser
{
public:
	virtual int onIncommingCall(
        const string& wsse, 
        const string& caller, 
        const string& pstn_number, 
        const string& recordid,
        const string& push_call) = 0;
	virtual int onAccept( string info ) = 0;
	virtual int onHangup() = 0;
	virtual int onDTMF(int code) = 0;
	virtual int onStatChange(int stat) = 0;
    virtual int onSound(void* data, int size) = 0;
    virtual int onRegCode(int code) = 0;
    virtual int onCallResult(int code) = 0;

    virtual int onNotifyActive() = 0;
    virtual int onNotifyVoiceMessage(    
        const string& group_id, 
        const string& from_client, 
        const string& created, 
        const string& url) = 0;

    virtual ~ISipUser(){};
};

class SipCaller : public IStatListener
{
public:
    static const int DEF_PJSUA_LOG_LEVEL = 5;
	static const int SOUND_BIT = 16;
   	static const int SOUND_SAMPLE_RATE = 16000;
	static const int SOUND_FRAME_LEN = 20; // ms
	static const int SOUND_SAMPLE_NUM_IN_FRAME = (SOUND_SAMPLE_RATE * SOUND_FRAME_LEN / 1000);
	static const int SOUND_FRAME_SIZE = (SOUND_BIT >> 3) *SOUND_SAMPLE_NUM_IN_FRAME;
    static const int ARG_STR_LEN = 1024;
    static const int DEF_WAIT_TIME = SOUND_FRAME_LEN;

    static const int REG_TIMEOUT = 180; //
    static const int REG_RETRY_INTERVAL = 60; // register to server


    // ============================ status machine ===============================
    enum {
        STAT_NOT_READY = STAT_BEGIN,
        STAT_READY,
        STAT_REGISTERING,
        STAT_IDLE,
        STAT_RINGING,
        STAT_DIALING,
        STAT_CONVERSATION
    };

    enum {
        EVT_INIT = EVT_DEFAULT + 1,  // <struct argument>
        EVT_DEINIT,
        EVT_REGISTER,         // register SIP account // <struct argument>
        EVT_REGISTER_SUCCESS, // register SIP account success
        EVT_REGISTER_FAILED,  // register SIP account failed
        EVT_REGISTER_EXPIRES, // registeration of SIP, should EXPIRES
        EVT_DAIL,             // outgoing call, from User to SIP // <struct argument>
        EVT_DAIL_USER,        // incomming call, from SIP to User
        EVT_HANGUP,           // hange up, by User
        EVT_HANGUP_USER,      // hange up, by SIP
        EVT_ACCEPT,           // accept, by User
        EVT_ACCEPT_USER,      // accept, by SIP
        EVT_ACCEPT_PSTN,      // incomming call, from SIP to accept PSTN // <struct argument>
        EVT_DTMF,             // dtmf, by SIP
        EVT_SEND_DTMF,        // dtmf, by User
    };


    // argument EVT_INIT
    typedef struct evt_init {
        pjsip_transport_type_e   transport_type;
        unsigned short           port;
        char                     stun[ARG_STR_LEN+4];
    } evt_init;

    // argument EVT_REGISTER
    typedef struct evt_register {
        char                     id[ARG_STR_LEN+4];
        char                     registrar[ARG_STR_LEN+4];
        char                     realm[ARG_STR_LEN+4];
        char                     username[ARG_STR_LEN+4];
        char                     password[ARG_STR_LEN+4];
        char                     proxy[ARG_STR_LEN+4];
        unsigned short           port;
        unsigned short           port_range;
    } evt_register;

    // argument EVT_DIAL
    typedef struct evt_dial {
        char                     uri[ARG_STR_LEN];
    } evt_dial;

    // argument EVT_DAIL_USER && EVT_ACCEPT_PSTN
    typedef struct evt_dial_user {
        pjsua_call_id            call_id;
		char                     caller[ARG_STR_LEN+4];
        char                     XWSSE[ARG_STR_LEN+4];
        char                     XPSTNNumber[ARG_STR_LEN+4];
		char                     XAMRecordID[ARG_STR_LEN+4];
		char                     XPushCallID[ARG_STR_LEN+4];
    } evt_dial_user;

    // argument EVT_ACCEPT_USER
    typedef struct evt_accept_user {
        pjsua_call_id            call_id;
    } evt_accept_user;

    typedef evt_dial_user evt_accept_pstn;


    // argument EVT_DTMF
    typedef struct evt_dtmf {
        int                      dtmf_digit;
    } evt_dtmf;


private:
    typedef union Frame {
        uint8_t   bytes[SOUND_FRAME_SIZE+4];
        uint16_t  samples[SOUND_SAMPLE_NUM_IN_FRAME+4];
    } Frame;
    
    ISipUser*              m_user;
    CStatMachine           m_sm;
    pjsua_acc_id           m_account_id;
    pjsua_call_id          m_call_id;
    pjsua_transport_id     m_transport_id;
    pjsua_transport_config m_transport_cfg;
    pjmedia_port*          m_conference_port;
	vector<string>         m_stun_servers;
    pjsip_module           m_sip_module;


#if _USE_SOUND_ARRAY
    uint32_t               m_snd_rec_idx;
    uint32_t               m_snd_play_idx;
#endif //_USE_SOUND_ARRAY

#define DEF_PJ_STR(n, v) \
    char n##str[ARG_STR_LEN] = "v"; \
    pj_str_t n = pj_str(n##str);

    class ReadThread : public CThread
    {
    private:
        Frame m_frame;

    public:
        virtual RunRet run(void* user)
        {
            SipCaller* caller = (SipCaller*)user;
            usleep(1000);
            RETNIF(caller == NULL, RUN_END);
            RETNIF(caller->m_user == NULL, RUN_END);
            RETNIF(caller->m_conference_port == NULL, RUN_END);
            RETNIF(caller->getStat() != STAT_CONVERSATION, RUN_CONTINUE);

            Frame buf;
            pjmedia_frame frame;

            memset(&buf, 0, sizeof(buf));
            memset(&frame, 0, sizeof(frame));
            frame.buf = buf.bytes;
            frame.size = SOUND_FRAME_SIZE;
            frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
			if( PJ_SUCCESS != pjmedia_port_get_frame( caller->m_conference_port, &frame ) )
			{
				LOGD( "SipCaller::ReadThread pjmedia_port_get_frame fail" );
				return RUN_CONTINUE;
			}
            

        #if _USE_SOUND_ARRAY
            if( caller->m_snd_rec_idx + SOUND_FRAME_SIZE > SOUND_ARRAY_SIZE )
            {
                caller->m_snd_rec_idx = 0;
            }
            memcpy(buf.bytes, sound_array + caller->m_snd_rec_idx, SOUND_FRAME_SIZE);
            caller->m_snd_rec_idx += SOUND_FRAME_SIZE;
        #endif //_USE_SOUND_ARRAY

            caller->m_user->onSound(buf.bytes, frame.size);
			return RUN_CONTINUE;
        };
    };
    friend class ReadThread;
    ReadThread m_read_thread;


	class RegWatchThread : public CThread, public UpnpWrapper::UpdateListener, public StunWrapper::UpdateListener
	{
	private:
		static const int TIME_STEP = 180; // 180s
		static const int REG_EXPIRES_TIME = 1800; // 30min
		static const int SLEEP_STEP_LEN = 100000; // 10ms
		static const int SLEEP_STEP = TIME_STEP * 1000000 / SLEEP_STEP_LEN;

		int              m_expire;
		string           m_last_ip;
		string           m_current_ip;
		UpnpWrapper      m_upnp_wrapper;
		StunWrapper      m_stun_wrapper;
		SipCaller*       m_caller;

	public:
	 	virtual void onStart(void* user)
	    {
	    	LOGD("RegWatchThread::onStart");
			m_expire = REG_EXPIRES_TIME;
			m_last_ip = "";
			m_current_ip = "";
			m_caller = (SipCaller*)user;
	    };
		
		virtual RunRet run(void* user)
		{
			UNUSED_ARG(user);
			RETNIF( m_caller == NULL, RUN_END );

			// wait for 3min
			for( int i = 0; i < SLEEP_STEP && !this->needAbort(); i++ )
			{
				usleep( SLEEP_STEP_LEN );
			};
			RETNIF( this->needAbort(), RUN_CONTINUE );

			// check if timeout of register status
			LOGD("RegWatchThread::run stat:%d   m_expire:%d", m_caller->m_sm.current(), m_expire );
			if( m_caller->m_sm.current() < STAT_IDLE )
			{
				m_expire = REG_EXPIRES_TIME;
			}
			else
			{
				m_expire -= TIME_STEP;
			}

			// re-register
			if( m_expire <= 0 )
			{
				LOGD("RegWatchThread::onResolved send event EVT_REGISTER_EXPIRES");
				m_expire = REG_EXPIRES_TIME;
				// THIRCHINA TEMP: disable for sip thread error
				// m_caller->inputEvent( EVT_REGISTER_EXPIRES );
			}

			// check if WAN external IP changed
			// Use UPnP as default, later if UPnP failed, call STUN test in callback
			// THIRCHINA TEMP:m_upnp_wrapper.update(this);
			return RUN_CONTINUE;
		};

		virtual void onExtIPAddress( string ip )
		{
			LOGD("RegWatchThread::onExtIPAddress upnp IP:%s", ip.c_str() );
			m_current_ip = ip;
		};

		virtual void onDone()
		{
			LOGD("RegWatchThread::onDone upnp m_current_ip:%s   m_last_ip:%s", m_current_ip.c_str(), m_last_ip.c_str() );
			if( m_current_ip.length() <= 0 )
			{
				LOGE("RegWatchThread::onDone failed to resolve ExtIPAddress through UPnP !");
				if( m_caller != NULL )
				{
					// THIRCHINA TEMP:m_stun_wrapper.update(this, m_caller->m_stun_servers);
				}
				return;
			}

			if( m_caller != NULL && m_last_ip.length() > 0 &&  m_last_ip != m_current_ip )
			{
				LOGD("RegWatchThread::onDone send event EVT_REGISTER_EXPIRES");
				// THIRCHINA TEMP: disable for sip thread error
                //m_caller->inputEvent( EVT_REGISTER_EXPIRES );
			}
			m_last_ip = m_current_ip;
			m_current_ip = "";
		};

		virtual void onResolved( string ip )
		{
			LOGD("RegWatchThread::onResolved stun ip:%s   m_last_ip:%s", ip.c_str(), m_last_ip.c_str() );
			if( ip.length() <= 0 )
			{
				LOGE("RegWatchThread::onResolved failed to resolve through STUN !");
				return;
			}

			if( m_caller != NULL && m_last_ip.length() > 0 &&  m_last_ip != ip )
			{
				LOGD("RegWatchThread::onResolved send event EVT_REGISTER_EXPIRES");
                // THIRCHINA TEMP: disable for sip thread error
                //m_caller->inputEvent( EVT_REGISTER_EXPIRES );
			}
			m_last_ip = ip;
		};
	};
	friend class RegWatchThread;
    RegWatchThread m_reg_watch_thread;


    // ================================= status machine ========================================
    #define ACT_CLASS(name) name##Act
    #define ACT_NAME(name) m_##name##_act
    #define IMPLEMENT_ACTION(name) \
        class ACT_CLASS(name) : public IAction { \
        private: \
        	SipCaller* m_caller; \
        public: \
            ACT_CLASS(name)(SipCaller* p_caller) : m_caller(p_caller) {}; \
            virtual bool onStatChange(TStat stat_from, TEvt evt, TStat stat_to, void* data) 

    #define IMPLEMENT_ACTION_END(name) \
        };\
        friend class ACT_CLASS(name); \
        ACT_CLASS(name)  ACT_NAME(name);

    void setup_status_machine()
    {
        m_sm.reset();
        m_sm.insertAction(STAT_NOT_READY,      EVT_INIT,               STAT_READY,           &ACT_NAME(init));

        m_sm.insertAction(STAT_READY,          EVT_DEINIT,             STAT_NOT_READY,       &ACT_NAME(deinit));
        m_sm.insertAction(STAT_READY,          EVT_REGISTER,           STAT_REGISTERING,     &ACT_NAME(register));

        m_sm.insertAction(STAT_REGISTERING,    EVT_DEINIT,             STAT_IDLE,            &ACT_NAME(deinit));
        m_sm.insertAction(STAT_REGISTERING,    EVT_REGISTER_SUCCESS,   STAT_IDLE,            &ACT_NAME(reportRegCode));
        m_sm.insertAction(STAT_REGISTERING,    EVT_REGISTER_FAILED,    STAT_READY,           &ACT_NAME(reportRegCode));
		m_sm.insertAction(STAT_REGISTERING,    EVT_REGISTER_EXPIRES,   STAT_REGISTERING,     &ACT_NAME(reregister));

        m_sm.insertAction(STAT_IDLE,           EVT_DEINIT,             STAT_READY,           &ACT_NAME(deinit));
        m_sm.insertAction(STAT_IDLE,           EVT_REGISTER_FAILED,    STAT_READY,           NULL);
        m_sm.insertAction(STAT_IDLE,           EVT_DAIL,               STAT_DIALING,         &ACT_NAME(dial));
        m_sm.insertAction(STAT_IDLE,           EVT_DAIL_USER,          STAT_RINGING,         &ACT_NAME(dialUser));
        m_sm.insertAction(STAT_IDLE,           EVT_ACCEPT_PSTN,        STAT_CONVERSATION,    &ACT_NAME(acceptPstn));
		m_sm.insertAction(STAT_IDLE,           EVT_REGISTER_EXPIRES,   STAT_REGISTERING,     &ACT_NAME(reregister));

        m_sm.insertAction(STAT_DIALING,        EVT_DEINIT,             STAT_READY,           &ACT_NAME(deinit));
		m_sm.insertAction(STAT_DIALING,        EVT_REGISTER_FAILED,    STAT_READY,           &ACT_NAME(hangupUser));
        m_sm.insertAction(STAT_DIALING,        EVT_HANGUP,             STAT_IDLE,            &ACT_NAME(hangup));
        m_sm.insertAction(STAT_DIALING,        EVT_HANGUP_USER,        STAT_IDLE,            &ACT_NAME(hangupUser));
        m_sm.insertAction(STAT_DIALING,        EVT_ACCEPT_USER,        STAT_CONVERSATION,    &ACT_NAME(acceptUser));

        m_sm.insertAction(STAT_RINGING,        EVT_DEINIT,             STAT_READY,           &ACT_NAME(deinit));
		m_sm.insertAction(STAT_RINGING,        EVT_REGISTER_FAILED,    STAT_READY,           &ACT_NAME(hangupUser));
        m_sm.insertAction(STAT_RINGING,        EVT_HANGUP,             STAT_IDLE,            &ACT_NAME(hangup));
        m_sm.insertAction(STAT_RINGING,        EVT_HANGUP_USER,        STAT_IDLE,            &ACT_NAME(hangupUser));
        m_sm.insertAction(STAT_RINGING,        EVT_ACCEPT,             STAT_CONVERSATION,    &ACT_NAME(accept));

        m_sm.insertAction(STAT_CONVERSATION,   EVT_DEINIT,             STAT_READY,           &ACT_NAME(deinit));
		m_sm.insertAction(STAT_CONVERSATION,   EVT_REGISTER_FAILED,    STAT_READY,           &ACT_NAME(hangupUser));
        m_sm.insertAction(STAT_CONVERSATION,   EVT_HANGUP,             STAT_IDLE,            &ACT_NAME(hangup));
        m_sm.insertAction(STAT_CONVERSATION,   EVT_HANGUP_USER,        STAT_IDLE,            &ACT_NAME(hangupUser));
        m_sm.insertAction(STAT_CONVERSATION,   EVT_DTMF,               STAT_CONVERSATION,    &ACT_NAME(dtmf));
		m_sm.insertAction(STAT_CONVERSATION,   EVT_SEND_DTMF,          STAT_CONVERSATION,    &ACT_NAME(sendDtmf));
    };

    IMPLEMENT_ACTION(init)
    {
        bool ret = false;
        pj_status_t stat;
        pjsua_config cfg;
        pjsua_logging_config cfg_log;
        pjsua_media_config cfg_media;

        LOGD("SipCaller.init");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);

        do
        {
            BREAKIF(data == NULL);
            evt_init* arg = (evt_init*)data;
        
            stat = pjsua_create();
            BREAKIF(stat != PJ_SUCCESS);

            pjsua_config_default(&cfg);
            cfg.cb.on_call_state = SipCaller::on_call_state;
            cfg.cb.on_incoming_call = SipCaller::on_incoming_call;
            cfg.cb.on_dtmf_digit = SipCaller::on_dtmf_digit;
            cfg.cb.on_reg_state2 = SipCaller::on_reg_state2;
            cfg.cb.on_call_media_state = SipCaller::on_call_media_state;

            if( arg->stun[0] != '\0' && strcmp(arg->stun, "null") != 0 )
            {
                cfg.stun_map_use_stun2 = PJ_TRUE;
                cfg.stun_srv_cnt = 1;
                cfg.stun_srv[0] = pj_str(const_cast<char*>(arg->stun));
            }

            pjsua_logging_config_default(&cfg_log);
            cfg_log.level = DEF_PJSUA_LOG_LEVEL;
            cfg_log.cb = SipCaller::on_log;

            pjsua_media_config_default(&cfg_media);
            cfg_media.clock_rate = SOUND_SAMPLE_RATE;
            cfg_media.channel_count = 1;
            cfg_media.audio_frame_ptime = SOUND_FRAME_LEN;

            stat = pjsua_init(&cfg, &cfg_log, &cfg_media);
            BREAKIF_LOGE(stat != PJ_SUCCESS, "SipCaller::init failed, pjsua_init !");

            pjsua_transport_config_default(&m_caller->m_transport_cfg);
			if( arg->port > 0 )
			{
				m_caller->m_transport_cfg.port = arg->port;
			}
            stat = pjsua_transport_create(arg->transport_type, &(m_caller->m_transport_cfg), &m_caller->m_transport_id);
            BREAKIF_LOGE(stat != PJ_SUCCESS, "SipCaller::init failed, pjsua_transport_create !");

            stat = pjsua_start();
            BREAKIF_LOGE(stat != PJ_SUCCESS, "SipCaller::init failed, pjsua_start !");

            BREAKIF_LOGE( !m_caller->m_read_thread.start(m_caller),  "SipCaller::init failed, start read thread !");

            pj_str_t id;
            pjsua_codec_set_priority(pj_cstr(&id, "speex/16000"),PJMEDIA_CODEC_PRIO_HIGHEST);
            m_caller->m_conference_port = pjsua_set_no_snd_dev();
            BREAKIF_LOGE(stat != PJ_SUCCESS, "SipCaller::init failed, pjsua_set_no_snd_dev !");

            stat = pjsip_endpt_register_module(pjsua_get_pjsip_endpt(), &(m_caller->m_sip_module));
            BREAKIF_LOGE(stat != PJ_SUCCESS, "SipCaller::init failed, pjsip_endpt_register_module !");

            ret = true;
        } while(0);

        if( !ret )
        {
            pjsua_destroy();
        }
        
        return ret;
    }
    IMPLEMENT_ACTION_END(init);

    IMPLEMENT_ACTION(deinit)
    {
        LOGD("SipCaller.deinit");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);

        pjsip_endpt_unregister_module(pjsua_get_pjsip_endpt(), &(m_caller->m_sip_module));
        m_caller->m_read_thread.stop();
        pjsua_call_hangup_all();
        pjsua_destroy();
        return true;
    }
    IMPLEMENT_ACTION_END(deinit);

    IMPLEMENT_ACTION(register)
    {
        LOGD("SipCaller.register");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);
        RETNIF(data == NULL, false);

        evt_register* arg = (evt_register*)data;
        RETNIF(arg->id == NULL, false);
        RETNIF(arg->registrar == NULL, false);
        RETNIF(arg->realm == NULL, false);
        RETNIF(arg->username == NULL, false);
        RETNIF(arg->password == NULL, false);
        RETNIF(arg->proxy == NULL, false);

        LOGD("SipCaller.register arg->id=%s", arg->id);
        LOGD("SipCaller.register arg->registrar=%s", arg->registrar);
        LOGD("SipCaller.register arg->realm=%s", arg->realm);
        LOGD("SipCaller.register arg->username=%s", arg->username);
        LOGD("SipCaller.register arg->password=%s", arg->password);
        LOGD("SipCaller.register arg->proxy=%s", arg->proxy);
		LOGD("SipCaller.register arg->port=%d", arg->port);

        DEF_PJ_STR(scheme, "Digest")

        pjsua_acc_config cfg;

        pjsua_acc_config_default(&cfg);
		pjsua_transport_config_default(&(cfg.rtp_cfg));

//		cfg.turn_cfg;
        cfg.reg_timeout = REG_TIMEOUT;
        cfg.reg_retry_interval = REG_RETRY_INTERVAL;
        cfg.id = pj_str(arg->id);
        cfg.reg_uri = pj_str(arg->registrar);
        cfg.cred_count = 1;
        cfg.cred_info[0].data_type = 0;
        cfg.cred_info[0].scheme = scheme;
        cfg.cred_info[0].realm = pj_str(arg->realm);
        cfg.cred_info[0].username = pj_str(arg->username);
        cfg.cred_info[0].data = pj_str(arg->password);
        cfg.sip_stun_use = PJSUA_STUN_USE_DEFAULT;
        cfg.media_stun_use = PJSUA_STUN_USE_DEFAULT;
        cfg.timer_setting.sess_expires = 300; // 5min
        cfg.ice_cfg_use = PJSUA_ICE_CONFIG_USE_CUSTOM;
        cfg.ice_cfg.enable_ice = PJ_TRUE;
        cfg.ice_cfg.ice_max_host_cands = -1;  // UNLIMITED
        cfg.ice_cfg.ice_opt.aggressive = PJ_FALSE;
        cfg.turn_cfg.enable_turn = PJ_FALSE;
        cfg.allow_contact_rewrite = 1;  // GLOBAL
        cfg.contact_rewrite_method = 2;  // SINGLE_REQUEST
        if( arg->proxy[0] != '*' && arg->proxy[1] == 0 )
        {
            cfg.proxy[cfg.proxy_cnt] = pj_str(arg->proxy);
            cfg.proxy_cnt++;
        }

		if( arg->port > 0 )
		{
			cfg.rtp_cfg.port = arg->port;

			if( arg->port_range > 0 )
			{
				cfg.rtp_cfg.port_range = arg->port_range;
			}
		}

        if( m_caller->m_account_id != PJSUA_INVALID_ID )
        {
            RETNIF( PJ_SUCCESS != pjsua_acc_modify(m_caller->m_account_id, &cfg), false );
        }
        else
        {
            RETNIF( PJ_SUCCESS != pjsua_acc_add(&cfg, PJ_TRUE, &m_caller->m_account_id), false );
        }
        RETNIF( PJ_SUCCESS != pjsua_acc_set_online_status(m_caller->m_account_id, PJ_TRUE), false );
        return true;
    }
    IMPLEMENT_ACTION_END(register);


    IMPLEMENT_ACTION(reregister)
    {
        LOGD("SipCaller.reregister");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
		UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);
		RETNIF(m_caller->m_account_id < 0, false);

		//RETNIF( PJ_SUCCESS != pjsua_acc_set_registration(m_caller->m_account_id, PJ_FALSE), false );
		//usleep(1000);
        RETNIF( PJ_SUCCESS != pjsua_acc_set_registration(m_caller->m_account_id, PJ_TRUE), false );
        return true;
    }
    IMPLEMENT_ACTION_END(reregister);


    IMPLEMENT_ACTION(dial)
    {
        LOGD("SipCaller.dial");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_user == NULL, false);
        RETNIF(m_caller->m_account_id == PJSUA_INVALID_ID, false);
        RETNIF(data == NULL, false);

        evt_dial* arg = (evt_dial*)data;

        pj_str_t uri = pj_str(arg->uri);

        LOGD("SipCaller.dial uri=%s", arg->uri);

        pjsua_call_id call_id;
        pj_status_t ret = pjsua_call_make_call(m_caller->m_account_id, &uri, NULL, NULL, NULL, &call_id);
        if( m_caller->m_user != NULL )
        {
            m_caller->m_user->onCallResult(ret);
        }
        RETNIF_LOGE( PJ_SUCCESS != ret, false, "SipCaller::dial failed, pjsua_call_make_call error !" );
        m_caller->m_call_id = call_id;

        return true;
    }
    IMPLEMENT_ACTION_END(dial);


    IMPLEMENT_ACTION(dialUser)
    {
        LOGD("SipCaller.dialUser");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_user == NULL, false);
        RETNIF(data == NULL, false);

        evt_dial_user* arg = (evt_dial_user*)data;        
        m_caller->m_call_id = arg->call_id;
        pjsua_call_answer(m_caller->m_call_id, PJSIP_SC_TRYING, NULL, NULL);
        if( m_caller->m_user->onIncommingCall(arg->XWSSE, arg->caller, arg->XPSTNNumber, arg->XAMRecordID, arg->XPushCallID) < 0 )
        {
        	if( strcmp(arg->XAMRecordID, "null") == 0 )
	        {
	        	pjsua_call_hangup(m_caller->m_call_id, PJSIP_SC_INTERNAL_SERVER_ERROR, NULL, NULL);
	        }
			else
			{
            	pjsua_call_hangup(m_caller->m_call_id, PJSIP_SC_NOT_FOUND, NULL, NULL);
			}
            m_caller->m_call_id = PJSUA_INVALID_ID;
            return false;
        }
		
        // tell ringing
        if( strcmp(arg->XAMRecordID, "null") == 0 )
        {
        	pjsua_call_answer(m_caller->m_call_id, PJSIP_SC_RINGING, NULL, NULL);
        }
        return true;
    }
    IMPLEMENT_ACTION_END(dialUser);


    IMPLEMENT_ACTION(hangup)
    {
        LOGD("SipCaller.hangup");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        //UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);

		long* pcode = (long*)data;

        if( m_caller->m_call_id != PJSUA_INVALID_ID )
        {
            pjsua_call_hangup(m_caller->m_call_id, *pcode, NULL, NULL);
            m_caller->m_call_id = PJSUA_INVALID_ID;
        }
        else
        {
            pjsua_call_hangup_all();
        }
        return true;
    }
    IMPLEMENT_ACTION_END(hangup);

    IMPLEMENT_ACTION(hangupUser)
    {
        LOGD("SipCaller.hangupUser");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_user == NULL, false);

        m_caller->m_user->onHangup();
        m_caller->m_call_id = PJSUA_INVALID_ID;
        return true;
    }
    IMPLEMENT_ACTION_END(hangupUser);


    IMPLEMENT_ACTION(accept)
    {
        LOGD("SipCaller.accept");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_call_id == PJSUA_INVALID_ID, false);

        // tell accept
        pjsua_call_answer(m_caller->m_call_id, PJSIP_SC_OK, NULL, NULL);
        return true;
    }
    IMPLEMENT_ACTION_END(accept);

    IMPLEMENT_ACTION(acceptUser)
    {
        LOGD("SipCaller.accept");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_call_id == PJSUA_INVALID_ID, false);

        // tell accept
        RETNIF(m_caller->m_user == NULL, false);
        m_caller->m_user->onAccept( string("null") );
        return true;
    }
    IMPLEMENT_ACTION_END(acceptUser);


    IMPLEMENT_ACTION(acceptPstn)
    {
        LOGD("SipCaller.acceptPstn");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);
        RETNIF(data == NULL, false);

        evt_dial_user* arg = (evt_dial_user*)data;

        m_caller->m_call_id = arg->call_id;
        pjsua_call_answer(m_caller->m_call_id, PJSIP_SC_TRYING, NULL, NULL);
        if( m_caller->m_user->onAccept(arg->XPushCallID) < 0 )
        {
            pjsua_call_hangup(m_caller->m_call_id, PJSIP_SC_INTERNAL_SERVER_ERROR, NULL, NULL);
            m_caller->m_call_id = PJSUA_INVALID_ID;
            return false;
        }

        // tell accept
        pjsua_call_answer(m_caller->m_call_id, PJSIP_SC_OK, NULL, NULL);
        return true;
    }
    IMPLEMENT_ACTION_END(acceptPstn);

    IMPLEMENT_ACTION(dtmf)
    {
        LOGD("SipCaller.dtmf");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_user == NULL, false);
        RETNIF(data == NULL, false);

        evt_dtmf* arg = (evt_dtmf*)data;
        RETNIF( m_caller->m_user->onDTMF(arg->dtmf_digit) < 0, false );
        return true;
    }
    IMPLEMENT_ACTION_END(dtmf);

    IMPLEMENT_ACTION(sendDtmf)
    {
        LOGD("SipCaller.hangup");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        UNUSED_ARG(data);
        RETNIF(m_caller == NULL, false);

        if( m_caller->m_call_id != PJSUA_INVALID_ID )
        {
        	pj_str_t str;
			str.slen = 1;
			str.ptr = (char*)data;
            pjsua_call_dial_dtmf(m_caller->m_call_id, &str);
        }
        return true;
    }
    IMPLEMENT_ACTION_END(sendDtmf);
	

    IMPLEMENT_ACTION(reportRegCode)
    {
        LOGD("SipCaller.dtmf");
        UNUSED_ARG(stat_from);
        UNUSED_ARG(evt);
        UNUSED_ARG(stat_to);
        RETNIF(m_caller == NULL, false);
        RETNIF(m_caller->m_user == NULL, false);
        RETNIF(data == NULL, false);

        int reg_code = *((int*)data);
		LOGE("reportRegCode reg_code=%d", reg_code);
        RETNIF( m_caller->m_user->onRegCode(reg_code) < 0, false );
        return true;
    }
    IMPLEMENT_ACTION_END(reportRegCode);


    // ============================= callbacks for pjsua =======================================
    static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
    {
        RETIF(e == NULL);
        LOGD( "SipCaller.on_call_state call_id=%d event_type=%d", call_id, e->type );
        UNUSED_ARG(e);
        SipCaller* pthis = SipCaller::GetInstance();
        RETIF(call_id != pthis->m_call_id);

        pjsua_call_info info;
        RETIF( PJ_SUCCESS != pjsua_call_get_info(call_id, &info) );

        switch(info.state)
        {
		case PJSIP_INV_STATE_DISCONNECTED:
			pthis->inputEvent(EVT_HANGUP_USER);
			break;
		case PJSIP_INV_STATE_CONFIRMED:
			pthis->inputEvent(EVT_ACCEPT_USER);
			break;
		default:
			break;
	    }
    };


	static bool get_filed_from_sip_msg( const char* msg, const char* key, char* buf, int buf_sz, const char* def = NULL )
	{
		const char* p = strstr(msg, key);

		if(p == NULL)
		{
			RETNIF( def == NULL, false );
			p = def;
		}
		else
		{
			p += strlen(key);
		}

        for( int i = 0; i < buf_sz && *p != 0 && *p != '\n' && *p != '\r'; i++, p++ )
        {
            buf[i] = *p;
        }
		return true;
	};

    static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id, pjsip_rx_data *rdata)
    {
        LOGD("SipCaller.on_incoming_call acc_id=%d  call_id=%d",acc_id, call_id);
        SipCaller* pthis = SipCaller::GetInstance();
        RETIF( pthis == NULL );
        RETIF( pthis->m_account_id != acc_id );

		evt_dial_user arg;

        memset(&arg, 0, sizeof(arg));
		arg.call_id = call_id;
		get_filed_from_sip_msg(rdata->msg_info.msg_buf, "From: ", arg.caller, ARG_STR_LEN, "null" );
		get_filed_from_sip_msg(rdata->msg_info.msg_buf, "X-WSSE: ", arg.XWSSE, ARG_STR_LEN, "null" );
		get_filed_from_sip_msg(rdata->msg_info.msg_buf, "X-AM-Recording-ID: ", arg.XAMRecordID, ARG_STR_LEN, "null" );
		get_filed_from_sip_msg(rdata->msg_info.msg_buf, "X-PSTN-Number: ", arg.XPSTNNumber, ARG_STR_LEN, "null" );
		get_filed_from_sip_msg(rdata->msg_info.msg_buf, "X-Push-Call-ID: ", arg.XPushCallID, ARG_STR_LEN, "null" );

		//LOGD("SipCaller.on_incoming_call arg.caller=%s", arg.caller);
		//LOGD("SipCaller.on_incoming_call arg.XPushCallID=%s", arg.XPushCallID);
        //LOGD("SipCaller.on_incoming_call arg.X_WSSE=%s", arg.XWSSE);
        //LOGD("SipCaller.on_incoming_call arg.X_AM_Record_ID=%s", arg.XAMRecordID);
		//LOGD("SipCaller.on_incoming_call arg.X_PSTN_Number=%s", arg.XPSTNNumber);

		if( strcmp(arg.XPushCallID, "null") != 0 )
		{
			pthis->inputEvent(EVT_ACCEPT_PSTN, &arg);
		}
		else
		{
			pthis->inputEvent(EVT_DAIL_USER, &arg);
		}
    };

    static void on_dtmf_digit(pjsua_call_id call_id, int digit)
    {
        LOGD("SipCaller.on_dtmf_digit call_id=%d  digit=%d", call_id, digit);
        SipCaller* pthis = SipCaller::GetInstance();
        RETIF( pthis == NULL );
        RETIF( pthis->m_call_id != call_id );

        evt_dtmf arg;
        arg.dtmf_digit = digit;
        pthis->inputEvent(EVT_DTMF, &arg);
    };

    static void on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info *info)
    {
        SipCaller* pthis = SipCaller::GetInstance();
	
        LOGD("SipCaller.on_reg_state2 acc_id=%d pthis->m_account_id=%d",acc_id, pthis->m_account_id);
        RETIF( pthis == NULL );
        RETIF( acc_id != pthis->m_account_id );
        RETIF( info == NULL || info->cbparam == NULL );

        LOGD("on_reg_state2 acc_id=%d  info->cbparam->status=%d  info->cbparam->code=%d",
            acc_id, info->cbparam->status, info->cbparam->code);
        if( info->cbparam->status == PJ_SUCCESS && info->cbparam->code < 300 )
        {
            pthis->inputEvent(EVT_REGISTER_SUCCESS, &info->cbparam->code);
        }
        else
        {
            pthis->inputEvent(EVT_REGISTER_FAILED, &info->cbparam->code);
        }
    };

    static void on_call_media_state(pjsua_call_id call_id)
    {
        LOGD("SipCaller.on_call_media_state call_id=%d", call_id);
        SipCaller* pthis = SipCaller::GetInstance();
        RETIF( pthis == NULL );
        RETIF( pthis->m_call_id != call_id );

        pjsua_call_info info; 
        pjsua_call_get_info(call_id, &info);
        if (info.media_status == PJSUA_CALL_MEDIA_ACTIVE ) {
            // When media is active, connect call to sound device.
            pjsua_conf_connect(info.conf_slot, 0);
            pjsua_conf_connect(0, info.conf_slot);
        }
    };

    static void on_log(int level, const char *data, int len)
    {
        LOGD("pjsua_log: %d: %s", level, data);
        UNUSED_ARG(len);
        UNUSED_ARG(level);
        UNUSED_ARG(data);
    };

    static pj_bool_t on_rx_request(pjsip_rx_data *rdata)
    {
        LOGD( "on_rx_request()" );
        RETNIF_LOGE(SipCaller::GetInstance()==NULL, PJ_FALSE, "SipCaller::on_rx_request(): failed by null caller !");

        pjsip_msg *message = rdata->msg_info.msg;
        pj_str_t event_header_key = pj_str(const_cast<char*>("Event"));
        pj_str_t event_header_value = pj_str(const_cast<char*>("mamotel-push"));
        pjsip_event_hdr* event_header = NULL;

        RETNIF( pjsip_method_cmp(&message->line.req.method, &pjsip_notify_method) != 0, PJ_FALSE );
        RETNIF_LOGE( !message || !message->body || message->body->len == 0,
            PJ_FALSE, "SipCaller::on_rx_request() No message body!" );

        event_header = (pjsip_event_hdr*) pjsip_msg_find_hdr_by_name(message, &event_header_key, NULL);
        RETNIF_LOGE( !event_header, 
            PJ_FALSE, "SipCaller::on_rx_request() NOTIFY request does not have event header!" );

        RETNIF_LOGE( pj_stricmp(&event_header->event_type, &event_header_value) != 0, 
            PJ_FALSE, "SipCaller::on_rx_request() NOTIFY request is not a mamotel PUSH event!" );

        LOGD("SipCaller::on_rx_request() Received NOTIFY request from {}:{}", rdata->pkt_info.src_name, rdata->pkt_info.src_port);

        // Respond with 200/OK first
        int status = pjsip_endpt_respond(pjsua_get_pjsip_endpt(), NULL, rdata, 200, NULL, NULL, NULL, NULL);
        if ( status != PJ_SUCCESS ) {
            LOGD("PjsipManager::OnRxRequest() Could not send 200/OK: Error {}", status);
        }

        SipCaller::GetInstance()->HandlePushProviderEvent(message->body->data, message->body->len);
        return PJ_TRUE;
    };


    void HandlePushProviderEvent( void* data, int size )
    {
        LOGD("PjsipManager::HandlePushProviderEvent data: %s", (char*)data);

        rapidjson::Document data_dom;
        RETIF_LOGE( data_dom.Parse( (const char*)data, size ).HasParseError(),
            "PjsipManager::HandlePushProviderEvent error when parse data" );

        rapidjson::Value& value_e3phone = data_dom["e3phone"];
        RETIF_LOGE(!value_e3phone.HasMember("message_type") || !value_e3phone["message_type"].IsString(),
            "PjsipManager::HandlePushProviderEvent can not found message_type" );

        string message_type = value_e3phone["message_type"].GetString();

        if ( message_type == "active_mode" )
        {
            LOGD("PjsipManager::HandlePushProviderEvent detect active_mode");
            if( this->m_user != NULL )
            {
                this->m_user->onNotifyActive();
            }
        }
        else if( message_type == "voice_message" )
        {
            LOGD("PjsipManager::HandlePushProviderEvent detect voice_message");
            RETIF_LOGE(!value_e3phone.HasMember("group_uuid") || !value_e3phone["group_uuid"].IsString(),
                "PjsipManager::HandlePushProviderEvent can not found group_uuid" );
            RETIF_LOGE(!value_e3phone.HasMember("from_client_uuid") || !value_e3phone["from_client_uuid"].IsString(),
                "PjsipManager::HandlePushProviderEvent can not found from_client_uuid" );
            RETIF_LOGE(!value_e3phone.HasMember("created") || !value_e3phone["created"].IsString(),
                "PjsipManager::HandlePushProviderEvent can not found created" );
            RETIF_LOGE(!value_e3phone.HasMember("voice_message_url") || !value_e3phone["voice_message_url"].IsString(),
                "PjsipManager::HandlePushProviderEvent can not found voice_message_url" );

            if( this->m_user != NULL )
            {
                this->m_user->onNotifyVoiceMessage(
                    value_e3phone["group_uuid"].GetString(), 
                    value_e3phone["from_client_uuid"].GetString(), 
                    value_e3phone["created"].GetString(), 
                    value_e3phone["voice_message_url"].GetString());
            }
        }
        else
        {
            LOGD("PjsipManager::HandlePushProviderEvent unhandle message_type:%s", message_type.c_str());
        }
    }

    SipCaller()
        : m_user(NULL)
        , m_sm(this)
        , m_account_id(PJSUA_INVALID_ID)
        , m_call_id(PJSUA_INVALID_ID)
        , m_transport_id(PJSUA_INVALID_ID)
        , m_transport_cfg()
        , m_conference_port(NULL)
        , m_stun_servers()
        , m_read_thread()
        , m_reg_watch_thread()
        , ACT_NAME(init)(this)
        , ACT_NAME(deinit)(this)
        , ACT_NAME(register)(this)
        , ACT_NAME(reregister)(this)
        , ACT_NAME(dial)(this)
        , ACT_NAME(dialUser)(this)
        , ACT_NAME(hangup)(this)
        , ACT_NAME(hangupUser)(this)
        , ACT_NAME(accept)(this)
        , ACT_NAME(acceptUser)(this)
        , ACT_NAME(acceptPstn)(this)
        , ACT_NAME(dtmf)(this)
        , ACT_NAME(sendDtmf)(this)
        , ACT_NAME(reportRegCode)(this)
    {
        // setup status machine
        this->setup_status_machine();
		m_reg_watch_thread.start(this);

        memset( &m_sip_module, 0, sizeof(m_sip_module) );
        m_sip_module.name = pj_str(const_cast<char*>("mod-notify-handler"));
        m_sip_module.id = -1;
        m_sip_module.priority = PJSIP_MOD_PRIORITY_APPLICATION;
        m_sip_module.on_rx_request = SipCaller::on_rx_request;
    };
public:
    static SipCaller* GetInstance()
    {
        static SipCaller instance;
        return &instance;
    };

    ~SipCaller()
    {
        this->inputEvent(EVT_DEINIT);
		m_reg_watch_thread.stop();
    };

    void setStunServers( const vector<string>& stun_servers )
    {
        m_stun_servers.clear();
        m_stun_servers = stun_servers;
        
    };

    void regUser( ISipUser* p_user )
    {
        if( p_user == NULL || m_user == p_user )
        {
            return;
        }
        m_user = p_user;
    };

    int inputEvent(TEvt evt, void* data = NULL)
    {
        return m_sm.input(evt, data);
    };

    int getStat()
    {
        return m_sm.current();
    }

    int writeSound( const void* data, unsigned int size )
    {
        //RETNIF(this->getStat() != STAT_CONVERSATION, size);
        RETNIF(this->m_conference_port == NULL, size );

        pjmedia_frame frame;

        memset( &frame, 0, sizeof(frame) );
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;

    #if _USE_SOUND_ARRAY
        UNUSED_ARG(data);
        for( uint32_t i = 0; i + SOUND_FRAME_SIZE <= size; i+=SOUND_FRAME_SIZE )
        {
            if( this->m_snd_rec_idx + size > SOUND_ARRAY_SIZE )
            {
                this->m_snd_rec_idx = 0;
            }       
            frame.buf = sound_array + this->m_snd_rec_idx;
            frame.size = SOUND_FRAME_SIZE;
            this->m_conference_port->put_frame(this->m_conference_port, &frame);
            this->m_snd_rec_idx += SOUND_FRAME_SIZE;
        }
    #else //_USE_SOUND_ARRAY
        // pull sound from queue
        for( uint32_t i = 0; i + SOUND_FRAME_SIZE <= size; i+=SOUND_FRAME_SIZE )
        {
            frame.buf = (char*)data + i;
            frame.size = SOUND_FRAME_SIZE;
			pjmedia_port_put_frame( this->m_conference_port, &frame );
            // this->m_conference_port->put_frame(this->m_conference_port, &frame);
        }
    #endif //_USE_SOUND_ARRAY
        return size;
    }

    virtual void onStatChange(TStat stat)
    {
    	LOGD("onStatChange: %d", stat);
        RETIF( m_user == NULL );
        m_user->onStatChange(stat);
    }
};

#endif //_SIP_CALLER_CC_
