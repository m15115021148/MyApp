package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.text.Html;
import android.text.TextUtils;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.blue.BlueDeviceStatusModel;
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
 * 获取设备状态
 *
 * @author chenmeng created by 2017/12/01
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueGetDeviceStatusActivity extends BaseActivity implements View.OnClickListener ,BluetoothConnListener {
    private BlueGetDeviceStatusActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private String serialNum = "";//序列号
    private String password = "";//密码
    private SimpleArcDialog mDialog;//dialog
    @BindViews({R.id.firm,R.id.mac,R.id.server_url})
    public List<TextView> mListGeneral;//控件
    @BindViews({R.id.reg_status,R.id.call_status})
    public List<TextView> mListCalls;//控件
    @BindViews({R.id.sim_card_status,R.id.network_signal_level,R.id.ipv4_status,R.id.ipv6_status,R.id.mcc,R.id.mnc})
    public List<TextView> mListNetwork;//控件
    @BindViews({R.id.gps_status,R.id.gps_signal_level,R.id.current_latitude,R.id.current_longitude})
    public List<TextView> mListGps;//控件
    @BindViews({R.id.voltage,R.id.batter_level,R.id.charger_status})
    public List<TextView> mListBatter;//控件
    @BindViews({R.id.group_status,R.id.uuid})
    public List<TextView> mListGroup;//控件
    @BindView(R.id.layout)
    public LinearLayout mLayout;//布局
    @BindView(R.id.noData)
    public TextView mEmpty;//空数据
    @BindView(R.id.reload)
    public TextView mReload;//重新加载

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_get_device_status;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.blue_device_status_title));
        mReload.setOnClickListener(this);

        serialNum = getIntent().getStringExtra("serial_number");
        password = getIntent().getStringExtra("password");

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
                mReload.setVisibility(View.VISIBLE);
                mEmpty.setVisibility(View.VISIBLE);
            }
        });

        getStatusData();

    }

    /**
     * 获取数据
     */
    private void getStatusData() {
        BlueDeviceStatusModel model = new BlueDeviceStatusModel();
        model.setPassword(password);
        model.setSerial(serialNum);
        model.setMethod(RequestCode.BLUE_GET_DEVICE_STATUS);
        String req = JSON.toJSONString(model);

        LogUtil.e("result","req:"+req);

        if(mBlueService.startScanBlue(this,req,blueName))mDialog.show();
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case 1002:
                    mDialog.dismiss();
                    BlueDeviceStatusModel model = JSON.parseObject(msg.obj.toString(),BlueDeviceStatusModel.class);
                    if (model.getMethod().equals(RequestCode.BLUE_GET_DEVICE_STATUS)){
                        if (model.getResult()==200){
                            mLayout.setVisibility(View.VISIBLE);
                            mEmpty.setVisibility(View.GONE);
                            mReload.setVisibility(View.GONE);

                            mListGeneral.get(0).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_firm) +"&nbsp;"+ model.getStatus().getGeneral().getFirm()));
                            mListGeneral.get(1).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_mac) +"&nbsp;"+model.getStatus().getGeneral().getMac()));
                            mListGeneral.get(2).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_server_url) +"&nbsp;"+model.getStatus().getGeneral().getServer_url()));

                            mListCalls.get(0).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_reg_status) +"&nbsp;"+model.getStatus().getCalls().getReg_status()));
                            mListCalls.get(1).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_call_status) +"&nbsp;"+model.getStatus().getCalls().getCall_status()));

                            mListNetwork.get(0).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_sim_card_status) +"&nbsp;"+model.getStatus().getVal_Network().getSim_card_status()));
                            mListNetwork.get(1).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_network_signal_level) +"&nbsp;"+model.getStatus().getVal_Network().getSignal_level()));
                            mListNetwork.get(2).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_ipv4_status) +"&nbsp;"+model.getStatus().getVal_Network().getIpv4_status()));
                            mListNetwork.get(3).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_ipv6_status) +"&nbsp;"+model.getStatus().getVal_Network().getIpv6_status()));
                            mListNetwork.get(4).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_mcc) +"&nbsp;"+model.getStatus().getVal_Network().getMcc()));
                            mListNetwork.get(5).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_mnc) +"&nbsp;"+model.getStatus().getVal_Network().getMnc()));

                            mListGps.get(0).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_gps_status) +"&nbsp;"+model.getStatus().getGPS().getStatus()));
                            mListGps.get(1).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_gps_signal_level) +"&nbsp;"+model.getStatus().getGPS().getSignal_level()));
                            mListGps.get(2).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_current_latitude) +"&nbsp;"+model.getStatus().getGPS().getCurrent_latitude()));
                            mListGps.get(3).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_current_longitude) +"&nbsp;"+model.getStatus().getGPS().getCurrent_longitude()));

                            mListBatter.get(0).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_voltage) +"&nbsp;"+model.getStatus().getBattery().getVoltage()));
                            mListBatter.get(1).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_batter_level) +"&nbsp;"+model.getStatus().getBattery().getLevel()));
                            mListBatter.get(2).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_charger_status) +"&nbsp;"+model.getStatus().getBattery().getCharger_status()));

                            mListGroup.get(0).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_group_status) +"&nbsp;"+model.getStatus().getGroup().getStatus()));
                            mListGroup.get(1).setText(Html.fromHtml(getResources().getString(R.string.blue_device_status_uuid) +"&nbsp;"+model.getStatus().getGroup().getUuid()));

                        } else {
                            mLayout.setVisibility(View.GONE);
                            mEmpty.setVisibility(View.VISIBLE);
                            mReload.setVisibility(View.VISIBLE);
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
    public void onClick(View v) {
        if (v == mBack) mContext.finish();
        if (v == mReload)getStatusData();
    }

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
