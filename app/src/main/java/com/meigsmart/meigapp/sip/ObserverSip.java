package com.meigsmart.meigapp.sip;

import org.pjsip.pjsua2.pjsip_inv_state;
import org.pjsip.pjsua2.pjsip_status_code;

/**
 * Created by chenMeng on 2018/1/12.
 */

public interface ObserverSip {
    void onRegistration(String accountID, pjsip_status_code registrationStateCode);
    void onCallState(String accountID, int callID, pjsip_inv_state callStateCode, long connectTimestamp, boolean isLocalHold, boolean isLocalMute);
    void onMessageNotify(String accountID, String fromUri, String msg);
    void onOutgoingCall(String accountID, int callID, String number);
}
