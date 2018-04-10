package com.meigsmart.meigapp.application;

import android.app.Application;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.TextView;

import com.baidu.mapapi.SDKInitializer;
import com.meigsmart.charlibs.db.BluetoothDao;
import com.meigsmart.charlibs.db.CharDataDao;
import com.meigsmart.charlibs.db.LastIdDao;
import com.meigsmart.meigapp.log.CrashHandler;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.ClientsModel;
import com.meigsmart.meigapp.sip.ObserverSip;
import com.meigsmart.meigapp.util.MapLocationUtil;
import com.meigsmart.meigapp.util.NetworkUtil;
import com.meigsmart.siplibs.SipAccountData;

import java.io.UnsupportedEncodingException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;


/**
 * 入口配置
 * Created by chenMeng on 2017/9/1.
 */
public class MyApplication extends Application {
    private static MyApplication instance;// application对象
    public static NetworkUtil netState;//网络状态
    public static int screenWidth = 0;//屏幕宽
    public static int screenHeight = 0;//屏幕高
    public MapLocationUtil mapLocationUtil;//百度定位
    public static double lat = 0;// 纬度
    public static double lng = 0;// 经度
    public static ClientsModel clientsModel;//实体类
    public static String diverId = "000000000000000";//设备id
    public static String userName = "";//用户名
    public CharDataDao mDb;//数据库
    public static boolean isPush = true;//是否有推送
    public static boolean isSipRegister = false;//sip是否注册
    public LastIdDao mLastDb;//数据库
    private boolean isDownload;// 是否在下载
    public static String downUrl = "";// 下载地址
    public static double length;// apk大小
    public static String versionName;// 版本号
    public static boolean isBlueConnect = false;//蓝牙是否连接
    public BluetoothDao mBlueDb;//数据库
    public static boolean isGuide;//第一有引导页面
    public ObserverSip callBackSip;
    public static SipAccountData sipAccount;

    @Override
    public void onCreate() {
        super.onCreate();
        netState = new NetworkUtil(getApplicationContext());
        SDKInitializer.initialize(getApplicationContext());
        getScreenSize();
        mDb = new CharDataDao(this);
        mLastDb = new LastIdDao(this);
        mBlueDb = new BluetoothDao(this);
        mapLocationUtil = new MapLocationUtil(getApplicationContext());
        CrashHandler.getInstance().init(this);
    }

    public static MyApplication getInstance(){
        return instance;
    }

    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        instance = this;
    }

    /**
     * 获取手机网络状态对象
     *
     * @return
     */
    public static NetworkUtil getNetObject() {
        if (netState != null) {
            return netState;
        } else {
            return new NetworkUtil(getInstance().getApplicationContext());
        }
    }

    /**
     * 获取屏幕尺寸
     */
    private void getScreenSize() {
        DisplayMetrics dm = getResources().getDisplayMetrics();
        screenHeight = dm.heightPixels;
        screenWidth = dm.widthPixels;
        LogUtil.w("result","height:"+screenHeight+" width:"+screenWidth);
    }

    /**
     * 描述：MD5加密.
     *(全大写字母)32
     * @param string
     *            要加密的字符串
     * @return String 加密的字符串
     */
    public static String md5(String string) {
        byte[] hash;
        try {
            hash = MessageDigest.getInstance("MD5").digest(string.getBytes("UTF-8"));
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException("Huh, MD5 should be supported?", e);
        } catch (UnsupportedEncodingException e) {
            throw new RuntimeException("Huh, UTF-8 should be supported?", e);
        }
        StringBuilder hex = new StringBuilder(hash.length * 2);
        for (byte b : hash) {
            if ((b & 0xFF) < 0x10) hex.append("0");
            hex.append(Integer.toHexString(b & 0xFF));
        }
        return hex.toString().toUpperCase();
    }

    /**
     * listview没有数据显示 的控件
     * @param context 本类
     * @param T AbsListView
     * @param txt 内容
     */
    public static View setEmptyShowText(Context context, AbsListView T, String txt){
        TextView emptyView = new TextView(context);
        emptyView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        emptyView.setText(txt);
        emptyView.setTextSize(18);
        emptyView.setTextColor(Color.parseColor("#808080"));
        emptyView.setGravity(Gravity.CENTER_HORIZONTAL| Gravity.CENTER_VERTICAL);
        emptyView.setVisibility(View.GONE);
        ((ViewGroup)T.getParent()).addView(emptyView);
        T.setEmptyView(emptyView);
        return emptyView;
    }

    /**
     * 获取版本号
     *
     * @return 当前应用的版本号
     */
    public String getVersionName() {
        try {
            PackageManager pm = getPackageManager();
            PackageInfo info = pm.getPackageInfo(getPackageName(), 0);
            String version = info.versionName;
            return version;
        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    /**
     * get App versionCode
     * @return
     */
    public String getVersionCode(){
        PackageManager packageManager=getApplicationContext().getPackageManager();
        PackageInfo packageInfo;
        String versionCode="";
        try {
            packageInfo=packageManager.getPackageInfo(getApplicationContext().getPackageName(),0);
            versionCode=packageInfo.versionCode+"";
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return versionCode;
    }

    public boolean isDownload() {
        return isDownload;
    }

    public void setDownload(boolean isDownload) {
        this.isDownload = isDownload;
    }

    public void setCallBackSip(ObserverSip callBackSip) {
        this.callBackSip = callBackSip;
    }
}
