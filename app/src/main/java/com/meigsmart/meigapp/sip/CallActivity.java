package com.meigsmart.meigapp.sip;

import android.annotation.SuppressLint;
import android.app.KeyguardManager;
import android.content.Context;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.os.SystemClock;
import android.text.TextUtils;
import android.view.View;
import android.widget.Chronometer;
import android.widget.ImageView;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.activity.BaseActivity;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.CallToggleButton;
import com.meigsmart.siplibs.SipServiceCommand;


import org.pjsip.pjsua2.pjsip_inv_state;
import org.pjsip.pjsua2.pjsip_status_code;

import butterknife.BindView;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 */
public class CallActivity extends BaseActivity implements ObserverSip{
    private CallActivity mContext;
    public static final String EXTRA_BUDDY_ID = "buddy_id";

    enum TYPE {
        CALL, INCOMING,
    }

    /* incoming call or call others */
    private TYPE mType;

    @BindView(R.id.btn_hangup)
    public ImageView mHangupBtn;
    @BindView(R.id.text_info)
    public TextView textInfo;
    @BindView(R.id.slider_view)
    public CallToggleButton mCallButton;
    @BindView(R.id.content)
    public TextView mNote;//提示信息
    @BindView(R.id.timer)
    public Chronometer mTimer;//分秒记时

    private PowerManager.WakeLock mWakLock;
    private KeyguardManager.KeyguardLock mKeyguardLock;

    private String remoteUri = "";
    private int callID = 0;

    private String name = "";//名称
    private String sipNumber = "";

    private static final int MAKE_CALL = 0x001;
    private static final int ACCEPT_INCOMING_CALL = 0x002;
    private static final int HANDLE_UP = 0x003;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_call;
    }

    @Override
    protected void initData() {
        mContext = this;
        MyApplication.getInstance().setCallBackSip(this);
        setType();
        setKeyGuardAndWakLock();
        setupUI();
    }

    @Override
    protected void onResume() {
        super.onResume();
        MyApplication.isPush = false;
    }

    @Override
    public void onDestroy() {
        MyApplication.isPush = true;
        super.onDestroy();
    }

    private void setupUI() {
        if (mType == TYPE.INCOMING) {//接入
            mNote.setText(getResources().getString(R.string.call_incoming));
            if (!TextUtils.isEmpty(remoteUri) && remoteUri.contains("@")){
                name = remoteUri.substring(0, remoteUri.indexOf("@"));
                textInfo.setText(String.valueOf(name));
            }
        } else {//拨打
            mNote.setText(getResources().getString(R.string.call_call));
            textInfo.setText(name.equals("")?getResources().getString(R.string.call_unknown):name);
            mHandler.sendEmptyMessageDelayed(MAKE_CALL,1000);
        }
    }

    private void setType() {
        int type = getIntent().getIntExtra(EXTRA_BUDDY_ID,0);
        if (type == 1) {
            mHangupBtn.setVisibility(View.VISIBLE);
            mCallButton.setVisibility(View.GONE);
            mType = TYPE.CALL;
            sipNumber = getIntent().getStringExtra("number");
            name = getIntent().getStringExtra("name");
        } else {
            mType = TYPE.INCOMING;
            mHangupBtn.setVisibility(View.GONE);
            mCallButton.setVisibility(View.VISIBLE);
            remoteUri = getIntent().getStringExtra("remoteUri");
            callID = getIntent().getIntExtra("callID",0);

            mCallButton.setSliderEndListener(new CallToggleButton.SliderListener() {
                @Override
                public void onSliderEnd() {
                    mHandler.sendEmptyMessage(HANDLE_UP);
                }

                @Override
                public void onSliderListen() {//滑动到最左边 接听
                    acceptCall(null);
                }
            });
        }
    }

    private void setKeyGuardAndWakLock() {
        KeyguardManager km = (KeyguardManager) getSystemService(Context.KEYGUARD_SERVICE);
        //得到键盘锁管理器对象
        mKeyguardLock = km.newKeyguardLock("unLock");
        //参数是LogCat里用的Tag
        mKeyguardLock.disableKeyguard();

        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakLock = pm.newWakeLock(PowerManager.ACQUIRE_CAUSES_WAKEUP | PowerManager.SCREEN_DIM_WAKE_LOCK, "bright");
        //获取PowerManager.WakeLock对象,后面的参数|表示同时传入两个值,最后的是LogCat里用的Tag
        mWakLock.acquire();
    }


    /**
     * 取消
     */
    public void hangupCall(View view) {
        if (MyApplication.sipAccount!=null)SipServiceCommand.hangUpActiveCalls(this,MyApplication.sipAccount.getIdUri());
        if (mTimer!=null)mTimer.stop();
        mContext.finish();
    }

    /**
     *接听
     */
    public void acceptCall(View view) {
        mHangupBtn.setVisibility(View.VISIBLE);
        mCallButton.setVisibility(View.GONE);
        mTimer.setVisibility(View.VISIBLE);

        mNote.setText(getResources().getString(R.string.call_line));

        mTimer.setBase(SystemClock.elapsedRealtime());//计时器清零
        mTimer.start();

        if (MyApplication.sipAccount!=null && !TextUtils.isEmpty(MyApplication.sipAccount.getIdUri())){
            mHandler.sendEmptyMessage(ACCEPT_INCOMING_CALL);
        }else{
            mHandler.sendEmptyMessage(HANDLE_UP);
        }
    }

    private void showMsg(String msg) {
        ToastUtil.showBottomShort(msg);
    }

    /**
     * 显示被接听后的ui界面
     */
    private void uiAcceptCall() {
        mTimer.setVisibility(View.VISIBLE);
        mNote.setText(getResources().getString(R.string.call_line));
        textInfo.setText(name);//名称
        mTimer.setBase(SystemClock.elapsedRealtime());//计时器清零
        mTimer.start();
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case MAKE_CALL://makeCall
                    if (MyApplication.sipAccount!=null){
                        SipServiceCommand.makeCall(mContext,MyApplication.sipAccount.getIdUri(),MyApplication.sipAccount.getRemoteUri(sipNumber));
                    }
                    break;
                case ACCEPT_INCOMING_CALL:
                    SipServiceCommand.acceptIncomingCall(mContext,MyApplication.sipAccount.getIdUri(),callID);
                    break;
                case HANDLE_UP:
                    hangupCall(null);
                    break;
            }
        }
    };

    @Override
    public void onBackPressed() {
        return;
    }

    @Override
    public void onRegistration(String accountID, pjsip_status_code registrationStateCode) {

    }

    @Override
    public void onCallState(String accountID, int callID, pjsip_inv_state callStateCode, long connectTimestamp, boolean isLocalHold, boolean isLocalMute) {
        LogUtil.w("CallActivity::state:"+callStateCode);
        if (callStateCode == pjsip_inv_state.PJSIP_INV_STATE_DISCONNECTED){
            mHandler.sendEmptyMessage(HANDLE_UP);
        } else if (callStateCode == pjsip_inv_state.PJSIP_INV_STATE_EARLY){

        } else if (callStateCode == pjsip_inv_state.PJSIP_INV_STATE_CALLING){

        } else if (callStateCode == pjsip_inv_state.PJSIP_INV_STATE_CONFIRMED){
            uiAcceptCall();
        }

    }

    @Override
    public void onMessageNotify(String accountID, String fromUri, String msg) {
    }

    @Override
    public void onOutgoingCall(String accountID, int callID, String number) {
    }

}
