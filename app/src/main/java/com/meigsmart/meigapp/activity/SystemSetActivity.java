package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.text.TextUtils;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.charlibs.db.BluetoothBean;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.SystemSetAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.blue.BluServerUrlModel;
import com.meigsmart.meigapp.blue.BluSetIntervalsModel;
import com.meigsmart.meigapp.blue.BluetoothConnListener;
import com.meigsmart.meigapp.blue.BluetoothService;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.blue.BluSetPswModel;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.util.DialogUtil;
import com.meigsmart.meigapp.util.RegularUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.SimpleArcDialog;

import java.util.Arrays;

import butterknife.BindArray;
import butterknife.BindView;

/**
 * 蓝牙系统设置 页面
 *
 * @author chenmeng created by 2017/9/6
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class SystemSetActivity extends BaseActivity implements View.OnClickListener ,BluetoothConnListener{
    private SystemSetActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.listView)
    public ListView mLv;//listView
    @BindArray(R.array.SystemSetData) String[] data;//获取数据
    @BindView(R.id.setPswLayout)
    public RelativeLayout mSetPswLayout;//设置密码

    private String serialNum = "";//序列号
    private String password = "";//密码
    private String serialId = "";//保存设备信息id

    private EditText mDialogEt;//设置内容
    private View dialog;//dialog

    private SystemSetAdapter mAdapter;//适配器
    private Intent mBlueS;//服务
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private SimpleArcDialog mDialog;//dialog

    @Override
    protected int getLayoutId() {
        return R.layout.activity_system_set;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mSetPswLayout.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.system_set_title));
        mBlueService = new BluetoothService();
        mBlueService.setmListener(this);

        serialNum = getIntent().getStringExtra("serial_number");
        mDialog = new SimpleArcDialog(this, R.style.MyDialogStyle);

        initListViewData();

        if (!TextUtils.isEmpty(serialNum)){
            if (serialNum.length()<6){
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

    private void getBlueData(){
        if (TextUtils.isEmpty(serialNum))return;
        BluetoothBean bean = MyApplication.getInstance().mBlueDb.getData(serialNum);
        LogUtil.v("result","bean:"+JSON.toJSONString(bean));
        if (bean!=null && !bean.equals("{}") && bean.getId()!=null){
            password = bean.getPassword();
            serialId = bean.getId();
            if (bean.getIsFirstSetPsw().equals("1")){
                if (mAdapter!=null)mAdapter.setData(Arrays.asList(data));
            }
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        startServer();
        getBlueData();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mBlueS!=null)stopService(mBlueS);
        mBlueService.stopScanBlu();
    }

    private void startServer(){
        mBlueS = new Intent(mContext,BluetoothService.class);
        startService(mBlueS);
    }

    /**
     * 初始化listview数据
     */
    private void initListViewData() {
        mAdapter = new SystemSetAdapter();
        mLv.setAdapter(mAdapter);
        mLv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                switch (position){
                    case 0://Group Invitation
                        Intent group = new Intent(mContext,BlueGroupInvitationActivity.class);
                        group.putExtra("serial_number",serialNum);
                        group.putExtra("password",password);
                        group.putExtra("type",0);
                        startActivity(group);
                        break;
                    case 1://获取设备状态
                        Intent status = new Intent(mContext,BlueGetDeviceStatusActivity.class);
                        status.putExtra("serial_number",serialNum);
                        status.putExtra("password",password);
                        startActivity(status);
                        break;
                    case 2://SetIntervals
                        dialog = DialogUtil.customInputDialog(mContext,
                                getResources().getString(R.string.system_set_interval_time),
                                getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialogInterface, int i) {
                                        if (TextUtils.isEmpty(mDialogEt.getText().toString().trim())){
                                            ToastUtil.showBottomShort(getResources().getString(R.string.message_chat_msg_no));
                                            return;
                                        }

                                        if (!RegularUtil.isNumber(mDialogEt.getText().toString().trim()) || Integer.parseInt(mDialogEt.getText().toString().trim()) == 0){
                                            ToastUtil.showBottomShort(getResources().getString(R.string.system_set_interval_number));
                                            return;
                                        }

                                        BluSetIntervalsModel.Intervals intervals = new BluSetIntervalsModel.Intervals();
                                        intervals.setActive(60);
                                        intervals.setPassive(Integer.parseInt(mDialogEt.getText().toString().trim())*60);

                                        BluSetIntervalsModel model = new BluSetIntervalsModel();
                                        model.setMethod(RequestCode.BLUE_SET_INTERVALS);
                                        model.setSerial(serialNum);
                                        model.setPassword(password);
                                        model.setIntervals(intervals);

                                        String test_json_req = JSON.toJSONString(model).replace("intervals","Intervals");
                                        LogUtil.v("result","req:"+test_json_req);
                                        if(mBlueService.startScanBlue(mContext,test_json_req,blueName))mDialog.show();
                                    }
                                },null);
                        mDialogEt = (EditText) dialog.findViewById(R.id.dialog_et_txt);
                        break;
                    case 3://get log
                        Intent log = new Intent(mContext,BlueGetLogActivity.class);
                        log.putExtra("serial_number",serialNum);
                        log.putExtra("password",password);
                        startActivity(log);
                        break;
                    case 4://get apn settings
                        Intent getApn = new Intent(mContext,BlueSetApnActivity.class);
                        getApn.putExtra("serial_number",serialNum);
                        getApn.putExtra("password",password);
                        getApn.putExtra("type",0);
                        startActivity(getApn);
                        break;
                    case 5://get thresholds
                        Intent getThresholds = new Intent(mContext,BlueSetThresholdsActivity.class);
                        getThresholds.putExtra("serial_number",serialNum);
                        getThresholds.putExtra("password",password);
                        getThresholds.putExtra("type",0);
                        startActivity(getThresholds);
                        break;
                    case 6://get GeoFence
                        Intent getGeo = new Intent(mContext,BlueSetGeoFenceActivity.class);
                        getGeo.putExtra("serial_number",serialNum);
                        getGeo.putExtra("password",password);
                        getGeo.putExtra("type",0);
                        startActivity(getGeo);
                        break;
                    case 7://设置sip
                        break;
                    case 8://设置服务器地址
                        dialog = DialogUtil.customInputDialog(mContext,
                                getResources().getString(R.string.system_set_server_url_title),
                                getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                                    @Override
                                    public void onClick(DialogInterface dialogInterface, int i) {
                                        if (TextUtils.isEmpty(mDialogEt.getText().toString().trim())){
                                            ToastUtil.showBottomShort(getResources().getString(R.string.message_chat_msg_no));
                                            return;
                                        }

                                        BluServerUrlModel model = new BluServerUrlModel();
                                        model.setMethod(RequestCode.BLUE_SET_SERVER_URL);
                                        model.setSerial(serialNum);
                                        model.setPassword(password);
                                        model.setServer_url(mDialogEt.getText().toString());

                                        String test_json_req = JSON.toJSONString(model);
                                        LogUtil.v("result","req:"+test_json_req);
                                        if(mBlueService.startScanBlue(mContext,test_json_req,blueName))mDialog.show();
                                    }
                                },null);
                        mDialogEt = (EditText) dialog.findViewById(R.id.dialog_et_txt);
                        break;
                    case 9://设置apn
                        Intent apn = new Intent(mContext,BlueSetApnActivity.class);
                        apn.putExtra("serial_number",serialNum);
                        apn.putExtra("password",password);
                        apn.putExtra("type",1);
                        startActivity(apn);
                        break;
                    case 10://设置门限制
                        Intent thresholds = new Intent(mContext,BlueSetThresholdsActivity.class);
                        thresholds.putExtra("serial_number",serialNum);
                        thresholds.putExtra("password",password);
                        thresholds.putExtra("type",1);
                        startActivity(thresholds);
                        break;
                    case 11://设置定位围栏
                        Intent geo = new Intent(mContext,BlueSetGeoFenceActivity.class);
                        geo.putExtra("serial_number",serialNum);
                        geo.putExtra("password",password);
                        geo.putExtra("type",1);
                        startActivity(geo);
                        break;
                    case 12://设置pin上锁
                        break;
                    case 13://解锁pin
                        break;
                    case 14://解锁puk
                        break;
                    case 15://固件升级
                        Intent upgrade = new Intent(mContext,BlueUpgradeFirmwareActivity.class);
                        upgrade.putExtra("serial_number",serialNum);
                        upgrade.putExtra("password",password);
                        startActivity(upgrade);
                        break;
                    default:
                        break;
                }
            }
        });
    }

    @Override
    public void onClick(View view) {
        if (view == mBack) {
            mContext.finish();
        }
        if (view == mSetPswLayout){
            Intent intent = new Intent(mContext,BlueSetPswActivity.class);
            intent.putExtra("serial_number",serialNum);
            intent.putExtra("serialId",serialId);
            startActivity(intent);
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
                    if (model.getMethod().equals(RequestCode.BLUE_SET_SERVER_URL)){
                        if (model.getResult()==200){
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.system_set_server_url_success));
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }else if (model.getMethod().equals(RequestCode.BLUE_SET_INTERVALS)){
                        if (model.getResult()==200){
                            ToastUtil.showBottomShort("success");
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
