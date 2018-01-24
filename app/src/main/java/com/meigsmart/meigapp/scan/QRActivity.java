package com.meigsmart.meigapp.scan;

import java.io.IOException;
import java.util.Vector;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetFileDescriptor;
import android.graphics.Color;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.os.Bundle;
import android.os.Handler;
import android.os.Vibrator;
import android.support.v4.app.ActivityCompat;
import android.text.TextUtils;
import android.view.SurfaceHolder;
import android.view.SurfaceHolder.Callback;
import android.view.SurfaceView;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.google.zxing.BarcodeFormat;
import com.google.zxing.Result;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.activity.BaseActivity;
import com.meigsmart.meigapp.activity.QrOtherActivity;
import com.meigsmart.meigapp.activity.UserDeviceInfoActivity;
import com.meigsmart.meigapp.config.RequestCode;

import butterknife.BindView;

//扫描Activity
public class QRActivity extends BaseActivity implements Callback, View.OnClickListener {
    private CaptureActivityHandler handler;
    private boolean hasSurface;
    private Vector<BarcodeFormat> decodeFormats;
    private String characterSet;
    private InactivityTimer inactivityTimer;
    private MediaPlayer mediaPlayer;
    private boolean playBeep;
    private static final float BEEP_VOLUME = 0.10f;
    private boolean vibrate;
    private Context context;
    public static final String RESULT = "RESULT";
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.viewfinder_view)
    public ViewfinderView viewfinderView;
    private int type = 0;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_qr;
    }

    @Override
    protected void initData() {
        CameraManager.init(getApplication());
        context = QRActivity.this;
        //动态权限
        int permission = ActivityCompat.checkSelfPermission(this,
                Manifest.permission.CAMERA);
        if (permission != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA},
                    1);
        }
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.scan_title));
        mTitle.setTextColor(Color.parseColor("#ffffff"));
        hasSurface = false;
        type = getIntent().getIntExtra("type",0);
        inactivityTimer = new InactivityTimer(this);
    }

    //	退出扫描
    @Override
    public void onClick(View v) {
        this.finish();
    }

    @SuppressWarnings("deprecation")
    @Override
    protected void onResume() {
        super.onResume();
        SurfaceView surfaceView = (SurfaceView) findViewById(R.id.preview_view);
        SurfaceHolder surfaceHolder = surfaceView.getHolder();
        if (hasSurface) {
            initCamera(surfaceHolder);
        } else {
            surfaceHolder.addCallback(this);
        /* 该方法在4.0以后被废弃了，用到的时候，系统会自动设置；但在4.0之前得手动设置；
			表示SurfaceView本身不包含原生数据，其数据来源于其它对象如Camera预览，就是由Camera对象向SurfaceView提供数据。
         */
            surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
        }
        decodeFormats = null;
        characterSet = null;

        playBeep = true;
        AudioManager audioService = (AudioManager) getSystemService(AUDIO_SERVICE);
        if (audioService.getRingerMode() != AudioManager.RINGER_MODE_NORMAL) {
            playBeep = false;
        }
        initBeepSound();
        vibrate = true;

    }

    @Override
    protected void onPause() {
        super.onPause();
        if (handler != null) {
            handler.quitSynchronously();
            handler = null;
        }
        CameraManager.get().closeDriver();
    }

    @Override
    protected void onDestroy() {
        inactivityTimer.shutdown();
        super.onDestroy();
    }

    /**
     * 处理扫描结果
     *
     * @param result
     */
    public void handleDecode(Result result) {
        inactivityTimer.onActivity();
        playBeepSoundAndVibrate();
        String resultString = result.getText();
        onResultHandler(resultString);
    }

    /**
     * 跳转到上一个页面
     *
     * @param resultString
     */
    private void onResultHandler(String resultString) {
        if (TextUtils.isEmpty(resultString)) {
            Toast.makeText(context, "Scan failed!", Toast.LENGTH_SHORT).show();
            ((Activity) context).finish();
        } else if (type == 0){
            Intent resultIntent = new Intent();
            Bundle bundle = new Bundle();
            bundle.putString(RESULT, resultString);
            resultIntent.putExtras(bundle);
            ((Activity) context).setResult(RESULT_OK, resultIntent);
            ((Activity) context).finish();
        }else if (type == 1){
            Intent intent = new Intent();
            if (resultString.contains(RequestCode.QR_DEVICE_ROOT)){
                intent.setClass(context,UserDeviceInfoActivity.class);
                intent.putExtra("type",1);
                intent.putExtra("qr_serial_num",resultString);
            }else{
                intent.setClass(context,QrOtherActivity.class);
                intent.putExtra("result",resultString);
            }
            startActivityForResult(intent,111);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        setResult(111);
        ((Activity) context).finish();
    }

    private void initCamera(SurfaceHolder surfaceHolder) {
        try {
            CameraManager.get().openDriver(surfaceHolder);
        } catch (IOException ioe) {
            return;
        } catch (RuntimeException e) {
            return;
        } catch (Exception e) {
            return;
        }
        if (handler == null) {
            handler = new CaptureActivityHandler(this, decodeFormats,
                    characterSet);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,
                               int height) {

    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (!hasSurface) {
            hasSurface = true;
            initCamera(holder);
        }

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        hasSurface = false;

    }

    public ViewfinderView getViewfinderView() {
        return viewfinderView;
    }

    public Handler getHandler() {
        return handler;
    }

    public void drawViewfinder() {
        viewfinderView.drawViewfinder();

    }

    private void initBeepSound() {
        if (playBeep && mediaPlayer == null) {
            // The volume on STREAM_SYSTEM is not adjustable, and users found it
            // too loud,
            // so we now play on the music stream.
            setVolumeControlStream(AudioManager.STREAM_MUSIC);
            mediaPlayer = new MediaPlayer();
            mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mediaPlayer.setOnCompletionListener(beepListener);

            AssetFileDescriptor file = getResources().openRawResourceFd(
                    R.raw.beep);
            try {
                mediaPlayer.setDataSource(file.getFileDescriptor(),
                        file.getStartOffset(), file.getLength());
                file.close();
                mediaPlayer.setVolume(BEEP_VOLUME, BEEP_VOLUME);
                mediaPlayer.prepare();
            } catch (IOException e) {
                mediaPlayer = null;
            }
        }
    }

    private static final long VIBRATE_DURATION = 200L;

    private void playBeepSoundAndVibrate() {
        if (playBeep && mediaPlayer != null) {
            mediaPlayer.start();
        }
        if (vibrate) {
            Vibrator vibrator = (Vibrator) getSystemService(VIBRATOR_SERVICE);
            vibrator.vibrate(VIBRATE_DURATION);
        }
    }

    /**
     * When the beep has finished playing, rewind to queue up another one.
     */
    private final OnCompletionListener beepListener = new OnCompletionListener() {
        public void onCompletion(MediaPlayer mediaPlayer) {
            mediaPlayer.seekTo(0);
        }
    };

}