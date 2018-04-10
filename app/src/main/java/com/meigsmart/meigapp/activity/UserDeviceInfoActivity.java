package com.meigsmart.meigapp.activity;

import android.content.DialogInterface;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.text.InputType;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.BindDeviceModel;
import com.meigsmart.meigapp.model.DeviceInfoModel;
import com.meigsmart.meigapp.scan.QRActivity;
import com.meigsmart.meigapp.sip.CallActivity;
import com.meigsmart.meigapp.util.DialogUtil;
import com.meigsmart.meigapp.util.PreferencesUtil;
import com.meigsmart.meigapp.util.QRCodeUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.GuideView;

import java.net.ConnectException;
import java.util.List;

import butterknife.BindView;
import butterknife.BindViews;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 设备信息
 *
 * @author chenmeng created by 2017/9/7
 */
public class UserDeviceInfoActivity extends BaseActivity implements View.OnClickListener {
    private UserDeviceInfoActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.more)
    public LinearLayout mRightLayout;//绑定设备
    @BindView(R.id.rightName)
    public TextView mRightName;//标题右侧名称
    @BindView(R.id.card_qr)
    public ImageView mQr;//二维码
    private int type = 0;//类别 0 show  1 binding
    private String deviceSerial = "";//设备序列号
    private boolean isBindFirst = true;//是否第一次绑定
    private EditText mBindPsw;//输入的绑定密码
    private String addClientUUid = "";//添加好友 uuid
    private DeviceInfoModel mInfoModel;//信息结果
    @BindView(R.id.deviceNumber)
    public TextView mDeviceNum;
    @BindViews({R.id.callLayout,R.id.trackLayout,R.id.eventLayout,R.id.setLayout})
    public List<LinearLayout> mLayoutList;//布局集合
    private GuideView guideView;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_user_device_info;
    }

    @Override
    protected void initData() {
        mContext = this;
        mTitle.setText(getResources().getString(R.string.user_info_title));
        mBack.setOnClickListener(this);
        mRightLayout.setOnClickListener(this);
        mLayoutList.get(0).setOnClickListener(this);
        mLayoutList.get(1).setOnClickListener(this);
        mLayoutList.get(2).setOnClickListener(this);
        mLayoutList.get(3).setOnClickListener(this);
        mRightName.setText(R.string.user_info_binding);
        if (!MyApplication.isGuide)setGuideView();

        type = getIntent().getIntExtra("type", 0);

        if (type == 0){
            deviceSerial = getIntent().getStringExtra("deviceSerial");
            getUserInfoBySerial(deviceSerial);
        }else if (type == 1){
            mRightLayout.setVisibility(View.VISIBLE);
            String msg = getIntent().getStringExtra("qr_serial_num");
            if (!TextUtils.isEmpty(msg))deviceSerial = msg.substring(2,msg.length());
            getUserInfoBySerial(deviceSerial);
        } else if (type == 2){
            String msg = getIntent().getStringExtra("qr_serial_num");
            if (!TextUtils.isEmpty(msg))deviceSerial = msg.substring(2,msg.length());
            getUserInfoBySerial(deviceSerial);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!MyApplication.isGuide && guideView!=null)guideView.show();
    }

    /**
     * 获取设备信息 serial_number
     */
    private void getUserInfoBySerial(String serial_number) {
        HttpManager.getApiService().getDeviceInfoBySerial(serial_number)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this, getResources().getString(R.string.loading_title)).<DeviceInfoModel>transformer())
                .subscribe(new BaseObserver<DeviceInfoModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }

                    }

                    @Override
                    protected void onSuccess(DeviceInfoModel model) {
                        Bitmap logo = BitmapFactory.decodeResource(mContext.getResources(), R.mipmap.ic_launcher);
                        String msg = "";
                        if (model.getResult().equals("200")) {
                            mInfoModel = model;
                            msg = RequestCode.QR_DEVICE_HEADER + (model.getData() != null ? model.getData().getSerialNumber() : getResources().getString(R.string.user_info_no_result));
                            mDeviceNum.setText("SN: "+model.getData().getSerialNumber());
                            if (model.getData().getBluetoothPwd() == null || model.getData().getBluetoothPwd().equals("")) {
                                isBindFirst = true;
                            } else {
                                isBindFirst = false;
                            }
                        } else {
                            msg = model.getReason();
                            ToastUtil.showBottomShort(model.getReason());
                        }
                        mQr.setImageBitmap(QRCodeUtil.createQRImage(msg, null));
                    }
                });
    }

    /**
     * 绑定设备
     *
     * @param serial_number
     */
    private void bindDevice(String serial_number) {
        HttpManager.getApiService().bindDevice(serial_number)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this, getResources().getString(R.string.loading_title)).<BindDeviceModel>transformer())
                .subscribe(new BaseObserver<BindDeviceModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                        if (!MyApplication.isGuide && guideView!=null)guideView.show();
                    }

                    @Override
                    protected void onSuccess(BindDeviceModel model) {
                        if (model.getResult().equals("200")) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.user_info_binding_success));
                            if (!MyApplication.isGuide)PreferencesUtil.isFristLogin(mContext,"isFirstGuide",true);
                            MyApplication.isGuide = PreferencesUtil.getFristLogin(mContext,"isFirstGuide");
                            setResult(111);
                            mContext.finish();
                        } else {
                            if (!MyApplication.isGuide && guideView!=null)guideView.show();
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    @Override
    public void onClick(View view) {
        if (view == mBack) {
            mContext.finish();
        }
        if (view == mRightLayout) {// 绑定
            bindDevice(deviceSerial);
        }
        if (view == mLayoutList.get(0)){//
            if (mInfoModel==null){
                return;
            }
            if (MyApplication.clientsModel.getClient_uuid().equals(mInfoModel.getData().getUuid())) return;

            Intent intent = new Intent(mContext, CallActivity.class);
            intent.putExtra(CallActivity.EXTRA_BUDDY_ID, 1);
            intent.putExtra("name",mInfoModel.getData().getName());
            intent.putExtra("number",mInfoModel.getData().getSipUsername());
            startActivity(intent);
        }
        if (view == mLayoutList.get(1)){//轨迹
            if (mInfoModel==null){
                return;
            }
            Intent intent = new Intent();
            if ("3".equals(PreferencesUtil.getStringData(mContext,"mapType"))){
                intent.setClass(mContext,MapGoogleActivity.class);
            }else{
                intent.setClass(mContext,MapActivity.class);
            }
            intent.putExtra("uuid",mInfoModel.getData().getUuid());
            intent.putExtra("sipUserName",mInfoModel.getData().getSipUsername());
            intent.putExtra("type",0);
            startActivity(intent);
        }
        if (view == mLayoutList.get(2)){//事件
            if (mInfoModel==null){
                return;
            }
            Intent intent = new Intent(mContext,DeviceEventActivity.class);
            intent.putExtra("uuid",mInfoModel.getData().getUuid());
            startActivity(intent);
        }
        if (view == mLayoutList.get(3)){//
            if (mInfoModel==null){
                return;
            }
            if (Build.VERSION.SDK_INT > Build.VERSION_CODES.KITKAT) {
                Intent intent = new Intent(mContext,SystemSetActivity.class);
                intent.putExtra("serial_number",mInfoModel.getData().getSerialNumber());
                startActivity(intent);
            } else {
                ToastUtil.showBottomShort(getResources().getString(R.string.device_set_no_support));
            }
        }

    }

    private void setGuideView() {
        TextView tv = new TextView(mContext);
        tv.setText(getResources().getString(R.string.guide_second_hint));
        tv.setTextColor(getResources().getColor(R.color.white));
        tv.setTextSize(18);
        tv.setGravity(Gravity.CENTER);

        guideView = GuideView.Builder
                .newInstance(mContext)
                .setTargetView(mRightLayout)//设置目标
                .setCustomGuideView(tv)
                .setDirction(GuideView.Direction.BOTTOM)
                .setShape(GuideView.MyShape.CIRCULAR)   // 设置圆形显示区域，
                .setBgColor(getResources().getColor(R.color.shadow))
                .setOnclickListener(new GuideView.OnClickCallback() {
                    @Override
                    public void onClickedGuideView() {
                        mRightLayout.callOnClick();
                        guideView.hide();
                    }
                })
                .build();
    }
}
