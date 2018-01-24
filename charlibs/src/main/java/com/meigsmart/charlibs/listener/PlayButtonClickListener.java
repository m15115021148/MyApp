package com.meigsmart.charlibs.listener;

import android.content.Context;
import android.graphics.drawable.AnimationDrawable;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;


import com.meigsmart.charlibs.R;
import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.db.CharDataDao;
import com.meigsmart.charlibs.view.PlayButton;

import java.io.File;

public class PlayButtonClickListener implements View.OnClickListener {
    private PlayButton playButton;
    private boolean leftSide;
    private Context context;
    private String path;

    private AnimationDrawable anim;
    public static PlayButtonClickListener mCurrentPlayButtonClickListner = null;
    private MediaPlayer mediaPlayer = null;
    public static boolean isPlaying = false;
    // 将MediaPlayer播放地址，
    public static String audioPath;
    private ImageView ivIsListened;
    private ChatMessageBean chatMessageBean;

    public PlayButtonClickListener(PlayButton playButton, ImageView ivIsListened, ChatMessageBean chatMessageBean, boolean leftSide, Context context, String path) {
        this.playButton = playButton;
        this.leftSide = leftSide;
        this.context = context;
        this.path = path;
        this.ivIsListened = ivIsListened;
        this.chatMessageBean = chatMessageBean;
    }

    @Override
    public void onClick(View v) {
        if (isPlaying) {
            if (TextUtils.equals(audioPath, path)) {
                mCurrentPlayButtonClickListner.stopPlayVoice();
                return;
            }
            mCurrentPlayButtonClickListner.stopPlayVoice();
        }
        playButton.startRecordAnimation();
        playVoice(path);
        if (leftSide && ivIsListened != null) {
            ivIsListened.setVisibility(View.INVISIBLE);
            chatMessageBean.setIsListened(true);

            //保存数据库 字段  更新 已读
            new CharDataDao(context).updateDataListened(String.valueOf(chatMessageBean.getId()),true);
        }
    }

    public void stopPlayVoice() {
        playButton.stopRecordAnimation();
        if (mediaPlayer != null) {
            mediaPlayer.stop();
            mediaPlayer.release();
        }
        isPlaying = false;
    }

    public void playVoice(String filePath) {
        if (!(new File(filePath).exists())) {
            Toast.makeText(context.getApplicationContext(),context.getResources().getString(R.string.error),Toast.LENGTH_SHORT).show();
            stopPlayVoice(); // stop animation
            return;
        }
        mediaPlayer = new MediaPlayer();
        this.audioPath = path;
        try {
            mediaPlayer.setDataSource(filePath);

//            mediaPlayer.setAudioStreamType(AudioManager.STREAM_VOICE_CALL);

            mediaPlayer.prepare();

//            mediaPlayer.prepareAsync();
            mediaPlayer.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {

                @Override
                public void onCompletion(MediaPlayer mp) {
                    mediaPlayer.release();
                    mediaPlayer = null;
                    stopPlayVoice(); // stop animation
                }

            });
            isPlaying = true;
            mCurrentPlayButtonClickListner = this;
            mediaPlayer.start();
            playButton.startRecordAnimation();
        } catch (Exception e) {
            System.out.println();
        }
    }


}
