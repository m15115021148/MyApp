package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.charlibs.db.BluetoothBean;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.blue.BluSetPswModel;
import com.meigsmart.meigapp.blue.BluetoothConnListener;
import com.meigsmart.meigapp.blue.BluetoothService;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.SimpleArcDialog;

import butterknife.BindView;

/**
 * 设备蓝牙通讯登录页面
 *
 * @author created by 2017/11/27 on chenmeng
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueSetPswActivity extends BaseActivity implements View.OnClickListener,BluetoothConnListener {
    private BlueSetPswActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.oldPsw)
    public EditText mOldPsw;// 密码
    @BindView(R.id.newPsw)
    public EditText mNewPsw;//密码
    @BindView(R.id.fix)
    public TextView mFix;//修改
    private BluetoothService mBlueService;
    private String serialNum = "";//设备序列号
    private String blueName = "";
    private SimpleArcDialog mDialog;//dialog
    private String serialId = "";//id

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_set_psw;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mFix.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.blue_set_psw_title));
        mFix.setSelected(true);

        serialNum = getIntent().getStringExtra("serial_number");
        serialId = getIntent().getStringExtra("serialId");

        if (!TextUtils.isEmpty(serialNum)){
            if (serialNum.length()<6){
                return;
            }
            blueName = serialNum.substring(serialNum.length() - 6);
        }
        mDialog = new SimpleArcDialog(this, R.style.MyDialogStyle);

        mBlueService = new BluetoothService();
        mBlueService.setmListener(this);

        mDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mBlueService.stopScanBlu();
            }
        });

    }


    @Override
    public void onClick(View v) {
        if (v == mBack) {
            mContext.finish();
        }
        if (v == mFix) {
            if (TextUtils.isEmpty(mNewPsw.getText().toString().trim())){
                ToastUtil.showBottomShort(getResources().getString(R.string.device_set_psw_no_empty));
                return;
            }

            BluSetPswModel model = new BluSetPswModel();
            model.setMethod(RequestCode.BLUE_SET_PASSWORD);
            model.setSerial(serialNum);
            model.setPassword(mOldPsw.getText().toString());
            model.setNew_password(mNewPsw.getText().toString());

            String test_json_req = JSON.toJSONString(model);
            LogUtil.v("result","req:"+test_json_req);

            if (mBlueService.startScanBlue(this,test_json_req,blueName))mDialog.show();
        }
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case 1002:
                    mDialog.dismiss();
                    BluSetPswModel model = JSON.parseObject(msg.obj.toString(),BluSetPswModel.class);
                    if (model.getMethod().equals(RequestCode.BLUE_SET_PASSWORD)){
                        if (model.getResult()==200){
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_set_psw_success));
                            if (TextUtils.isEmpty(serialId)){
                                BluetoothBean bean = new BluetoothBean();
                                bean.setSerialNum(serialNum);
                                bean.setPassword(mNewPsw.getText().toString());
                                bean.setIsFirstSetPsw("1");
                                bean.setBlueName(blueName);
                                MyApplication.getInstance().mBlueDb.addData(bean);
                            }else{
                                MyApplication.getInstance().mBlueDb.updatePsw(serialId,mNewPsw.getText().toString());
                            }
                            mContext.finish();
                        }else if (model.getResult()==401){
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_set_psw_wrong_password));
                        }else {
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_set_psw_wrong));
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
    public void onCommunication(){
    }

    @Override
    public void onReceiveData(String data) {
        Message msg = mHandler.obtainMessage();
        msg.what = 1002;
        msg.obj = data;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onStopConnect() {
        mHandler.sendEmptyMessage(1003);
    }
}
