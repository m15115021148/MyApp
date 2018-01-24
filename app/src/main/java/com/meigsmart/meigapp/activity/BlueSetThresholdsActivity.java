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
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.blue.BlueSetThresholdsModel;
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
 * 设置门限制
 *
 * @author created by 2017-11-30 on chenmeng
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueSetThresholdsActivity extends BaseActivity implements View.OnClickListener,BluetoothConnListener {
    private BlueSetThresholdsActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.more)
    public LinearLayout mSave;//保存
    @BindView(R.id.rightName)
    public TextView rightName;//名称
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private String serialNum = "";//序列号
    private String password = "";//密码
    private SimpleArcDialog mDialog;//dialog
    @BindViews({R.id.shock_x,R.id.shock_y,R.id.shock_z,R.id.tem_upper,R.id.tem_lower,R.id.humidity_upper,R.id.humidity_lower,R.id.pressure_upper,R.id.pressure_lower})
    public List<EditText> mListEt;//集合
    private int type = 0;//类别 0 get ;1 set
    @BindView(R.id.reload)
    public TextView mReload;//重新加载
    @BindView(R.id.layoutEmpty)
    public LinearLayout mLayoutEmpty;//空布局

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_set_thresholds;
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
            mTitle.setText(R.string.blue_set_thresholds_title);
            mSave.setVisibility(View.VISIBLE);
            mLayoutEmpty.setVisibility(View.GONE);

        }else{
            mTitle.setText(R.string.blue_get_thresholds_title);
            mSave.setVisibility(View.GONE);
            mLayoutEmpty.setVisibility(View.VISIBLE);
            mReload.setOnClickListener(this);

            for (EditText et : mListEt){
                et.setFocusable(false);
                et.setEnabled(false);
            }
            getThresholds();
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

    private void getThresholds(){
        BlueSetThresholdsModel model = new BlueSetThresholdsModel();
        model.setMethod(RequestCode.BLUE_GET_THRESHOLDS);
        model.setSerial(serialNum);
        model.setPassword(password);

        String req = JSON.toJSONString(model);
        LogUtil.v("result","req:"+req);
        if (mBlueService.startScanBlue(this,req,blueName)) mDialog.show();
    }

    @Override
    public void onClick(View v) {
        if (v == mBack){
            mContext.finish();
        }
        if (v == mSave){
            if (TextUtils.isEmpty(mListEt.get(0).getText().toString().trim())){
                ToastUtil.showBottomShort(getResources().getString(R.string.blue_set_thresholds_no_empty));
                return;
            }

            BlueSetThresholdsModel model = new BlueSetThresholdsModel();

            BlueSetThresholdsModel.Thresholds m = new BlueSetThresholdsModel.Thresholds();
            m.setShock_x(Float.parseFloat(mListEt.get(0).getText().toString()));
            m.setShock_y(Float.parseFloat(mListEt.get(1).getText().toString()));
            m.setShock_z(Float.parseFloat(mListEt.get(2).getText().toString()));
            m.setTemperature_upper(Float.parseFloat(mListEt.get(3).getText().toString()));
            m.setTemperature_lower(Float.parseFloat(mListEt.get(4).getText().toString()));
            m.setHumidity_upper(Float.parseFloat(mListEt.get(5).getText().toString()));
            m.setHumidity_lower(Float.parseFloat(mListEt.get(6).getText().toString()));
            m.setPressure_upper(Float.parseFloat(mListEt.get(7).getText().toString()));
            m.setPressure_lower(Float.parseFloat(mListEt.get(8).getText().toString()));


            model.setMethod(RequestCode.BLUE_SET_THRESHOLDS);
            model.setThresholds(m);
            model.setPassword(password);
            model.setSerial(serialNum);

            String req = JSON.toJSONString(model).replace("thresholds","Thresholds");
            LogUtil.v("result","req:"+req);

            if (mBlueService.startScanBlue(mContext, req,blueName))mDialog.show();
        }
        if (v == mReload) getThresholds();
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case 1002:
                    mDialog.dismiss();
                    BlueSetThresholdsModel model = JSON.parseObject(msg.obj.toString(),BlueSetThresholdsModel.class);
                    if (msg.arg1 == 1){
                        mLayoutEmpty.setVisibility(View.GONE);
                        if (model.getMethod().equals(RequestCode.BLUE_SET_THRESHOLDS)){
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
                        if (model.getMethod().equals(RequestCode.BLUE_GET_THRESHOLDS)){
                            if (model.getResult() == 200){
                                mLayoutEmpty.setVisibility(View.GONE);

                                mListEt.get(0).setText(String.valueOf(model.getThresholds().getShock_x()));
                                mListEt.get(1).setText(String.valueOf(model.getThresholds().getShock_y()));
                                mListEt.get(2).setText(String.valueOf(model.getThresholds().getShock_z()));
                                mListEt.get(3).setText(String.valueOf(model.getThresholds().getTemperature_upper()));
                                mListEt.get(4).setText(String.valueOf(model.getThresholds().getTemperature_lower()));
                                mListEt.get(5).setText(String.valueOf(model.getThresholds().getHumidity_upper()));
                                mListEt.get(6).setText(String.valueOf(model.getThresholds().getHumidity_lower()));
                                mListEt.get(7).setText(String.valueOf(model.getThresholds().getPressure_upper()));
                                mListEt.get(8).setText(String.valueOf(model.getThresholds().getPressure_lower()));
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
