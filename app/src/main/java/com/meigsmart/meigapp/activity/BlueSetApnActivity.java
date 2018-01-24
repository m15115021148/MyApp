package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.blue.BluSetApnSettingsModel;
import com.meigsmart.meigapp.blue.BluetoothConnListener;
import com.meigsmart.meigapp.blue.BluetoothService;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.SimpleArcDialog;

import java.util.List;

import butterknife.BindView;
import butterknife.BindViews;

/**
 * 设置apn
 *
 * @author created by 2017-11-30 on chenmeng
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueSetApnActivity extends BaseActivity implements View.OnClickListener ,BluetoothConnListener {
    private BlueSetApnActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.more)
    public LinearLayout mSave;//保存
    @BindView(R.id.rightName)
    public TextView rightName;//名称
    @BindViews({R.id.profileName, R.id.apn, R.id.auth_type, R.id.user_name, R.id.password, R.id.protocol, R.id.ipv4_primary_dns, R.id.ipv4_secondary_dns, R.id.ipv6_primary_dns, R.id.ipv6_secondary_dns})
    public List<EditText> mListEt;// 集合
    private String[] authTypes = {"PAP","CHAP"};
    private String[] protocols = {"IPV4","IPV6","IPV4_IPV6"};
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private String serialNum = "";//序列号
    private String password = "";//密码
    private SimpleArcDialog mDialog;//dialog
    private int type = 0;//类别 0 get ;1 set
    @BindView(R.id.reload)
    public TextView mReload;//重新加载
    @BindView(R.id.layoutEmpty)
    public LinearLayout mLayoutEmpty;//空布局

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_set_apn;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mSave.setOnClickListener(this);
        rightName.setText(R.string.blue_set_apn_right_name);

        mDialog = new SimpleArcDialog(this, R.style.MyDialogStyle);
        mBlueService = new BluetoothService();
        mBlueService.setmListener(this);

        serialNum = getIntent().getStringExtra("serial_number");
        password = getIntent().getStringExtra("password");
        type = getIntent().getIntExtra("type",0);

        if (!TextUtils.isEmpty(serialNum)){
            if (serialNum.length()<6){
                return;
            }
            blueName = serialNum.substring(serialNum.length() - 6);
        }

        if (type == 1){
            mTitle.setText(R.string.blue_set_apn_title);
            mSave.setVisibility(View.VISIBLE);
            mListEt.get(2).setOnClickListener(this);
            mListEt.get(5).setOnClickListener(this);
            mLayoutEmpty.setVisibility(View.GONE);

        }else{
            mTitle.setText(R.string.blue_get_apn_settings_title);
            mSave.setVisibility(View.GONE);
            mLayoutEmpty.setVisibility(View.VISIBLE);
            mReload.setOnClickListener(this);

            for (EditText et : mListEt){
                et.setFocusable(false);
                et.setEnabled(false);
            }
            getApnSettings();
        }

        mDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mBlueService.stopScanBlu();
                if (type == 0){
                    mLayoutEmpty.setVisibility(View.VISIBLE);
                    mReload.setVisibility(View.VISIBLE);
                }
            }
        });
    }

    private void getApnSettings(){
        BluSetApnSettingsModel model = new BluSetApnSettingsModel();
        model.setSerial(serialNum);
        model.setPassword(password);
        model.setMethod(RequestCode.BLUE_GET_APN_SETTINGS);

        String req = JSON.toJSONString(model);
        LogUtil.v("result","req:"+req);
        if (mBlueService.startScanBlue(this,req,blueName))mDialog.show();
    }

    @Override
    public void onClick(View v) {
        if (v == mBack) {
            mContext.finish();
        }
        if (v == mListEt.get(2)) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setItems(authTypes, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mListEt.get(2).setText(authTypes[which]);
                }
            });
            builder.create().show();
        }
        if (v == mListEt.get(5)) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setItems(protocols, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mListEt.get(5).setText(protocols[which]);
                }
            });
            builder.create().show();
        }
        if (v == mSave) {

            BluSetApnSettingsModel.ApnSettings apn = new BluSetApnSettingsModel.ApnSettings();
            apn.setProfile_name(mListEt.get(0).getText().toString());
            apn.setApn(mListEt.get(1).getText().toString());
            apn.setAuth_type(mListEt.get(2).getText().toString());
            apn.setUser_name(mListEt.get(3).getText().toString());
            apn.setPassword(mListEt.get(4).getText().toString());
            apn.setProtocol(mListEt.get(5).getText().toString());
            apn.setIpv4_primary_dns(mListEt.get(6).getText().toString());
            apn.setIpv4_secondary_dns(mListEt.get(7).getText().toString());
            apn.setIpv6_primary_dns(mListEt.get(8).getText().toString());
            apn.setIpv6_secondary_dns(mListEt.get(9).getText().toString());

            BluSetApnSettingsModel model = new BluSetApnSettingsModel();
            model.setApnProfile(apn);
            model.setMethod(RequestCode.BLUE_SET_APN_SETTINGS);
            model.setPassword(password);
            model.setSerial(serialNum);

            String req = JSON.toJSONString(model).replace("apnProfile","ApnProfile");
            LogUtil.v("result","req:"+req);

            if (mBlueService.startScanBlue(mContext, req,blueName))mDialog.show();
        }
        if (v == mReload) getApnSettings();
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case 1002:
                    mDialog.dismiss();
                    BluSetApnSettingsModel model = JSON.parseObject(msg.obj.toString(),BluSetApnSettingsModel.class);
                    if (msg.arg1 == 1){
                        mLayoutEmpty.setVisibility(View.GONE);
                        if (model.getMethod().equals(RequestCode.BLUE_SET_APN_SETTINGS)){
                            if (model.getResult()==200){
                                ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_set_apn_success));
                                mContext.finish();
                            } else {
                                ToastUtil.showBottomShort(model.getReason());
                            }
                        }else{
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_connection_exception));
                        }
                    }else{
                        mLayoutEmpty.setVisibility(View.VISIBLE);
                        if (model.getMethod().equals(RequestCode.BLUE_GET_APN_SETTINGS)){
                            if (model.getResult() == 200){
                                mLayoutEmpty.setVisibility(View.GONE);

                                mListEt.get(0).setText(model.getApnProfile().getProfile_name());
                                mListEt.get(1).setText(model.getApnProfile().getApn());
                                mListEt.get(2).setText(model.getApnProfile().getAuth_type());
                                mListEt.get(3).setText(model.getApnProfile().getUser_name());
                                mListEt.get(4).setText(model.getApnProfile().getPassword());
                                mListEt.get(5).setText(model.getApnProfile().getProtocol());
                                mListEt.get(6).setText(model.getApnProfile().getIpv4_primary_dns());
                                mListEt.get(7).setText(model.getApnProfile().getIpv4_secondary_dns());
                                mListEt.get(8).setText(model.getApnProfile().getIpv6_primary_dns());
                                mListEt.get(9).setText(model.getApnProfile().getIpv6_secondary_dns());
                            }else{
                                mReload.setVisibility(View.VISIBLE);
                                ToastUtil.showBottomShort(model.getReason());
                            }
                        }else{
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_connection_exception));
                        }
                    }
                    mBlueService.stopScanBlu();
                    break;
                case 1003:
                    ToastUtil.showBottomShort(getResources().getString(R.string.device_set_conn_fail));
                    if (mDialog.isShowing())mDialog.dismiss();
                    mBlueService.stopScanBlu();
                    break;
                default:
                    break;
            }
        }
    };


    @Override
    public void onSuccessConnect() {
    }

    @Override
    public void onCancelConnect() {
    }

    @Override
    public void onCommunication() {
    }

    @Override
    public void onReceiveData(String data) {
        Message msg = mHandler.obtainMessage();
        msg.what = 1002;
        msg.obj = data;
        msg.arg1 = type;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onStopConnect() {
        mHandler.sendEmptyMessage(1003);
    }
}
