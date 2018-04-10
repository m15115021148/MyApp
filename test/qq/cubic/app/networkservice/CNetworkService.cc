#ifndef __CNETWORKSERVICE_CC__
#define __CNETWORKSERVICE_CC__ 1


#include "CFramework.cc"
#include "cubic_inc.h"
#include <iostream>
#include <arpa/inet.h>

extern "C"
{
#include "device_management_service_v01.h"  //dms
#include "network_access_service_v01.h"   //nas
#include "wireless_data_service_v01.h"   //wds
#include "user_identity_module_v01.h"   //uim
#include "qualcomm_mobile_access_point_msgr_v01.h" //qcmap
#include "QCMAP_Client.h"
#include "qmi_client.h"
}

#ifdef CUBIC_LOG_TAG
#undef CUBIC_LOG_TAG
#endif //CUBIC_LOG_TAG
#define CUBIC_LOG_TAG "NetworkService"



#define CUBIC_NET_REFRESH_STAT_TIMEOUT        100000
#define QMI_MAX_TIMEOUT_MS                     10000
#define QMI_CUR_PROFILE_INDEX                  1

class NetworkService : public ICubicApp, public ITimer
{
private:
    QCMAP_Client    *m_qcmap_client;
    qmi_client_type  m_qmi_client_dms;
    qmi_client_type  m_qmi_client_nas;
    qmi_client_type  m_qmi_client_wds;
    qmi_client_type  m_qmi_client_uim;
    int              m_refresh_stat_timer;


    static void qmi_qcmap_ind_cb (
        qmi_client_type user_handle,                    /* QMI user handle       */
        unsigned int    msg_id,                         /* Indicator message ID  */
        void           *ind_buf,                        /* Raw indication data   */
        unsigned int    ind_buf_len,                    /* Raw data length       */
        void           *ind_cb_data                     /* User call back handle */
    )
    {
        NetworkService *p_this = ( NetworkService * )ind_cb_data;
        LOGD( "qmi_qcmap_ind_cb, msg_id=%d", msg_id );

        switch( msg_id )
        {
        case QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01:
            LOGD( "qmi_qcmap_ind_cb, QMI_QCMAP_MSGR_WWAN_STATUS_IND_V01" );
            {
                qcmap_msgr_wwan_status_ind_msg_v01 qmi_ind;
                qmi_client_error_type              qmi_err;
                qmi_err = qmi_client_message_decode( user_handle,
                                                     QMI_IDL_INDICATION,
                                                     msg_id,
                                                     ind_buf,
                                                     ind_buf_len,
                                                     &qmi_ind,
                                                     sizeof( qmi_ind ) );
                BREAKIF_LOGE( qmi_err != QMI_NO_ERR, "qmi_qcmap_ind_cb, decode indication fail" );

                if( qmi_ind.wwan_status < QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTING_V01 )
                {
                    p_this->updateWANStat( qmi_ind.wwan_status, 0 );
                }
                else
                {
                    p_this->updateWANStat( 0, qmi_ind.wwan_status );
                }

                if( qmi_ind.wwan_call_end_reason_valid )
                {
                    LOGE( "qmi_qcmap_ind_cb, call end reason: type:%d, code:%d",
                          qmi_ind.wwan_call_end_reason.wwan_call_end_reason_type,
                          qmi_ind.wwan_call_end_reason.wwan_call_end_reason_code );
                }
            }
            break;

        default:
            break;
        }

        return;
    }

    static void qmi_dms_ind_cb (
        qmi_client_type user_handle,                    /* QMI user handle       */
        unsigned int    msg_id,                         /* Indicator message ID  */
        void           *ind_buf,                        /* Raw indication data   */
        unsigned int    ind_buf_len,                    /* Raw data length       */
        void           *ind_cb_data                     /* User call back handle */
    )
    {
        // NetworkService* p_this = (NetworkService*)ind_cb_data;
        LOGD( "qmi_dms_ind_cb, msg_id=%d", msg_id );

        switch( msg_id )
        {
        case QMI_DMS_EVENT_REPORT_IND_V01:
            LOGD( "qmi_dms_ind_cb, QMI_DMS_EVENT_REPORT_IND_V01" );
            {
                dms_event_report_ind_msg_v01 qmi_ind;
                qmi_client_error_type        qmi_err;
                qmi_err = qmi_client_message_decode( user_handle,
                                                     QMI_IDL_INDICATION,
                                                     msg_id,
                                                     ind_buf,
                                                     ind_buf_len,
                                                     &qmi_ind,
                                                     sizeof( qmi_ind ) );
                BREAKIF_LOGE( qmi_err != QMI_NO_ERR, "qmi_dms_ind_cb, decode indication fail" );

                if( qmi_ind.uim_state_valid )
                {
                    CubicStatSet( CUBIC_STAT_net_uim_state, qmi_ind.uim_state );

                    if( qmi_ind.uim_state == 0 )
                    {
                        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_SIM_READY );
                    }
                    else
                    {
                        CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_SIM_ERROR );
                    }
                }

                if( qmi_ind.pin1_status_valid )
                {
                    CubicStatSet( CUBIC_STAT_net_pin_status,           qmi_ind.pin1_status.status );
                    CubicStatSet( CUBIC_STAT_net_verify_retries_left,  qmi_ind.pin1_status.verify_retries_left );
                    CubicStatSet( CUBIC_STAT_net_unblock_retries_left, qmi_ind.pin1_status.unblock_retries_left );
                }
            }
            break;

        default:
            break;
        }

        return;
    }

    static void qmi_nas_ind_cb (
        qmi_client_type user_handle,                    /* QMI user handle       */
        unsigned int    msg_id,                         /* Indicator message ID  */
        void           *ind_buf,                        /* Raw indication data   */
        unsigned int    ind_buf_len,                    /* Raw data length       */
        void           *ind_cb_data                     /* User call back handle */
    )
    {
        NetworkService *p_this = ( NetworkService * )ind_cb_data;
        LOGD( "qmi_nas_ind_cb, msg_id=%d", msg_id );

        switch( msg_id )
        {
        case QMI_NAS_EVENT_REPORT_IND_MSG_V01:
            LOGD( "qmi_nas_ind_cb, QMI_DMS_EVENT_REPORT_IND_V01" );
            {
                nas_event_report_ind_msg_v01 qmi_ind;
                qmi_client_error_type        qmi_err;
                qmi_err = qmi_client_message_decode( user_handle,
                                                     QMI_IDL_INDICATION,
                                                     msg_id,
                                                     ind_buf,
                                                     ind_buf_len,
                                                     &qmi_ind,
                                                     sizeof( qmi_ind ) );
                BREAKIF_LOGE( qmi_err != QMI_NO_ERR, "qmi_nas_ind_cb, decode indication fail" );

                if( qmi_ind.rsrp_valid )
                {
                    p_this->updateRSRP( qmi_ind.rsrp );
                }
            }
            break;

        case QMI_NAS_NETWORK_TIME_IND_MSG_V01:
            LOGD( "qmi_nas_ind_cb, QMI_NAS_NETWORK_TIME_IND_MSG_V01" );
            {
                nas_network_time_ind_msg_v01 qmi_ind;
                qmi_client_error_type        qmi_err;
                qmi_err = qmi_client_message_decode( user_handle,
                                                     QMI_IDL_INDICATION,
                                                     msg_id,
                                                     ind_buf,
                                                     ind_buf_len,
                                                     &qmi_ind,
                                                     sizeof( qmi_ind ) );
                BREAKIF_LOGE( qmi_err != QMI_NO_ERR, "qmi_nas_ind_cb, decode indication fail" );
                p_this->updateTime(
                    qmi_ind.universal_time.year - 1900,
                    qmi_ind.universal_time.month - 1,
                    qmi_ind.universal_time.day,
                    qmi_ind.universal_time.hour,
                    qmi_ind.universal_time.minute,
                    qmi_ind.universal_time.second );
            }
            break;

        default:
            break;
        }

        return;
    }

    static void qmi_wds_ind_cb (
        qmi_client_type user_handle,                    /* QMI user handle       */
        unsigned int    msg_id,                         /* Indicator message ID  */
        void           *ind_buf,                        /* Raw indication data   */
        unsigned int    ind_buf_len,                    /* Raw data length       */
        void           *ind_cb_data                     /* User call back handle */
    )
    {
        // NetworkService* p_this = (NetworkService*)ind_cb_data;
        LOGD( "qmi_wds_ind_cb, msg_id=%d", msg_id );
        return;
    }

    static void qmi_uim_ind_cb (
        qmi_client_type user_handle,                    /* QMI user handle       */
        unsigned int    msg_id,                         /* Indicator message ID  */
        void           *ind_buf,                        /* Raw indication data   */
        unsigned int    ind_buf_len,                    /* Raw data length       */
        void           *ind_cb_data                     /* User call back handle */
    )
    {
        // NetworkService* p_this = (NetworkService*)ind_cb_data;
        LOGD( "qmi_uim_ind_cb, msg_id=%d", msg_id );
        return;
    }

    bool setupQmiClients()
    {
        LOGD( "setupQmiClients" );
        {
            // init mobile ap
            qmi_error_type_v01 err;
            m_qcmap_client = new QCMAP_Client( qmi_qcmap_ind_cb, this );
            RETNIF_LOGE( m_qcmap_client == NULL || m_qcmap_client->qmi_qcmap_msgr_handle == NULL,
                         false, "failed to create QCMAP client" );
            m_qcmap_client->EnableMobileAP( &err );
            LOGD( "setupQmiClients, enable mobile AP: err=%d", err );
        }
        {
            // init DMS client
            qmi_client_os_params           qmi_os_params;
            qmi_idl_service_object_type    qmi_service_object;
            qmi_client_error_type          qmi_client_error;
            qmi_service_object = dms_get_service_object_v01();
            RETNIF_LOGE( qmi_service_object == NULL, false, "failed to get DMS service object" );
            qmi_client_error = qmi_client_init_instance( qmi_service_object,
                               QMI_CLIENT_INSTANCE_ANY,
                               qmi_dms_ind_cb,
                               this,
                               &qmi_os_params,
                               QMI_MAX_TIMEOUT_MS,
                               &m_qmi_client_dms );
            RETNIF_LOGE( qmi_client_error != QMI_NO_ERR, false, "failed to init DMS qmi client" );
        }
        {
            // init MAS client
            qmi_client_os_params           qmi_os_params;
            qmi_idl_service_object_type    qmi_service_object;
            qmi_client_error_type          qmi_client_error;
            qmi_service_object = nas_get_service_object_v01();
            RETNIF_LOGE( qmi_service_object == NULL, false, "failed to get NAS service object" );
            qmi_client_error = qmi_client_init_instance( qmi_service_object,
                               QMI_CLIENT_INSTANCE_ANY,
                               qmi_nas_ind_cb,
                               this,
                               &qmi_os_params,
                               QMI_MAX_TIMEOUT_MS,
                               &m_qmi_client_nas );
            RETNIF_LOGE( qmi_client_error != QMI_NO_ERR, false, "failed to init NAS qmi client" );
        }
        {
            // init WDS client
            qmi_client_os_params           qmi_os_params;
            qmi_idl_service_object_type    qmi_service_object;
            qmi_client_error_type          qmi_client_error;
            qmi_service_object = wds_get_service_object_v01();
            RETNIF_LOGE( qmi_service_object == NULL, false, "failed to get WDS service object" );
            qmi_client_error = qmi_client_init_instance( qmi_service_object,
                               QMI_CLIENT_INSTANCE_ANY,
                               qmi_wds_ind_cb,
                               this,
                               &qmi_os_params,
                               QMI_MAX_TIMEOUT_MS,
                               &m_qmi_client_wds );
            RETNIF_LOGE( qmi_client_error != QMI_NO_ERR, false, "failed to init WDS qmi client" );
        }
        {
            // init UIM client
            qmi_client_os_params           qmi_os_params;
            qmi_idl_service_object_type    qmi_service_object;
            qmi_client_error_type          qmi_client_error;
            qmi_service_object = uim_get_service_object_v01();
            RETNIF_LOGE( qmi_service_object == NULL, false, "failed to get UIM service object" );
            qmi_client_error = qmi_client_init_instance( qmi_service_object,
                               QMI_CLIENT_INSTANCE_ANY,
                               qmi_uim_ind_cb,
                               this,
                               &qmi_os_params,
                               QMI_MAX_TIMEOUT_MS,
                               &m_qmi_client_uim );
            RETNIF_LOGE( qmi_client_error != QMI_NO_ERR, false, "failed to init UIM qmi client" );
        }
        return true;
    };

    void releaseQmiClients()
    {
        LOGD( "releaseQmiClients" );

        if( m_qcmap_client != NULL )
        {
            qmi_error_type_v01 err;
            m_qcmap_client->DisableMobileAP( &err );
            delete m_qcmap_client;
            m_qcmap_client = NULL;
        }

        if( m_qmi_client_dms != NULL )
        {
            qmi_client_release( m_qmi_client_dms );
            m_qmi_client_dms = NULL;
        };

        if( m_qmi_client_nas != NULL )
        {
            qmi_client_release( m_qmi_client_nas );
            m_qmi_client_nas = NULL;
        };

        if( m_qmi_client_wds != NULL )
        {
            qmi_client_release( m_qmi_client_wds );
            m_qmi_client_wds = NULL;
        };

        if( m_qmi_client_uim != NULL )
        {
            qmi_client_release( m_qmi_client_uim );
            m_qmi_client_uim = NULL;
        };
    };



    void getSN()
    {
        LOGD( "getSN" );
        RETIF( CubicCfgGetStr( CUBIC_CFG_serial_num ).length() > 0 &&
               CubicCfgGetStr( CUBIC_CFG_serial_num ) != "null" );
        RETIF_LOGE( m_qmi_client_dms == NULL, "getSN: qmi client not ready" );
        qmi_client_error_type   qmi_err;
        dms_get_sn_resp_msg_v01 qmi_resp;
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_dms,
                                            QMI_DMS_GET_SN_REQ_V01,
                                            NULL,
                                            0,
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "getSN: failed to QMI_DMS_GET_SN_REQ_V01, ret=%d", qmi_err );
        LOGD( "getSN: got SN number: %s", qmi_resp.sn );
        CUtil::WriteFile( CUBIC_CONFIG_PATH"/ro/core/serial_num", qmi_resp.sn );
    };

    void getBTAddr()
    {
        LOGD( "getBTAddr" );
        RETIF( CubicCfgGetStr( CUBIC_CFG_bt_addr ).length() > 0 &&
               CubicCfgGetStr( CUBIC_CFG_bt_addr ) != "null"    );
        RETIF_LOGE( m_qmi_client_dms == NULL, "getBTAddr: qmi client not ready" );
        qmi_client_error_type            qmi_err;
        dms_get_mac_address_req_msg_v01  qmi_req;
        dms_get_mac_address_resp_msg_v01 qmi_resp;
        memset( &qmi_req, 0, sizeof( qmi_req ) );
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_req.device = DMS_DEVICE_MAC_BT_V01;
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_dms,
                                            QMI_DMS_GET_MAC_ADDRESS_REQ_V01,
                                            &qmi_req,
                                            sizeof( qmi_req ),
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "getBTAddr: failed to QMI_DMS_GET_MAC_ADDRESS_REQ_V01, ret=%d", qmi_err );
        LOGD( "getBTAddr: qmi result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );

        if( qmi_resp.mac_address_valid )
        {
            static const int MAC_ADDR_BUF = 32;
            char mac_address[MAC_ADDR_BUF];
            int n = snprintf( mac_address, MAC_ADDR_BUF, "%02x%02x%02x%02x%02x%02x",
                              qmi_resp.mac_address[5],
                              qmi_resp.mac_address[4],
                              qmi_resp.mac_address[3],
                              qmi_resp.mac_address[2],
                              qmi_resp.mac_address[1],
                              qmi_resp.mac_address[0] );
            LOGD( "getBTAddr: got BT ADDR: %s", mac_address );
            CUtil::WriteFile( CUBIC_CONFIG_PATH"/ro/core/bt_addr", mac_address, n );
        }
    };

    void setupIndDMS_SIM()
    {
        LOGD( "setupIndDMS_SIM" );
        RETIF_LOGE( m_qmi_client_dms == NULL,
                    "setupIndDMS_SIM: qmi client not ready" );
        qmi_client_error_type               qmi_err;
        dms_set_event_report_req_msg_v01    qmi_req;
        dms_set_event_report_resp_msg_v01   qmi_resp;
        memset( &qmi_req, 0, sizeof( qmi_req ) );
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_req.report_uim_state_valid = TRUE;
        qmi_req.report_uim_state = TRUE;
        qmi_req.report_pin_state_valid = TRUE;
        qmi_req.report_pin_state = TRUE;
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_dms,
                                            QMI_DMS_SET_EVENT_REPORT_REQ_V01,
                                            &qmi_req,
                                            sizeof( qmi_req ),
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "setupIndDMS_SIM: failed to QMI_DMS_SET_EVENT_REPORT_REQ_V01, ret=%d", qmi_err );
        LOGD( "setupIndDMS_SIM: qmi result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );
    };

    void setupIndNAS_Signal()
    {
        LOGD( "setupIndNAS_Signal" );
        RETIF_LOGE( m_qmi_client_nas == NULL,
                    "setupIndNAS_Signal: qmi client not ready" );
        qmi_client_error_type               qmi_err;
        nas_set_event_report_req_msg_v01    qmi_req;
        nas_set_event_report_resp_msg_v01   qmi_resp;
        memset( &qmi_req, 0, sizeof( qmi_req ) );
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_req.lte_rsrp_delta_indicator_valid = TRUE;
        qmi_req.lte_rsrp_delta_indicator.report_lte_rsrp = TRUE;
        qmi_req.lte_rsrp_delta_indicator.lte_rsrp_delta = 8;
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_nas,
                                            QMI_NAS_SET_EVENT_REPORT_REQ_MSG_V01,
                                            &qmi_req,
                                            sizeof( qmi_req ),
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "setupIndNAS_Signal: failed to QMI_DMS_SET_EVENT_REPORT_REQ_V01, ret=%d", qmi_err );
        LOGD( "setupIndNAS_Signal: qmi result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );
    };

    void setupIndNAS_other()
    {
        LOGD( "disableIndNAS_unuse" );
        RETIF_LOGE( m_qmi_client_nas == NULL,
                    "disableIndNAS_unuse: qmi client not ready" );
        qmi_client_error_type                  qmi_err;
        nas_indication_register_req_msg_v01    qmi_req;
        nas_indication_register_resp_msg_v01   qmi_resp;
        memset( &qmi_req, 0, sizeof( qmi_req ) );
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_req.req_serving_system_valid = TRUE;
        qmi_req.req_serving_system = FALSE;
        qmi_req.reg_network_time_valid = TRUE;
        qmi_req.reg_network_time = TRUE;
        qmi_req.sys_info_valid = TRUE;
        qmi_req.sys_info = FALSE;
        qmi_req.sig_info_valid = TRUE;
        qmi_req.sig_info = FALSE;
        qmi_req.reg_current_plmn_name_valid = TRUE;
        qmi_req.reg_current_plmn_name = FALSE;
        qmi_req.reg_operator_name_data_valid = TRUE;
        qmi_req.reg_operator_name_data = FALSE;
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_nas,
                                            QMI_NAS_SET_EVENT_REPORT_REQ_MSG_V01,
                                            &qmi_req,
                                            sizeof( qmi_req ),
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "setupIndNAS_Signal: failed to QMI_NAS_SET_EVENT_REPORT_REQ_MSG_V01, ret=%d", qmi_err );
        LOGD( "setupIndNAS_Signal: qmi result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );
    };

    // update all network status by querying
    void updateStatus()
    {
        LOGD( "updateStatus entry" );

        // qcmap stat
        if( m_qcmap_client )
        {
            qmi_error_type_v01              err;
            qcmap_msgr_wwan_status_enum_v01 v4_stat;
            qcmap_msgr_wwan_status_enum_v01 v6_stat;
            boolean ret = m_qcmap_client->GetWWANStatus( &v4_stat, &v6_stat, &err );

            if( ret == FALSE || err != 0 )
            {
                LOGE( "updateStatus, fail to get WWAN status, err=%d", err );
            }
            else
            {
                updateWANStat( v4_stat, v6_stat );
            }
        }

        // sim status
        if( m_qmi_client_dms )
        {
            qmi_client_error_type          qmi_err;
            dms_uim_get_state_resp_msg_v01 qmi_resp;
            memset( &qmi_resp, 0, sizeof( qmi_resp ) );
            qmi_err = qmi_client_send_msg_sync( m_qmi_client_dms,
                                                QMI_DMS_UIM_GET_STATE_REQ_V01,
                                                NULL,
                                                0,
                                                &qmi_resp,
                                                sizeof( qmi_resp ),
                                                QMI_MAX_TIMEOUT_MS );

            if( qmi_err != QMI_NO_ERR )
            {
                LOGE( "updateStatus: failed to QMI_DMS_UIM_GET_STATE_REQ_V01, ret=%d", qmi_err );
            }
            else
            {
                LOGD( "updateStatus: qmi 1 result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );
                updateUimStat( qmi_resp.uim_state );
            }
        }

        // signal
        if( m_qmi_client_nas )
        {
            qmi_client_error_type                qmi_err;
            nas_get_signal_strength_req_msg_v01  qmi_req;
            nas_get_signal_strength_resp_msg_v01 qmi_resp;
            memset( &qmi_req, 0, sizeof( qmi_req ) );
            memset( &qmi_resp, 0, sizeof( qmi_resp ) );
            qmi_req.request_mask_valid = TRUE;
            qmi_req.request_mask |= QMI_NAS_REQUEST_SIG_INFO_LTE_RSRP_MASK_V01;
            qmi_err = qmi_client_send_msg_sync( m_qmi_client_nas,
                                                QMI_NAS_GET_SIGNAL_STRENGTH_REQ_MSG_V01,
                                                &qmi_req,
                                                sizeof( qmi_req ),
                                                &qmi_resp,
                                                sizeof( qmi_resp ),
                                                QMI_MAX_TIMEOUT_MS );

            if( qmi_err != QMI_NO_ERR )
            {
                LOGE( "updateStatus: failed to QMI_NAS_GET_SIGNAL_STRENGTH_REQ_MSG_V01, ret=%d", qmi_err );
            }
            else
            {
                LOGD( "updateStatus: qmi 2 result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );

                if( qmi_resp.lte_rsrp_valid )
                {
                    updateRSRP( qmi_resp.lte_rsrp );
                }
            }
        }

        // plmn, mcc, mnc
        if( m_qmi_client_nas )
        {
            qmi_client_error_type                qmi_err;
            nas_get_serving_system_resp_msg_v01  qmi_resp;
            memset( &qmi_resp, 0, sizeof( qmi_resp ) );
            qmi_err = qmi_client_send_msg_sync( m_qmi_client_nas,
                                                QMI_NAS_GET_SERVING_SYSTEM_REQ_MSG_V01,
                                                NULL,
                                                0,
                                                &qmi_resp,
                                                sizeof( qmi_resp ),
                                                QMI_MAX_TIMEOUT_MS );

            if( qmi_err != QMI_NO_ERR )
            {
                LOGE( "updateStatus: failed to QMI_NAS_GET_SERVING_SYSTEM_REQ_MSG_V01, ret=%d", qmi_err );
            }
            else
            {
                LOGD( "updateStatus: qmi 3 result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );

                if( qmi_resp.current_plmn_valid )
                {
                    updatePlmn( qmi_resp.current_plmn.mobile_country_code,
                                qmi_resp.current_plmn.mobile_network_code );
                }
            }
        }

        // network time
        if( m_qmi_client_nas )
        {
            qmi_client_error_type                qmi_err;
            nas_get_network_time_resp_msg_v01    qmi_resp;
            memset( &qmi_resp, 0, sizeof( qmi_resp ) );
            qmi_err = qmi_client_send_msg_sync( m_qmi_client_nas,
                                                QMI_NAS_GET_NETWORK_TIME_REQ_MSG_V01,
                                                NULL,
                                                0,
                                                &qmi_resp,
                                                sizeof( qmi_resp ),
                                                QMI_MAX_TIMEOUT_MS );

            if( qmi_err != QMI_NO_ERR )
            {
                LOGE( "updateStatus: failed to QMI_NAS_GET_NETWORK_TIME_REQ_MSG_V01, ret=%d", qmi_err );
            }
            else
            {
                LOGD( "updateStatus: qmi 4 result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );

                if( qmi_resp.nas_3gpp_time_valid )
                {
                    updateTime(
                        qmi_resp.nas_3gpp_time.universal_time.year - 1900,
                        qmi_resp.nas_3gpp_time.universal_time.month - 1,
                        qmi_resp.nas_3gpp_time.universal_time.day,
                        qmi_resp.nas_3gpp_time.universal_time.hour,
                        qmi_resp.nas_3gpp_time.universal_time.minute,
                        qmi_resp.nas_3gpp_time.universal_time.second );
                }
                else if ( qmi_resp.nas_3gpp2_time_valid )
                {
                    updateTime(
                        qmi_resp.nas_3gpp2_time.universal_time.year - 1900,
                        qmi_resp.nas_3gpp2_time.universal_time.month - 1,
                        qmi_resp.nas_3gpp2_time.universal_time.day,
                        qmi_resp.nas_3gpp2_time.universal_time.hour,
                        qmi_resp.nas_3gpp2_time.universal_time.minute,
                        qmi_resp.nas_3gpp2_time.universal_time.second );
                }
            }
        }
    };


    void setAPN( const char *apn_id )
    {
        LOGD( "setAPN" );
        RETIF_LOGE( m_qmi_client_wds == NULL,
                    "setAPN: qmi client not ready" );
        RETIF_LOGE( CubicCfgGetVStr( CUBIC_CFG_net_apn_apn_name, apn_id ).length() <= 0,
                    "setAPN: apn[%s] not valid", apn_id );
        qmi_client_error_type                       qmi_err;
        wds_modify_profile_settings_req_msg_v01     qmi_req;
        wds_modify_profile_settings_resp_msg_v01    qmi_resp;
        memset( &qmi_req, 0, sizeof( qmi_req ) );
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_req.profile.profile_type = WDS_PROFILE_TYPE_3GPP_V01;
        qmi_req.profile.profile_index = QMI_CUR_PROFILE_INDEX;
        qmi_req.profile_name_valid = TRUE;
        strncpy( qmi_req.profile_name,
                 CubicCfgGetVStr( CUBIC_CFG_net_apn_profile_name, apn_id ).c_str(),
                 QMI_WDS_PROFILE_NAME_MAX_V01 );
        qmi_req.apn_name_valid = TRUE;
        strncpy( qmi_req.apn_name,
                 CubicCfgGetVStr( CUBIC_CFG_net_apn_apn_name, apn_id ).c_str(),
                 QMI_WDS_APN_NAME_MAX_V01 );
        qmi_req.username_valid = TRUE;
        strncpy( qmi_req.username,
                 CubicCfgGetVStr( CUBIC_CFG_net_apn_user_name, apn_id ).c_str(),
                 QMI_WDS_USER_NAME_MAX_V01 );
        qmi_req.password_valid = TRUE;
        strncpy( qmi_req.password,
                 CubicCfgGetVStr( CUBIC_CFG_net_apn_password, apn_id ).c_str(),
                 QMI_WDS_PASSWORD_MAX_V01 );
        qmi_req.authentication_preference_valid = TRUE;
        qmi_req.authentication_preference = ( wds_auth_pref_mask_v01 )CubicCfgGetVI( CUBIC_CFG_net_apn_auth_type, apn_id );
        qmi_req.pdp_type_valid = TRUE;
        qmi_req.pdp_type = ( wds_pdp_type_enum_v01 )CubicCfgGetVI( CUBIC_CFG_net_apn_protocol, apn_id );
        qmi_req.primary_DNS_IPv4_address_preference_valid = TRUE;
        inet_pton( AF_INET,  CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv4_primary_dns,   apn_id ).c_str(),   &( qmi_req.primary_DNS_IPv4_address_preference ) );
        qmi_req.secondary_DNS_IPv4_address_preference_valid = TRUE;
        inet_pton( AF_INET,  CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv4_secondary_dns, apn_id ).c_str(),   &( qmi_req.secondary_DNS_IPv4_address_preference ) );
        qmi_req.primary_dns_ipv6_address_preference_valid = TRUE;
        inet_pton( AF_INET6, CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv6_primary_dns,   apn_id ).c_str(),   qmi_req.primary_dns_ipv6_address_preference );
        qmi_req.secodnary_dns_ipv6_address_preference_valid = TRUE;
        inet_pton( AF_INET6, CubicCfgGetVStr( CUBIC_CFG_net_apn_ipv6_secondary_dns, apn_id ).c_str(),   qmi_req.secodnary_dns_ipv6_address_preference );
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_wds,
                                            QMI_WDS_MODIFY_PROFILE_SETTINGS_REQ_V01,
                                            &qmi_req,
                                            sizeof( qmi_req ),
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "setAPN: failed to QMI_NAS_GET_NETWORK_TIME_REQ_MSG_V01, ret=%d", qmi_err );
        LOGD( "setAPN: qmi 4 result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );
    };

    void connectData()
    {
        qmi_error_type_v01 err;
        LOGD( "connectData" );
        RETIF_LOGE( m_qcmap_client == NULL,
                    "connectData: qmi client not ready" );

        if( FALSE == m_qcmap_client->SetAutoconnect( TRUE, &err ) || err != 0 )
        {
            LOGE( "connectData, failed to set auto connect err=%d", err );
        }

        if( FALSE == m_qcmap_client->ConnectBackHaul( QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01, &err ) || err != 0 )
        {
            LOGE( "connectData, failed to connect v4, err=%d", err );
        }

        if( FALSE == m_qcmap_client->ConnectBackHaul( QCMAP_MSGR_WWAN_CALL_TYPE_V6_V01, &err ) || err != 0 )
        {
            LOGE( "connectData, failed to connect v6, err=%d", err );
        }
    }

    void disconnectData()
    {
        qmi_error_type_v01 err;
        LOGD( "disconnectData" );
        RETIF_LOGE( m_qcmap_client == NULL,
                    "disconnectData: qmi client not ready" );

        if( FALSE == m_qcmap_client->SetAutoconnect( FALSE, &err ) || err != 0 )
        {
            LOGE( "disconnectData, failed to unset auto connect err=%d", err );
        }

        if( FALSE == m_qcmap_client->DisconnectBackHaul( QCMAP_MSGR_WWAN_CALL_TYPE_V4_V01, &err ) || err != 0 )
        {
            LOGE( "connectData, failed to disconnect v4, err=%d", err );
        }

        if( FALSE == m_qcmap_client->DisconnectBackHaul( QCMAP_MSGR_WWAN_CALL_TYPE_V6_V01, &err ) || err != 0 )
        {
            LOGE( "connectData, failed to disconnect v6, err=%d", err );
        }
    }

    void verifyWithPIN( const char *pin )
    {
        LOGD( "verifyWithPIN" );
        RETIF_LOGE( m_qmi_client_uim == NULL,
                    "verifyWithPIN: qmi client not ready" );
        qmi_client_error_type                qmi_err;
        uim_verify_pin_req_msg_v01           qmi_req;
        uim_verify_pin_resp_msg_v01          qmi_resp;
        memset( &qmi_req, 0, sizeof( qmi_req ) );
        memset( &qmi_resp, 0, sizeof( qmi_resp ) );
        qmi_req.verify_pin.pin_id        = UIM_PIN_ID_PIN_1_V01;
        qmi_req.verify_pin.pin_value_len = strlen( pin );
        strncpy( qmi_req.verify_pin.pin_value, pin, QMI_UIM_PIN_MAX_V01 );
        qmi_err = qmi_client_send_msg_sync( m_qmi_client_nas,
                                            QMI_UIM_VERIFY_PIN_REQ_V01,
                                            &qmi_req,
                                            sizeof( qmi_req ),
                                            &qmi_resp,
                                            sizeof( qmi_resp ),
                                            QMI_MAX_TIMEOUT_MS );
        RETIF_LOGE( qmi_err != QMI_NO_ERR, "verifyWithPIN: failed to QMI_UIM_VERIFY_PIN_REQ_V01, ret=%d", qmi_err );
        LOGD( "verifyWithPIN: qmi result:%d, error:%d", qmi_resp.resp.result, qmi_resp.resp.error );
    };


    void updateWANStat( int v4_stat, int v6_stat )
    {
        LOGD( "updateWANStat, v4_stat:%d, v6_stat:%d", v4_stat, v6_stat );
        int last_v4 = CubicStatGetI( CUBIC_STAT_net_wanstat_v4 );
        int last_v6 = CubicStatGetI( CUBIC_STAT_net_wanstat_v6 );

        if( v4_stat == 0 )
        {
            v4_stat = last_v4;
        }

        if( v6_stat == 0 )
        {
            v6_stat = last_v6;
        }

        RETIF( last_v4 == v4_stat && last_v6 == v6_stat );
        CubicStatSet( CUBIC_STAT_net_wanstat_v4, v4_stat );
        CubicStatSet( CUBIC_STAT_net_wanstat_v6, v6_stat );

        if( v4_stat == QCMAP_MSGR_WWAN_STATUS_CONNECTED_V01 ||
                v6_stat == QCMAP_MSGR_WWAN_STATUS_IPV6_CONNECTED_V01 )
        {
            CubicStatSet( CUBIC_STAT_net_connected, 1 );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_NETWORK_CONNECTED );
        }
        else
        {
            CubicStatSet( CUBIC_STAT_net_connected, 0 );
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_NETWORK_LOST );
        }
    }

    void updateUimStat( int uim_state )
    {
        LOGD( "updateUimStat, uim_state:%d", uim_state );
        int last_stat = CubicStatGetI( CUBIC_STAT_net_uim_state );
        RETIF( last_stat == uim_state );
        CubicStatSet( CUBIC_STAT_net_uim_state, uim_state );

        if( uim_state == 0 )
        {
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_SIM_READY );
        }
        else
        {
            CubicPost( CUBIC_APP_NAME_CORE, CUBIC_MSG_SIM_ERROR );
        }
    }

    void updateRSRP( int rsrp )
    {
        LOGD( "updateRSRP, rsrp:%d", rsrp );
        int last_signal = CubicStatGetI( CUBIC_STAT_net_signal );
        int new_signal = 0;

        if( rsrp >= -80 )
        {
            new_signal = 3;
        }
        else if( rsrp >= -100 )
        {
            new_signal = 2;
        }
        else if( rsrp >= -110 )
        {
            new_signal = 1;
        }

        RETIF( last_signal == new_signal );
        CubicStatSet( CUBIC_STAT_net_signal, new_signal );
    }

    void updatePlmn( int mcc, int mnc )
    {
        LOGD( "updatePlmn, mcc:%d  mnc:%d", mcc, mnc );
        int last_mcc = CubicStatGetI( CUBIC_STAT_net_mcc );
        int last_mnc = CubicStatGetI( CUBIC_STAT_net_mnc );
        RETIF( last_mcc == mcc && last_mnc == mnc );
        CubicStatSet( CUBIC_STAT_net_mcc, mcc );
        CubicStatSet( CUBIC_STAT_net_mnc, mnc );
        // set APN ?
    }

    void updateTime( int year, int mon, int day, int hour, int min, int sec )
    {
        LOGD( "updateTime set time of year:%d mon:%d day:%d, hour:%d, min:%d, sec:%d",
              year, mon, day, hour, min, sec );
        CUtil::setSystemTime( year, mon, day, hour, min, sec );
    }

    void cancelRefreshStatTimer()
    {
        RETIF( m_refresh_stat_timer < 0 );
        LOGD( "cancelRefreshStatTimer" );
        CubicKillTimer( m_refresh_stat_timer );
        m_refresh_stat_timer = -1;
    };

    void setRefreshStatTimer()
    {
        cancelRefreshStatTimer();
        m_refresh_stat_timer = CubicSetTimerInterval( CUBIC_NET_REFRESH_STAT_TIMEOUT, this );
        LOGD( "setRefreshStatTimer id=%d", m_refresh_stat_timer );
    };

public:
    NetworkService()
        : m_qcmap_client( NULL )
        , m_qmi_client_dms( NULL )
        , m_qmi_client_nas( NULL )
        , m_qmi_client_wds( NULL )
        , m_qmi_client_uim( NULL )
    {};

    ~NetworkService()
    {};

    bool onInit()
    {
        LOGD( "%s onInit: %d", CUBIC_THIS_APP, getpid() );

        // init qmi clients
        if( !setupQmiClients() )
        {
            LOGE( "failed to setupQmiClients" );
            releaseQmiClients();
            return false;
        }

        // read SN and BT addr
        getSN();
        getBTAddr();
        // setup indication
        setupIndDMS_SIM();
        setupIndNAS_Signal();
        setupIndNAS_other();
        // get status
        updateStatus();
        // bring up network
        setAPN( CUBIC_APN_CUSTMOIZE );
        connectData();
        return true;
    };

    void onDeInit()
    {
        LOGD( "onDeInit" );
        // tear down network
        disconnectData();
        // release clients
        releaseQmiClients();
        return;
    };

    virtual int onMessage( const string &str_src_app_name, int n_msg_id, const void *p_data )
    {
        LOGD( "onMeassage n_msg_id : %d", n_msg_id );

        switch( n_msg_id )
        {
        case CUBIC_MSG_NET_CONNECT:
            LOGD( "onMeassage, CUBIC_MSG_NET_CONNECT" );
            connectData();
            break;

        case CUBIC_MSG_NET_DISCONNECT:
            LOGD( "onMeassage, CUBIC_MSG_NET_DISCONNECT" );
            disconnectData();
            break;

        case CUBIC_MSG_NET_SET_APN:
            LOGD( "onMeassage, CUBIC_MSG_NET_SET_APN" );
            disconnectData();
            setAPN( CUBIC_APN_CUSTMOIZE );
            connectData();
            break;

        case CUBIC_MSG_NET_PIN_VERIFY:
            LOGD( "onMeassage, CUBIC_MSG_NET_PIN_VERIFY" );
            {
                cubic_msg_net_pin_verify *data = ( cubic_msg_net_pin_verify * )p_data ;
                BREAKIF_LOGE( data == NULL, "CUBIC_MSG_NET_PIN_VERIFY, data is emyty" );
                verifyWithPIN( data->pin_puk_code );
            }
            break;

        default:
            break;
        }

        return 0;
    };

    virtual void onTimer( int n_timer_id )
    {
        if( m_refresh_stat_timer == n_timer_id )
        {
            updateStatus();
        }
    }
};


IMPLEMENT_CUBIC_APP( NetworkService )

#endif

