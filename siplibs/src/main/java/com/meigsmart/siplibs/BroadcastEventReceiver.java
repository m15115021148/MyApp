package com.meigsmart.siplibs;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;

import com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastParameters;

import org.pjsip.pjsua2.pjsip_inv_state;
import org.pjsip.pjsua2.pjsip_status_code;

import java.util.ArrayList;

import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.CALL_STATE;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.CODEC_PRIORITIES;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.CODEC_PRIORITIES_SET_STATUS;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.INCOMING_CALL;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.MESSAGE_NOTIFY;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.OUTGOING_CALL;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.REGISTRATION;
import static com.meigsmart.siplibs.BroadcastEventEmitter.BroadcastAction.STACK_STATUS;

/**
 * Reference implementation to receive events emitted by the sip service.
 * Created by chenMeng on 2018/1/11.
 */
public class BroadcastEventReceiver extends BroadcastReceiver {

    private static final String LOG_TAG = "SipServiceBR";

    private Context receiverContext;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null) return;

        receiverContext = context;

        String action = intent.getAction();

        if (action.equals(BroadcastEventEmitter.getAction(REGISTRATION))) {
            int stateCode = intent.getIntExtra(BroadcastParameters.CODE, -1);
            onRegistration(intent.getStringExtra(BroadcastParameters.ACCOUNT_ID),
                    pjsip_status_code.swigToEnum(stateCode));

        } else if (action.equals(BroadcastEventEmitter.getAction(INCOMING_CALL))) {
            onIncomingCall(intent.getStringExtra(BroadcastParameters.ACCOUNT_ID),
                    intent.getIntExtra(BroadcastParameters.CALL_ID, -1),
                    intent.getStringExtra(BroadcastParameters.DISPLAY_NAME),
                    intent.getStringExtra(BroadcastParameters.REMOTE_URI));

        } else if (action.equals(BroadcastEventEmitter.getAction(CALL_STATE))) {
            int callState = intent.getIntExtra(BroadcastParameters.CALL_STATE, -1);
            onCallState(intent.getStringExtra(BroadcastParameters.ACCOUNT_ID),
                    intent.getIntExtra(BroadcastParameters.CALL_ID, -1),
                    pjsip_inv_state.swigToEnum(callState),
                    intent.getLongExtra(BroadcastParameters.CONNECT_TIMESTAMP, -1),
                    intent.getBooleanExtra(BroadcastParameters.LOCAL_HOLD, false),
                    intent.getBooleanExtra(BroadcastParameters.LOCAL_MUTE, false));

        } else if (action.equals(BroadcastEventEmitter.getAction(OUTGOING_CALL))) {
            onOutgoingCall(intent.getStringExtra(BroadcastParameters.ACCOUNT_ID),
                    intent.getIntExtra(BroadcastParameters.CALL_ID, -1),
                    intent.getStringExtra(BroadcastParameters.NUMBER));

        } else if (action.equals(BroadcastEventEmitter.getAction(STACK_STATUS))) {
            onStackStatus(intent.getBooleanExtra(BroadcastParameters.STACK_STARTED, false));

        } else if (action.equals(BroadcastEventEmitter.getAction(CODEC_PRIORITIES))) {
            ArrayList<CodecPriority> codecList = intent.getParcelableArrayListExtra(BroadcastParameters.CODEC_PRIORITIES_LIST);
            onReceivedCodecPriorities(codecList);

        } else if (action.equals(BroadcastEventEmitter.getAction(CODEC_PRIORITIES_SET_STATUS))) {
            onCodecPrioritiesSetStatus(intent.getBooleanExtra(BroadcastParameters.SUCCESS, false));
        } else if (action.equals(BroadcastEventEmitter.getAction(MESSAGE_NOTIFY))){
            onMessageNotify(intent.getStringExtra(BroadcastParameters.ACCOUNT_ID),
                    intent.getStringExtra(BroadcastParameters.FROM_URI),
                    intent.getStringExtra(BroadcastParameters.MESSAGE_BODY));
        }else if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION)){
            onNetworkChange();
        }
    }

    protected Context getReceiverContext() {
        return receiverContext;
    }

    /**
     * Register this broadcast receiver.
     * It's recommended to register the receiver in Activity's onResume method.
     *
     * @param context context in which to register this receiver
     */
    public void register(Context context) {

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BroadcastEventEmitter.getAction(REGISTRATION));
        intentFilter.addAction(BroadcastEventEmitter.getAction(INCOMING_CALL));
        intentFilter.addAction(BroadcastEventEmitter.getAction(CALL_STATE));
        intentFilter.addAction(BroadcastEventEmitter.getAction(OUTGOING_CALL));
        intentFilter.addAction(BroadcastEventEmitter.getAction(STACK_STATUS));
        intentFilter.addAction(BroadcastEventEmitter.getAction(CODEC_PRIORITIES));
        intentFilter.addAction(BroadcastEventEmitter.getAction(CODEC_PRIORITIES_SET_STATUS));
        intentFilter.addAction(BroadcastEventEmitter.getAction(MESSAGE_NOTIFY));
        intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        context.registerReceiver(this, intentFilter);
    }

    /**
     * Unregister this broadcast receiver.
     * It's recommended to unregister the receiver in Activity's onPause method.
     *
     * @param context context in which to unregister this receiver
     */
    public void unregister(final Context context) {
        context.unregisterReceiver(this);
    }

    public void onRegistration(String accountID, pjsip_status_code registrationStateCode) {
        Logger.debug(LOG_TAG, "onRegistration - accountID: " + accountID +
                ", registrationStateCode: " + registrationStateCode);
    }

    public void onIncomingCall(String accountID, int callID, String displayName, String remoteUri) {
        Logger.debug(LOG_TAG, "onIncomingCall - accountID: " + accountID +
                ", callID: " + callID +
                ", displayName: " + displayName +
                ", remoteUri: " + remoteUri);
    }

    public void onCallState(String accountID, int callID, pjsip_inv_state callStateCode,
                            long connectTimestamp, boolean isLocalHold, boolean isLocalMute) {
        Logger.debug(LOG_TAG, "onCallState - accountID: " + accountID +
                ", callID: " + callID +
                ", callStateCode: " + callStateCode +
                ", connectTimestamp: " + connectTimestamp +
                ", isLocalHold: " + isLocalHold +
                ", isLocalMute: " + isLocalMute);
    }

    public void onOutgoingCall(String accountID, int callID, String number) {
        Logger.debug(LOG_TAG, "onOutgoingCall - accountID: " + accountID +
                ", callID: " + callID +
                ", number: " + number);
    }

    public void onStackStatus(boolean started) {
        Logger.debug(LOG_TAG, "SIP service stack " + (started ? "started" : "stopped"));
    }

    public void onReceivedCodecPriorities(ArrayList<CodecPriority> codecPriorities) {
        Logger.debug(LOG_TAG, "Received codec priorities");
        for (CodecPriority codec : codecPriorities) {
            Logger.debug(LOG_TAG, codec.toString());
        }
    }

    public void onCodecPrioritiesSetStatus(boolean success) {
        Logger.debug(LOG_TAG, "Codec priorities " + (success ? "successfully set" : "set error"));
    }

    public void onMessageNotify(String accountID, String fromUri, String msg){
        Logger.debug(LOG_TAG, "accountID: " + accountID +
                ", fromUri: " + fromUri +
                ", msg: " + msg);
    }

    public void onNetworkChange(){
        Logger.debug(LOG_TAG, "net work change");
    }
}
