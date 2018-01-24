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
import com.meigsmart.meigapp.blue.BlueGetLogMode;
import com.meigsmart.meigapp.blue.BluetoothConnListener;
import com.meigsmart.meigapp.blue.BluetoothService;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.SimpleArcDialog;


import butterknife.BindView;

/**
 * get log
 *
 * @author created by 2017-11-30 on chenmeng
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueGetLogActivity extends BaseActivity implements View.OnClickListener, BluetoothConnListener {
    private BlueGetLogActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private String serialNum = "";//序列号
    private String password = "";//密码
    private SimpleArcDialog mDialog;//dialog
    @BindView(R.id.type)
    public TextView mType;//类型
    @BindView(R.id.lines)
    public EditText mLines;//lines
    @BindView(R.id.getLog)
    public TextView mGetLog;//getLog
    private String[] types = {"system", "cdr", "calls", "security", "communicate"};

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_get_log;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(R.string.blue_get_log_title);
        mType.setOnClickListener(this);
        mGetLog.setOnClickListener(this);

        mDialog = new SimpleArcDialog(this, R.style.MyDialogStyle);
        mBlueService = new BluetoothService();
        mBlueService.setmListener(this);

        serialNum = getIntent().getStringExtra("serial_number");
        password = getIntent().getStringExtra("password");

        if (!TextUtils.isEmpty(serialNum)) {
            if (serialNum.length() < 6) {
                return;
            }
            blueName = serialNum.substring(serialNum.length() - 6);
        }

        mDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mBlueService.stopScanBlu();
            }
        });
    }

    @Override
    public void onClick(View v) {
        if (v == mBack) mContext.finish();
        if (v == mType) {
            AlertDialog.Builder builder = new AlertDialog.Builder(this);
            builder.setItems(types, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    mGetLog.setSelected(true);
                    mType.setText(types[which]);
                }
            });
            builder.create().show();
        }
        if (v == mGetLog) {
            if (TextUtils.isEmpty(mType.getText().toString()) || !mGetLog.isSelected()) {
                ToastUtil.showBottomShort(getResources().getString(R.string.blue_get_log_please_type));
                return;
            }

            getLog();

        }
    }

    /**
     * 获取log
     *
     */
    private void getLog() {
        BlueGetLogMode mode = new BlueGetLogMode();

        BlueGetLogMode.GetLog m = new BlueGetLogMode.GetLog();
        m.setType(mType.getText().toString());
        m.setLines(1);

        mode.setMethod(RequestCode.BLUE_GET_LOG);
        mode.setPassword(password);
        mode.setSerial(serialNum);
        mode.setLog(m);

        String req = JSON.toJSONString(mode).replace("log", "Log");
        LogUtil.e("result", "req:" + req);

        if (mBlueService.startScanBlue(mContext, req, blueName)) mDialog.show();
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case 1002:
                    mDialog.dismiss();
                    BlueGetLogMode model = JSON.parseObject(msg.obj.toString(), BlueGetLogMode.class);
                    if (model.getMethod().equals(RequestCode.BLUE_GET_LOG)) {
                        if (model.getResult() == 200) {
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_get_log_success));
                            mContext.finish();
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }else{
                        ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_connection_exception));
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
        mHandler.sendMessage(msg);
    }

    @Override
    public void onStopConnect() {
        mHandler.sendEmptyMessage(1003);
    }
}
