package com.meigsmart.meigapp.activity;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.support.v4.app.FragmentActivity;
import android.view.Window;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.util.SystemManagerUtil;

import butterknife.ButterKnife;
import butterknife.Unbinder;


/**
 * 基本的activity 配置常量
 * Created by chenemng on 2017/9/1.
 */
public abstract class BaseActivity extends FragmentActivity {
    protected static final String TAG_ESC_ACTIVITY = "com.broader.esc";//内容描述 退出activity时 发送的广播信号
    private MyBroaderEsc receiver;//广播
    private Unbinder butterKnife;//取消绑定

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // 设置无标题
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(getLayoutId());
        SystemManagerUtil.setSystemManager(this,getResources().getColor(R.color.red_dan));
        if (getRequestedOrientation() != ActivityInfo.SCREEN_ORIENTATION_PORTRAIT) {//设置为竖屏
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        }
        // 注册广播
        receiver = new MyBroaderEsc();
        registerReceiver(receiver, new IntentFilter(TAG_ESC_ACTIVITY));
        // 反射注解机制初始化
        butterKnife = ButterKnife.bind(this);
        initData();
    }

    protected abstract int getLayoutId();//获取布局layout

    protected abstract void initData();//初始化数据


    /**
     * @发送广播 退出activity
     *
     */
    class MyBroaderEsc extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent != null) {
                butterKnife.unbind();
                finish();
            }
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
    }

    @Override
    protected void onResume() {
        super.onResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onStop() {
        super.onStop();
    }

    @Override
    protected void onRestart() {
        super.onRestart();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(receiver);
    }
}
