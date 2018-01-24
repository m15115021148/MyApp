package com.meigsmart.meigapp.activity;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.telephony.TelephonyManager;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.ClientsModel;
import com.meigsmart.meigapp.util.PreferencesUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.tbruyelle.rxpermissions2.RxPermissions;

import java.net.ConnectException;
import java.util.HashMap;
import java.util.Map;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.functions.Consumer;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 登录页面
 *
 * @author chenemng created by 2017/9/5
 */
public class LoginActivity extends BaseActivity implements View.OnClickListener{
    private LoginActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.more)
    public LinearLayout mRegister;//注册按钮
    @BindView(R.id.rightName)
    public TextView mRightName;//内容
    @BindView(R.id.login)
    public TextView mLogin;//登录
    @BindView(R.id.userName)
    public EditText mUerName;//用户名
    @BindView(R.id.psw)
    public EditText mPsw;//密码

    @Override
    protected int getLayoutId() {
        return R.layout.activity_login;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setVisibility(View.GONE);
        mTitle.setText(getResources().getString(R.string.login_title));
        mRegister.setVisibility(View.GONE);
        mRegister.setOnClickListener(this);
        mRightName.setText(getResources().getString(R.string.login_register));
        mLogin.setOnClickListener(this);
        mLogin.setSelected(true);
        getPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.ACCESS_FINE_LOCATION,Manifest.permission.READ_PHONE_STATE,Manifest.permission.RECORD_AUDIO);
        if (PreferencesUtil.getFristLogin(mContext,"isFirst")){
            MyApplication.clientsModel = PreferencesUtil.getDataModel(mContext,"clientsModel");
            MyApplication.userName = PreferencesUtil.getStringData(mContext,"userName");
            MyApplication.diverId = getDeviceID(mContext);
            Intent intent = new Intent(mContext,HomePagerActivity.class);
            startActivity(intent);
            finish();
            return;
        }
    }

    @Override
    public void onDestroy(){
        super.onDestroy();
    }

    @Override
    public void onClick(View view) {
        if (view == mRegister) {

        }
        if (view == mLogin){

            if (MyApplication.diverId.equals("000000000000000")){
                ToastUtil.showBottomShort(getResources().getString(R.string.login_phone_limits));
                return;
            }

            if (TextUtils.isEmpty(mUerName.getText().toString().trim())){
                ToastUtil.showBottomShort(getResources().getString(R.string.login_name_limits));
                return;
            }

            login(mUerName.getText().toString());

        }
    }

    /**
     * 登录
     */
    private void login(String userName) {
        Map<String,Object> map = new HashMap<>();
        map.put("token",getDeviceID(mContext));
        map.put("lang", RequestCode.USER_LANG);
        map.put("bundle_id",RequestCode.USER_BUNDLE_ID);
        map.put("name",userName);
        map.put("versionCode",MyApplication.getInstance().getVersionCode());
        map.put("versionName",MyApplication.getInstance().getVersionName());
        map.put("appKey",RequestCode.APP_KEY);
        map.put("appSecret",RequestCode.APP_SECRET);

        HttpManager.getApiService().registerApp(HttpManager.getParameter(JSON.toJSONString(map)))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(mContext,getResources().getString(R.string.loading_title)).<ClientsModel>transformer())
                .subscribe(new BaseObserver<ClientsModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        }else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                    }

                    @Override
                    protected void onSuccess(ClientsModel model) {
                        if (model.getResult().equals("200")){
                            PreferencesUtil.isFristLogin(mContext,"isFirst",true);
                            PreferencesUtil.setDataModel(mContext,"clientsModel",model);
                            PreferencesUtil.setStringData(mContext,"userName",mUerName.getText().toString());
                            MyApplication.clientsModel = PreferencesUtil.getDataModel(mContext,"clientsModel");
                            MyApplication.userName = PreferencesUtil.getStringData(mContext,"userName");
                            Intent intent = new Intent(mContext,HomePagerActivity.class);
                            startActivity(intent);
                            finish();
                        }else{
                            ToastUtil.showBottomShort(getResources().getString(R.string.login_fail));
                        }
                    }
                });
    }

    /**
     * 获取手机设备id
     */
    @SuppressLint("MissingPermission")
    public static String getDeviceID(Activity activity){
        TelephonyManager tm = (TelephonyManager)activity.getSystemService(Context.TELEPHONY_SERVICE);
        return tm.getDeviceId();
    }

    /**
     * 获取权限
     * @param context
     * @param str
     */
    public void getPermission(Activity context, String...str){
        RxPermissions permissions = new RxPermissions(context);
        permissions.request(str)
                .subscribe(new Consumer<Boolean>() {
                    @Override
                    public void accept(Boolean aBoolean) throws Exception {
                        if (aBoolean){//所以权限都同意 才为true
                            MyApplication.diverId = getDeviceID(mContext);
                        }else{

                        }
                    }
                });
    }
}
