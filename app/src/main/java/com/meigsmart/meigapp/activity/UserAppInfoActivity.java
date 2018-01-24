package com.meigsmart.meigapp.activity;

import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.DeviceInfoModel;
import com.meigsmart.meigapp.model.MemberAddModel;
import com.meigsmart.meigapp.sip.CallActivity;
import com.meigsmart.meigapp.util.QRCodeUtil;
import com.meigsmart.meigapp.util.ToastUtil;


import java.net.ConnectException;
import java.util.HashMap;
import java.util.Map;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 用户信息页面
 * @author chenmeng created by 2017/9/6
 */
public class UserAppInfoActivity extends BaseActivity implements View.OnClickListener {
    private UserAppInfoActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.mainLayout)
    public LinearLayout mLayout;//整个布局
    @BindView(R.id.userName)
    public TextView mUserName;
    @BindView(R.id.phone)
    public TextView mPhone;
    @BindView(R.id.qr)
    public ImageView mQr;
    @BindView(R.id.add)
    public TextView mAdd;
    private int type = 0;//类别 0 user  1 other  2 add other
    private DeviceInfoModel mInfoModel = null;//信息结果
    private String clientUUid = "";
    private String groupUUid = "";

    @Override
    protected int getLayoutId() {
        return R.layout.activity_user_app_info;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.user_app_info_title));
        mAdd.setOnClickListener(this);
        mPhone.setOnClickListener(this);

        type = getIntent().getIntExtra("type",0);

        if (type == 0){
            if (MyApplication.clientsModel!=null)getUserInfoByUUid(MyApplication.clientsModel.getClient_uuid());
        } else if (type == 1){
            clientUUid = getIntent().getStringExtra("client_uuid");
            if (!TextUtils.isEmpty(clientUUid)) getUserInfoByUUid(clientUUid);
        } else if (type == 2){
            mAdd.setVisibility(View.VISIBLE);
            String msg = getIntent().getStringExtra("qr_client_uuid");
            groupUUid = getIntent().getStringExtra("group_uuid");
            if (!TextUtils.isEmpty(msg))clientUUid = msg.split(RequestCode.QR_CHARACTER)[2];
            getUserInfoByUUid(clientUUid);
        }

    }

    /**
     * 获取app信息 通过uuid
     */
    private void getUserInfoByUUid(String client_uuid) {
        HttpManager.getApiService().getDeviceInfoByUUid(client_uuid)
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
                        Bitmap logo = BitmapFactory.decodeResource(mContext.getResources(), R.drawable.app_user_info_logo);
                        String msg = "";
                        if (model.getResult().equals("200")) {
                            mInfoModel = model;
                            msg = RequestCode.QR_HEADER_ROOT + model.getData().getName() + RequestCode.QR_CHARACTER + model.getData().getUuid();
                            mUserName.setText(mInfoModel.getData().getName());
                            mPhone.setText(mInfoModel.getData().getSipUsername());
                        } else {
                            msg = model.getReason();
                            ToastUtil.showBottomShort(model.getReason());
                        }
                        mQr.setImageBitmap(QRCodeUtil.createQRImage(msg, null));
                    }
                });
    }

    /**
     * 添加成员
     */
    private void addMembers(String group_uuid, String body) {
        HttpManager.getApiService().addMember(group_uuid, HttpManager.getParameter(body))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(mContext, getResources().getString(R.string.loading_title)).<MemberAddModel>transformer())
                .subscribe(new BaseObserver<MemberAddModel>() {

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
                    protected void onSuccess(MemberAddModel model) {
                        if (model.getResult().equals("200")) {
                            Intent intent = new Intent();
                            intent.putExtra("name", mInfoModel.getData().getName());
                            intent.putExtra("client_uuid", model.getClient_uuid());
                            setResult(113, intent);
                            ToastUtil.showBottomShort(getResources().getString(R.string.user_info_add_success));
                            mContext.finish();
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    @Override
    public void onClick(View view) {
        if (view == mBack){
            mContext.finish();
        }
        if (view == mPhone){
            if (mInfoModel==null)return;
            if (MyApplication.clientsModel.getClient_uuid().equals(mInfoModel.getData().getUuid())) return;

            Intent intent = new Intent(mContext, CallActivity.class);
            intent.putExtra(CallActivity.EXTRA_BUDDY_ID, 1);
            intent.putExtra("name",mInfoModel.getData().getName());
            intent.putExtra("number",mInfoModel.getData().getSipUsername());
            startActivity(intent);
        }
        if (view == mAdd){
            if (mInfoModel==null)return;

            if (TextUtils.isEmpty(groupUUid) || TextUtils.isEmpty(clientUUid)) {
                ToastUtil.showBottomShort(getResources().getString(R.string.user_info_add_error));
            }

            Map<String, Object> map = new HashMap<>();
            map.put("client_uuid", clientUUid);
            addMembers(groupUUid, JSON.toJSONString(map));
        }
    }
}
