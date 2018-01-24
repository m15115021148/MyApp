package com.meigsmart.meigapp.sip;

import android.app.AlertDialog;
import android.app.DownloadManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.support.v7.app.NotificationCompat;
import android.view.View;
import android.widget.Toast;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.activity.HomePagerActivity;
import com.meigsmart.meigapp.activity.MessageChatActivity;
import com.meigsmart.meigapp.activity.SystemMessageActivity;
import com.meigsmart.meigapp.model.PushModel;

import java.io.File;

/**
 * Created by chenmeng on 2017/9/24.
 */
public class NotifyUtil {
    public static void startIntent(Context con, Class<?> cls) {
        Intent intent = new Intent(con, cls);
        con.startActivity(intent);
    }

    public static void startIntent(Context con, Intent intent) {
        con.startActivity(intent);
    }

    public static void deleteFilesByDirectory(File directory) {
        if (directory != null && directory.exists() && directory.isDirectory()) {
            for (File item : directory.listFiles()) {
                item.delete();
            }
        }
    }

    public static void alertYesCanel(View layout, DialogInterface.OnClickListener onConfirmClickListener) {
        AlertDialog ad = new AlertDialog.Builder(layout.getContext()).setView(layout)
                .setPositiveButton(layout.getContext().getString(android.R.string.ok), onConfirmClickListener)
                .setNegativeButton(layout.getContext().getString(android.R.string.cancel), null).show();
//        ad.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
//        ad.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
    }

    public static void alertConfirm(Context con, String msg) {
        AlertDialog ad = new AlertDialog.Builder(con).setMessage(msg).setPositiveButton(android.R.string.ok, null).show();
    }

    public static void toast(Context context, int resourceId) {
        Toast.makeText(context, resourceId, Toast.LENGTH_LONG).show();
    }

//    public static String getRegStatus(Context con,pjsip_status_code regStatus) {
//        String text = "";
//        if (regStatus == null){
//            text=con.getString(R.string.registering);
//        }else if (regStatus == pjsip_status_code.PJSIP_SC_OK){
//            text = con.getString(R.string.registered);
//        }else  if (regStatus == pjsip_status_code.PJSIP_SC_REQUEST_TIMEOUT){
//            text = con.getString(R.string.request_timeout);
//        }
//        return text;
//    }

    /**
     * 检测网络是否可用
     *
     * @return
     */
    public static boolean isNetworkConnected(Context con) {
        ConnectivityManager cm = (ConnectivityManager) con.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo ni = cm.getActiveNetworkInfo();
        return ni != null && ni.isConnectedOrConnecting();
    }

    public static void setNeverSleepPolicy(Context context) {
        ContentResolver cr = context.getContentResolver();
        int set = android.provider.Settings.System.WIFI_SLEEP_POLICY_NEVER;
        android.provider.Settings.System.putInt(cr, android.provider.Settings.System.WIFI_SLEEP_POLICY, set);
    }

    public static boolean isWifiConnected(Context context) {
        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo wifiNetworkInfo = connectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
        if (wifiNetworkInfo.isConnected()) {
            return true;
        }

        return false;
    }

    public static String getVer(Context context) {
        String ver = "";
        PackageManager pm = context.getPackageManager();
        try {
            PackageInfo pi = pm.getPackageInfo(context.getPackageName(), 0);
            ver = "" + pi.versionCode;
        } catch (PackageManager.NameNotFoundException e) {
            e.printStackTrace();
        }
        return ver;
    }

    public static void installAPK(Context context, String sId) {
        Long id = Long.parseLong(sId);
        DownloadManager dManager = (DownloadManager) context.getSystemService(Context.DOWNLOAD_SERVICE);
        Intent install = new Intent(Intent.ACTION_VIEW);
        Uri downloadFileUri = dManager.getUriForDownloadedFile(id);
        install.setDataAndType(downloadFileUri, "application/vnd.android.package-archive");
        install.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        context.startActivity(install);
    }

    public static void resultActivityBackApp(Context context,PushModel model,int flag,String ticker,String title) {

        NotificationManager nm = (NotificationManager)context.getSystemService(Context.NOTIFICATION_SERVICE);

        NotificationCompat.Builder mBuild = new NotificationCompat.Builder(context);

        mBuild.setTicker(ticker);
        mBuild.setSmallIcon(R.mipmap.app_logo1);
        mBuild.setContentTitle(title);//通知标题2
        mBuild.setContentText(model.getMeigmsg().getFrom_name());
        mBuild.setDefaults(Notification.DEFAULT_SOUND|Notification.DEFAULT_VIBRATE);

        //设置点击一次后消失（如果没有点击事件，则该方法无效。）
        mBuild.setAutoCancel(true);

        // 设置启动的程序，如果存在则找出，否则新的启动
        Intent in = new Intent(Intent.ACTION_MAIN);
        in.setAction("com.app2.activity");
        in.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);

        Intent intent = new Intent();
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP );

        if (model.getMessageType().equals(MessageType.TEXT_MESSAGE.getName()) || model.getMessageType().equals(MessageType.VOICE_MESSAGE.getName())){
            intent.setComponent(new ComponentName(context, MessageChatActivity.class));//用ComponentName得到class对象
            intent.putExtra("type",0);
            intent.putExtra("groupId",model.getMeigmsg().getGroup_uuid());
            intent.putExtra("groupName",model.getMeigmsg().getFrom_name());
        } else {
            intent.setComponent(new ComponentName(context, SystemMessageActivity.class));//用ComponentName得到class对象
        }

        PendingIntent contentIntent = PendingIntent.getActivity(context, 0, intent, PendingIntent.FLAG_CANCEL_CURRENT);

        mBuild.setContentIntent(contentIntent);

        nm.notify(flag, mBuild.build());
    }

    private static Intent[] makeIntentStack(Context context,PushModel model) {
        Intent[] intents = new Intent[2];
        intents[0] = Intent.makeRestartActivityTask(new ComponentName(context, HomePagerActivity.class));
        intents[1] = new Intent(context, MessageChatActivity.class);
        intents[1].putExtra("type",0);
        intents[1].putExtra("groupId",model.getMeigmsg().getGroup_uuid());
        intents[1].putExtra("groupName",model.getMeigmsg().getFrom_name());
        return intents;
    }
}
