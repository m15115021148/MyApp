package com.meigsmart.meigapp.activity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.Environment;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.support.v4.app.NotificationCompat;
import android.support.v4.content.FileProvider;
import android.webkit.MimeTypeMap;
import android.widget.RemoteViews;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.log.LogUtil;

public class DownloadService extends Service {
    private static final int NOTIFY_ID = 100;//0
    private int progress;//进度条
    private NotificationManager mNotificationManager;//状态栏
    private boolean canceled;//取消
    // 返回的安装包url
    private String apkUrl = MyApplication.downUrl;//下载地址
    /* 下载包安装路径 */
    private static final String savePath = Environment.getExternalStorageDirectory().toString() + "/MeiGLinkApp/updateApk/";

    private static final String saveFileName = savePath + "MeiGLink.apk";//apk名字
    private NotificationUpdateActivity.ICallbackResult callback;//返回
    private DownloadBinder binder;//下载服务
    private MyApplication app;//application对象
    private boolean serviceIsDestroy = false;//服务是否停止

    private Context mContext = this;//本类

    private NotificationCompat.Builder mBuild;//通知栏

    private Handler mHandler = new Handler() {

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case 0:// 下载完毕// 取消通知
                    app.setDownload(false);
                    mNotificationManager.cancel(NOTIFY_ID);
                    installApk();
                    break;
                case 2:
                    app.setDownload(false);
                    mNotificationManager.cancel(NOTIFY_ID);
                    break;
                case 1:
                    int rate = msg.arg1;
                    app.setDownload(true);
                    if (rate < 100) {
                        RemoteViews contentview = mBuild.build().contentView;
                        contentview.setTextViewText(R.id.tv_progress, rate + "%");
                        contentview.setProgressBar(R.id.progressbar, 100, rate,
                                false);
                    } else {
                        System.out.println("下载完毕!!!!!!!!!!!");
                        Notification notification = mBuild.build();
                        // 下载完毕后变换通知形式
                        notification.flags = Notification.FLAG_AUTO_CANCEL;
                        mBuild.setContent(null);

                        Intent intent = new Intent(mContext,
                                NotificationUpdateActivity.class);
                        // 告知已完成
                        intent.putExtra("completed", "yes");
                        // 更新参数,注意flags要使用FLAG_UPDATE_CURRENT
                        PendingIntent contentIntent = PendingIntent.getActivity(
                                mContext, 0, intent,
                                PendingIntent.FLAG_UPDATE_CURRENT);
                        mBuild.setContentIntent(contentIntent);

                        serviceIsDestroy = true;
                        stopSelf();// 停掉服务自身

                    }
                    mNotificationManager.notify(NOTIFY_ID, mBuild.build());
                    break;
            }
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        LogUtil.v("result", "downloadservice ondestroy");
        // 假如被销毁了，无论如何都默认取消了。
        app.setDownload(false);
    }

    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }

    @Override
    public void onRebind(Intent intent) {
        super.onRebind(intent);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        binder = new DownloadBinder();
        mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        app = (MyApplication) getApplication();
    }

    public class DownloadBinder extends Binder {

        public void start() {
            if (downLoadThread == null || !downLoadThread.isAlive()) {

                progress = 0;
                setUpNotification();
                new Thread() {
                    public void run() {
                        startDownload();
                    }
                }.start();
            }
        }

        /**
         * 取消
         */
        public void cancel() {
            canceled = true;
        }

        /**
         * 进度条
         */
        public int getProgress() {
            return progress;
        }

        /**
         * 是否取消
         */
        public boolean isCanceled() {
            return canceled;
        }

        /**
         * 服务是否销毁
         */
        public boolean serviceIsDestroy() {
            return serviceIsDestroy;
        }

        /**
         * 取消
         */
        public void cancelNotification() {
            mHandler.sendEmptyMessage(2);
        }

        /**
         * 添加返回
         */
        public void addCallback(NotificationUpdateActivity.ICallbackResult callback) {
            DownloadService.this.callback = callback;
        }
    }

    /**
     * 开始下载
     */
    private void startDownload() {
        LogUtil.v("result", "startDownload...");
        canceled = false;
        downloadApk();
    }

    /**
     * 创建通知
     */
    private void setUpNotification() {
        RemoteViews contentView = new RemoteViews(getPackageName(), R.layout.download_notification_layout);
        contentView.setTextViewText(R.id.name, mContext.getResources().getString(R.string.about_server_title));

        Intent intent = new Intent(this, NotificationUpdateActivity.class);

        PendingIntent contentIntent = PendingIntent.getActivity(this, 0,
                intent, PendingIntent.FLAG_UPDATE_CURRENT);

        mBuild = new NotificationCompat.Builder(this);
        mBuild.setSmallIcon(R.mipmap.app_logo1);
        mBuild.setWhen(System.currentTimeMillis());
        mBuild.setTicker(mContext.getResources().getString(R.string.about_server_ticker));
        mBuild.setOngoing(true);
        mBuild.setContent(contentView);
        mBuild.setContentIntent(contentIntent);

        mNotificationManager.notify(NOTIFY_ID, mBuild.build());
    }

    //
    /**
     * 下载apk
     *
     * @param url
     */
    private Thread downLoadThread;

    private void downloadApk() {
        downLoadThread = new Thread(mdownApkRunnable);
        downLoadThread.start();
    }

    /**
     * 安装apk
     **/
    private void installApk() {
        File apkfile = new File(saveFileName);
        if (!apkfile.exists()) {
            return;
        }else{
            openFile(apkfile,mContext);
        }
//        Intent i = new Intent(Intent.ACTION_VIEW);
//        i.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
//        i.setDataAndType(Uri.parse("file://" + apkfile.toString()), "application/vnd.android.package-archive");
//        mContext.startActivity(i);
        callback.OnBackResult("finish");
    }

    public void openFile(File file, Context context) {
        Intent var2 = new Intent();
        var2.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        var2.setAction(Intent.ACTION_VIEW);
        if(Build.VERSION.SDK_INT>=Build.VERSION_CODES.N){
            Uri uriForFile = FileProvider.getUriForFile(context, context.getApplicationContext().getPackageName() + ".provider", file);
            var2.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            var2.setDataAndType(uriForFile, context.getContentResolver().getType(uriForFile));
        }else{
            var2.setDataAndType(Uri.fromFile(file), getMIMEType(file));
        }
        try {
            context.startActivity(var2);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public String getMIMEType(File var0) {
        String var1 = "";
        String var2 = var0.getName();
        String var3 = var2.substring(var2.lastIndexOf(".") + 1, var2.length()).toLowerCase();
        var1 = MimeTypeMap.getSingleton().getMimeTypeFromExtension(var3);
        return var1;
    }

    private int lastRate = 0;//进度值
    private Runnable mdownApkRunnable = new Runnable() {
        @Override
        public void run() {
            try {
                LogUtil.w("result", "apkUrl:" + apkUrl);
                URL url = new URL(apkUrl);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.connect();
                int length = conn.getContentLength();

                InputStream is = conn.getInputStream();

                File file = new File(savePath);
                if (!file.exists()) {
                    file.mkdirs();
                }
                String apkFile = saveFileName;
                File ApkFile = new File(apkFile);
                FileOutputStream fos = new FileOutputStream(ApkFile);

                int count = 0;
                byte buf[] = new byte[1024];
                MyApplication.length = length / 1000;
                java.text.DecimalFormat df = new java.text.DecimalFormat("#.00");
                df.format(MyApplication.length);
                do {
                    int numread = is.read(buf);
                    count += numread;
                    progress = (int) (((float) count / length) * 100);
                    // 更新进度
                    Message msg = mHandler.obtainMessage();
                    msg.what = 1;
                    msg.arg1 = progress;
                    if (progress >= lastRate + 1) {
                        mHandler.sendMessage(msg);
                        lastRate = progress;
                        if (callback != null)
                            callback.OnBackResult(progress);
                    }
                    if (numread <= 0) {
                        // 下载完成通知安装
                        mHandler.sendEmptyMessage(0);
                        // 下载完了，cancelled也要设置
                        canceled = true;
                        break;
                    }
                    fos.write(buf, 0, numread);
                } while (!canceled);// 点击取消就停止下载.

                fos.close();
                is.close();
            } catch (MalformedURLException e) {
                e.printStackTrace();
                canceled = true;
                app.setDownload(false);
                mNotificationManager.cancel(NOTIFY_ID);
                callback.OnBackResult("error");
            } catch (IOException e) {
                e.printStackTrace();
                canceled = true;
                app.setDownload(false);
                mNotificationManager.cancel(NOTIFY_ID);
                callback.OnBackResult("error");
            }

        }
    };

}
