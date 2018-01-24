package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.app.NotificationManager;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.support.design.widget.TabLayout;
import android.support.v4.view.ViewPager;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.View;
import android.widget.LinearLayout;

import com.alibaba.fastjson.JSON;
import com.baidu.location.BDLocation;
import com.baidu.location.BDLocationListener;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.MenuFragmentAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.config.WebHostConfig;
import com.meigsmart.meigapp.fragment.BaseFragment;
import com.meigsmart.meigapp.fragment.LinkManFragment;
import com.meigsmart.meigapp.fragment.MessageFragment;
import com.meigsmart.meigapp.fragment.SettingFragment;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.http.upload.UpLoadHelper;
import com.meigsmart.meigapp.http.upload.UpLoadListener;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.ClientsModel;
import com.meigsmart.meigapp.model.PushModel;
import com.meigsmart.meigapp.sip.CallActivity;
import com.meigsmart.meigapp.sip.NotifyUtil;
import com.meigsmart.meigapp.util.FileUtil;
import com.meigsmart.meigapp.util.MapLocationUtil;
import com.meigsmart.meigapp.util.PreferencesUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.siplibs.BroadcastEventReceiver;
import com.meigsmart.siplibs.Logger;
import com.meigsmart.siplibs.SipAccountData;
import com.meigsmart.siplibs.SipServiceCommand;

import org.pjsip.pjsua2.pjsip_inv_state;
import org.pjsip.pjsua2.pjsip_status_code;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import butterknife.BindArray;
import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;

/**
 * HomePagerActivity
 *
 * @author chenmeng created by 2017/9/7
 * @see HomePagerActivity
 */
public class HomePagerActivity extends BaseActivity implements View.OnClickListener, TabLayout.OnTabSelectedListener {
    private HomePagerActivity mContext;//本类
    @BindView(R.id.msgLayout)
    public LinearLayout mMsgLayout;//message
    @BindView(R.id.tabLayout)
    public TabLayout mTabLayout;//导航布局
    @BindView(R.id.viewPager)
    public ViewPager mViewPager;//viewPager
    private MenuFragmentAdapter fragmentAdapter;//适配器
    private List<BaseFragment> mFragmentList = new ArrayList<>();//fragment集合
    @BindArray(R.array.MenuData)
    String[] data;//获取数据
    @BindView(R.id.errorLayout)
    public LinearLayout mError;//error
    private NotificationManager mNotify;//通知
    private long exitTime = 0;//退出的时间
    private MapLocationUtil mapLocationUtil;//百度定位

    @Override
    protected int getLayoutId() {
        return R.layout.activity_group_manager;
    }

    @Override
    protected void initData() {
        mContext = this;
        mMsgLayout.setOnClickListener(this);
        mError.setOnClickListener(this);
        MyApplication.isGuide = PreferencesUtil.getFristLogin(mContext,"isFirstGuide");
        initTabLayout();

        mNotify = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        if (!TextUtils.isEmpty(MyApplication.userName)) loginNoDialog(MyApplication.userName);
        locationPoint();
    }

    /**
     * 初始化tablayout数据
     */
    private void initTabLayout() {
        mFragmentList.add(new MessageFragment());
        mFragmentList.add(new LinkManFragment());
        mFragmentList.add(new SettingFragment());

        mTabLayout.removeAllTabs();
        mTabLayout.setTabMode(TabLayout.MODE_FIXED);//设置tab模式，当前为系统默认模式
        mViewPager.setOffscreenPageLimit(1);
        fragmentAdapter = new MenuFragmentAdapter(getSupportFragmentManager(), data, mFragmentList, mContext);
        mViewPager.setAdapter(fragmentAdapter);
        //绑定ViewPager
        mTabLayout.setupWithViewPager(mViewPager);
        mViewPager.setCurrentItem(1);
        for (int i = 0; i < mTabLayout.getTabCount(); i++) {
            TabLayout.Tab tab = mTabLayout.getTabAt(i);
            if (tab != null) {
                View v = fragmentAdapter.getView(i);
                if (i == 0) {//默认第一个选中
                    v.setSelected(true);
                }
                tab.setCustomView(v);
            }
        }
        mTabLayout.addOnTabSelectedListener(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        sipEvents.register(this);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        sipEvents.unregister(this);
        mapLocationUtil.unregisterListener(mBDListener); //注销掉监听
        mapLocationUtil.stop(); //停止定位服务
        mNotify.cancelAll();
        SipServiceCommand.stop(mContext);
    }

    /**
     * 开启定位
     */
    private void locationPoint() {
        // -----------location config ------------
        mapLocationUtil = ((MyApplication) getApplication()).mapLocationUtil;
        mapLocationUtil.registerListener(mBDListener);
        //注册监听
        int type = getIntent().getIntExtra("from", 0);
        if (type == 0) {
            mapLocationUtil.setLocationOption(mapLocationUtil.getDefaultLocationClientOption());
        } else if (type == 1) {
            mapLocationUtil.setLocationOption(mapLocationUtil.getOption());
        }
        mapLocationUtil.start();// 定位SDK
    }

    private BroadcastEventReceiver sipEvents = new BroadcastEventReceiver() {

        @Override
        public void onRegistration(String accountID, pjsip_status_code registrationStateCode) {
            if (MyApplication.getInstance().callBackSip!=null)MyApplication.getInstance().callBackSip.onRegistration(accountID,registrationStateCode);
            if (registrationStateCode == pjsip_status_code.PJSIP_SC_OK) {
                LogUtil.e("result","Registered");
                mHandler.sendEmptyMessage(RequestCode.REGISTER_STATUS_OK);
            } else {
                LogUtil.e("result","Unregistered");
                mHandler.sendEmptyMessage(RequestCode.REGISTER_STATUS_FAILED);
            }
        }

        @Override
        public void onCallState(String accountID, int callID, pjsip_inv_state callStateCode, long connectTimestamp, boolean isLocalHold, boolean isLocalMute) {
            if (MyApplication.getInstance().callBackSip!=null)MyApplication.getInstance().callBackSip.onCallState(accountID, callID, callStateCode, connectTimestamp, isLocalHold, isLocalMute);
        }

        @Override
        public void onIncomingCall(String accountID, int callID, String displayName, String remoteUri) {
            LogUtil.e("result","onIncomingCall");
            LogUtil.e("result","accountID     :" +accountID);
            LogUtil.e("result","callID        :"+callID);
            LogUtil.e("result","displayName   :"+displayName);
            LogUtil.e("result","remoteUri     :"+remoteUri);
            Intent intent = new Intent(mContext, CallActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.putExtra("accountID",accountID);
            intent.putExtra("remoteUri",remoteUri);
            intent.putExtra("callID",callID);
            startActivity(intent);
        }

        @Override
        public void onOutgoingCall(String accountID, int callID, String number) {
            if (MyApplication.getInstance().callBackSip!=null)MyApplication.getInstance().callBackSip.onOutgoingCall(accountID, callID, number);
        }

        @Override
        public void onMessageNotify(String accountID, String fromUri, String msg) {
            LogUtil.w("result","onMessageNotify\n"+msg);
            if (MyApplication.getInstance().callBackSip!=null)MyApplication.getInstance().callBackSip.onMessageNotify(accountID, fromUri, msg);
            try{
                PushModel model = JSON.parseObject(msg,PushModel.class);
                LogUtil.v("result","model:"+JSON.toJSONString(model));
                if (MyApplication.isPush)
                    NotifyUtil.resultActivityBackApp(mContext,model,2,
                            mContext.getResources().getString(R.string.notify_ticker),
                            mContext.getResources().getString(R.string.notify_title)
                    );
            }catch (Exception e){
                LogUtil.e("result","push json format error!");
                e.printStackTrace();
            }
        }

        @Override
        public void onNetworkChange() {
            SipServiceCommand.netWorkChange(mContext);
        }
    };

    /**
     * 初始化sip
     */
    private void initSipData() {
        Logger.setLogLevel(Logger.LogLevel.OFF);

        SipAccountData mSipAccount= new SipAccountData();

        mSipAccount.setDomain(MyApplication.clientsModel.getSip().getDomain())
                .setPort(6080)
                .setTcpTransport(true)
                .setUsername(MyApplication.clientsModel.getSip().getUsername())
                .setPassword(MyApplication.clientsModel.getSip().getPassword());
        LogUtil.e("result",JSON.toJSONString(mSipAccount));

        MyApplication.sipAccount = mSipAccount;

        SipServiceCommand.setAccount(this, mSipAccount);
    }

    private void sendAppLog() {
        File file = FileUtil.createRootDirectory(RequestCode.UPLOAD_SAVE_FILE_NAME);
        final File log = new File(file.getPath(), RequestCode.UPLOAD_APP_LOG);
        if (log.exists() && !log.isDirectory()) {
            String url = WebHostConfig.getHostName() + "uploadlog.json";

            Map<String, Object> map = new HashMap<>();
            map.put("uuid", MyApplication.clientsModel == null ? "" : MyApplication.clientsModel.getClient_uuid());
            map.put("serial_number", MyApplication.userName);
            map.put("type", "1");//0 设备  1 app
            map.put("file", log);

            UpLoadHelper helper = new UpLoadHelper(url, map);
            helper.upLoadFile(new UpLoadListener() {
                @Override
                public void upLoad(long bytesRead, long contentLength) {
                }

                @Override
                public void onSuccess(String result) {
                    FileUtil.deleteFilePath(log.getPath());
                }

                @Override
                public void onFailure(Throwable t) {
                    LogUtil.e("result", "onFailure............");
                }
            });
        } else {
            LogUtil.e("result", "come fail....");
        }
    }

    /**
     * 登录
     */
    private void loginNoDialog(String userName) {
        if (MyApplication.diverId.equals("000000000000000")) {
            return;
        }
        Map<String, Object> map = new HashMap<>();
        map.put("token", MyApplication.diverId);
        map.put("lang", RequestCode.USER_LANG);
        map.put("bundle_id", RequestCode.USER_BUNDLE_ID);
        map.put("name", userName);
        map.put("versionCode", MyApplication.getInstance().getVersionCode());
        map.put("versionName", MyApplication.getInstance().getVersionName());
        map.put("appKey",RequestCode.APP_KEY);
        map.put("appSecret",RequestCode.APP_SECRET);

        HttpManager.getApiService().registerApp(HttpManager.getParameter(JSON.toJSONString(map)))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<ClientsModel>() {
                    @Override
                    public void onError(Throwable e) {
                    }

                    @Override
                    protected void onSuccess(ClientsModel model) {
                        if (model.getResult().equals("200")) {
                            PreferencesUtil.setDataModel(mContext, "clientsModel", model);
                            MyApplication.clientsModel = PreferencesUtil.getDataModel(mContext, "clientsModel");

                            sendAppLog();
                            initSipData();
                        }
                    }
                });
    }

    @Override
    public void onClick(View view) {
        if (view == mMsgLayout) {
            Intent system = new Intent(mContext, SystemMessageActivity.class);
            startActivity(system);
        }
        if (view == mError) {
            mError.setVisibility(View.GONE);
            SipServiceCommand.netWorkChange(mContext);
        }
    }

    /**
     * 退出activity
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_DOWN) {
            if ((System.currentTimeMillis() - exitTime) > 2000) {
                ToastUtil.showBottomShort(getResources().getString(R.string.home_pager_exit_show));
                exitTime = System.currentTimeMillis();
            } else {
                //退出所有的activity
                Intent intent = new Intent();
                intent.setAction(BaseActivity.TAG_ESC_ACTIVITY);
                sendBroadcast(intent);
                finish();
            }
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    /**
     * 定位结果回调，重写onReceiveLocation方法，可以直接拷贝如下代码到自己工程中修改
     */
    private BDLocationListener mBDListener = new BDLocationListener() {

        @Override
        public void onReceiveLocation(BDLocation location) {
            if (null != location && location.getLocType() != BDLocation.TypeServerError) {
                MyApplication.lat = location.getLatitude();
                MyApplication.lng = location.getLongitude();
            }
        }
    };

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case RequestCode.REGISTER_STATUS_OK:
                    MyApplication.isSipRegister = true;
                    mError.setVisibility(View.GONE);
                    break;
                case RequestCode.REGISTER_STATUS_FAILED:
                    MyApplication.isSipRegister = false;
                    mError.setVisibility(View.VISIBLE);
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
    }

    @Override
    public void onTabSelected(TabLayout.Tab tab) {
        mViewPager.setCurrentItem(tab.getPosition());
    }

    @Override
    public void onTabUnselected(TabLayout.Tab tab) {
    }

    @Override
    public void onTabReselected(TabLayout.Tab tab) {
    }
}
