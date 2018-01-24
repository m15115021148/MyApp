package com.meigsmart.charlibs.viewholder;

import android.content.Context;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.ImageView;
import android.widget.TextView;

import com.meigsmart.charlibs.R;
import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.listener.PlayButtonClickListener;
import com.meigsmart.charlibs.view.PlayButton;

import java.text.DecimalFormat;


public class ChatItemAudioHolder extends ChatItemHolder {

    public PlayButton playButton;
    protected TextView durationView;
    private ImageView ivAudioUnread;
    private String path;

    private int mMinItemWith;// 设置对话框的最大宽度和最小宽度
    private int mMaxItemWith;

    public ChatItemAudioHolder(Context context, ViewGroup root, boolean isLeft) {
        super(context, root, isLeft);
    }

    @Override
    public void initView() {
        super.initView();
        // 获取系统宽度
        WindowManager wManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        DisplayMetrics outMetrics = new DisplayMetrics();
        wManager.getDefaultDisplay().getMetrics(outMetrics);
        mMaxItemWith = (int) (outMetrics.widthPixels * 0.35f);
        mMinItemWith = (int) (outMetrics.widthPixels * 0.2f);
        if (isLeft) {
            conventLayout.addView(View.inflate(getContext(), R.layout.chat_item_left_audio_layout, null));
            ivAudioUnread = (ImageView) itemView.findViewById(R.id.chat_item_audio_unread);
        } else {
            conventLayout.addView(View.inflate(getContext(), R.layout.chat_item_right_audio_layout, null));
        }
        playButton = (PlayButton) itemView.findViewById(R.id.chat_item_audio_play_btn);
        durationView = (TextView) itemView.findViewById(R.id.chat_item_audio_duration_view);
    }

    @Override
    public void bindData(Object o) {
        super.bindData(o);
        if (o instanceof ChatMessageBean) {
            final ChatMessageBean audioMessage = (ChatMessageBean) o;
            DecimalFormat decimalFormat=new DecimalFormat("0.0");//构造方法的字符格式这里如果小数不足2位,会以0补足.
            String p = decimalFormat.format(audioMessage.getUserVoiceTime());//format 返回的是字符串
            durationView.setText(p+"\"");
            ViewGroup.LayoutParams layoutParams = playButton.getLayoutParams();
            layoutParams.width = (int) (mMinItemWith + mMaxItemWith / 60f * audioMessage.getUserVoiceTime());
            playButton.setLayoutParams(layoutParams);
            String localFilePath = audioMessage.getUserVoicePath();
            final boolean leftSide = audioMessage.getType() == 1;
            if (!TextUtils.isEmpty(localFilePath)) {
                path = localFilePath;
            } else {
                path = audioMessage.getUserVoiceUrl();
            }
            playButton.setPath(path);
            if (leftSide && ivAudioUnread != null)
                if (audioMessage.getIsListened()) {
                    ivAudioUnread.setVisibility(View.INVISIBLE);
                }else{
                    ivAudioUnread.setVisibility(View.VISIBLE);
                }
            playButton.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    new PlayButtonClickListener(playButton, ivAudioUnread, audioMessage, leftSide, getContext(), path).onClick(v);
                }
            });
            playButton.setOnLongClickListener(new View.OnLongClickListener() {
                @Override
                public boolean onLongClick(View v) {
                    if (mCallBack!=null){
                        mCallBack.onLongItemClickListener(conventLayout,mPosition,audioMessage,1);
                    }
                    return true;//长按松开 不触发OnClickListener事件
                }
            });
        }
    }

}