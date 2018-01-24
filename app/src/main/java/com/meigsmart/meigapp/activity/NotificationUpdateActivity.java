package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.ProgressBar;
import android.widget.TextView;


import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import butterknife.BindView;
import butterknife.ButterKnife;


/**
 * @author chenmeng 更新类 create at 2017年11月2日 下午4:49:01
 */
public class NotificationUpdateActivity extends Activity implements OnClickListener {
    @BindView(R.id.cancel)
    public TextView btn_cancel;// 关闭按钮
    @BindView(R.id.currentPos)
    public TextView tv_progress;// 正在下载
    @BindView(R.id.download_version)
    public TextView download_version;// 版本号
    @BindView(R.id.download_size)
    public TextView download_size;// apk大小
    @BindView(R.id.progressbar1)
    public ProgressBar mProgressBar;// 进度条
    private boolean isDestroy = true;// 是否销毁
    // 获取到下载url后，直接复制给MapApp,里面的全局变量
    private MyApplication app;// application对象
    private DownloadService.DownloadBinder binder;// 下载服务
    private boolean isBinded;// 是否开启

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.update);
        ButterKnife.bind(this);
        initData();
    }

    /**
     * 初始化数据
     */
    protected void initData() {
        btn_cancel.setOnClickListener(this);
        app = (MyApplication) getApplication();
        download_version.setText("V "+MyApplication.versionName);
    }

    @Override
    public void onResume() {
        super.onResume();
        if (isDestroy && app.isDownload()) {
            Intent it = new Intent(NotificationUpdateActivity.this,
                    DownloadService.class);
            startService(it);
            bindService(it, conn, Context.BIND_AUTO_CREATE);
        }
    }

    /**
     * 服务链接
     */
    ServiceConnection conn = new ServiceConnection() {

        @Override
        public void onServiceDisconnected(ComponentName name) {
            isBinded = false;
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            binder = (DownloadService.DownloadBinder) service;
            LogUtil.w("result", "服务启动!!!");
            // 开始下载
            isBinded = true;
            binder.addCallback(callback);
            binder.start();

        }
    };

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (isDestroy && app.isDownload()) {
            LogUtil.i("result", "移动了");
            Intent it = new Intent(NotificationUpdateActivity.this, DownloadService.class);
            startService(it);
            bindService(it, conn, Context.BIND_AUTO_CREATE);
        }

    }

    @Override
    protected void onStop() {
        super.onStop();
        isDestroy = false;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isBinded) {
            unbindService(conn);
        }
        if (binder != null && binder.isCanceled()) {
            LogUtil.w("result", " onDestroy  stopservice");
            Intent it = new Intent(this, DownloadService.class);
            stopService(it);
        }
    }

    /**
     * 返回更新进度条
     */
    private ICallbackResult callback = new ICallbackResult() {

        @Override
        public void OnBackResult(Object result) {
            if ("finish".equals(result)) {
                LogUtil.i("result", "下载完成了！");
                finish();
                return;
            }
            if ("error".equals(result)) {
                LogUtil.i("result", "下载异常！");
                mHandler.sendEmptyMessage(102);
                finish();
                return;
            }
            int i = (Integer) result;
            mProgressBar.setProgress(i);
            Message msg = mHandler.obtainMessage();
            msg.what = 101;
            msg.obj = i;
            mHandler.sendMessage(msg);
        }

    };

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what){
                case 101:
                    tv_progress.setText(getResources().getString(R.string.about_notify_progress) + msg.obj + "%");
                    java.text.DecimalFormat df = new java.text.DecimalFormat("#.00");
                    download_size.setText(df.format(MyApplication.length / 1024) + "MB");
                    break;
                case 102:
                    ToastUtil.showBottomShort(getResources().getString(R.string.about_down_error));
                    break;
                default:
                    break;
            }
        }
    };

    @Override
    public void onClick(View v) {
        if (v == btn_cancel) {
            finish();
        }
    }

    public interface ICallbackResult {
        void OnBackResult(Object result);
    }

    /**
     * 返回键
     */
    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK
                && event.getAction() == KeyEvent.ACTION_DOWN) {
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }
}
