package com.meigsmart.charlibs.adapter;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;

import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.bean.ChatMessageType;
import com.meigsmart.charlibs.listener.PlayButtonClickListener;
import com.meigsmart.charlibs.viewholder.ChatItemAudioHolder;
import com.meigsmart.charlibs.viewholder.ChatItemHolder;
import com.meigsmart.charlibs.viewholder.ChatItemImageHolder;
import com.meigsmart.charlibs.viewholder.ChatItemTextHolder;
import com.meigsmart.charlibs.viewholder.CommonViewHolder;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class ChatRecyclerAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

    private final int ITEM_LEFT = 100;
    private final int ITEM_LEFT_TEXT = 101;// 左侧文字
    private final int ITEM_LEFT_IMAGE = 102;// 左侧图片
    private final int ITEM_LEFT_AUDIO = 103;// 左侧语音

    private final int ITEM_RIGHT = 200;
    private final int ITEM_RIGHT_TEXT = 201;// 右侧文字
    private final int ITEM_RIGHT_IMAGE = 202;// 右侧图片
    private final int ITEM_RIGHT_AUDIO = 203;// 右侧语音

    // 时间间隔最小为十分钟
    private final static long TIME_INTERVAL = 1000 * 60 * 3;
    private boolean isShowUserName = true;//是否显示名字

    private List<ChatMessageBean> chatMessageList = new ArrayList<>();
    private Context context;
    private int playButtonId = -1;
    private boolean isMe;
    private OnCallBackClickListener mCallBack;//回调

    public ChatRecyclerAdapter(Context context, List<ChatMessageBean> chatMessageList,OnCallBackClickListener callBack) {
        this.context = context;
        this.chatMessageList = chatMessageList;
        this.mCallBack = callBack;
    }

    @Override
    public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
        switch (viewType) {
            case ITEM_LEFT_TEXT:
                return new ChatItemTextHolder(parent.getContext(), parent, true);
            case ITEM_LEFT_AUDIO:
                return new ChatItemAudioHolder(parent.getContext(), parent, true);
            case ITEM_LEFT_IMAGE:
                return new ChatItemImageHolder(parent.getContext(), parent, true);
            case ITEM_RIGHT_TEXT:
                return new ChatItemTextHolder(parent.getContext(), parent, false);
            case ITEM_RIGHT_AUDIO:
                return new ChatItemAudioHolder(parent.getContext(), parent, false);
            case ITEM_RIGHT_IMAGE:
                return new ChatItemImageHolder(parent.getContext(), parent, false);
            default:
        }
        return null;
    }

    @Override
    public int getItemViewType(int position) {
        ChatMessageBean chatMessageBean = chatMessageList.get(position);
        if (null != chatMessageBean) {
            isMe = fromMe(chatMessageBean);
            int messagetype = chatMessageBean.getMessagetype();
            if (messagetype == ChatMessageType.TextMessageType.getType()) {
                return isMe ? ITEM_LEFT_TEXT : ITEM_RIGHT_TEXT;
            } else if (messagetype == ChatMessageType.ImageMessageType.getType()) {
                return isMe ? ITEM_LEFT_IMAGE : ITEM_RIGHT_IMAGE;
            } else if (messagetype == ChatMessageType.AudioMessageType.getType()) {
                return isMe ? ITEM_LEFT_AUDIO : ITEM_RIGHT_AUDIO;
            } else {
                return isMe ? ITEM_LEFT : ITEM_RIGHT;
            }

        }
        return 8888;
    }

    @Override
    public void onBindViewHolder(RecyclerView.ViewHolder holder, int position) {
        ((CommonViewHolder) holder).bindData(chatMessageList.get(position));
        if (holder instanceof ChatItemHolder) {
            ((ChatItemHolder) holder).showTimeView(shouldShowTime(position));
            ((ChatItemHolder) holder).showUserName(isShowUserName);
            ((ChatItemHolder) holder).setCallBackPosition(mCallBack,position);
        }
        if (getItemViewType(position) == ITEM_LEFT_AUDIO || getItemViewType(position) == ITEM_RIGHT_AUDIO) {
            playButtonId = ((ChatItemAudioHolder) holder).playButton.getId();
        }
    }

    @Override
    public int getItemCount() {
        return chatMessageList.size();
    }

    public void setChatMessageList(List<ChatMessageBean> chatMessageList) {
        this.chatMessageList.clear();
        if (null != chatMessageList) {
            this.chatMessageList.addAll(chatMessageList);
        }
    }

    public void addMessageListAll(List<ChatMessageBean> chatMessageList) {
        this.chatMessageList.addAll(0, chatMessageList);
        this.notifyDataSetChanged();
        this.notifyItemInserted(this.chatMessageList.size() - 1);
    }

    public void addMessageBean(ChatMessageBean message) {
        this.chatMessageList.add(message);
        this.notifyDataSetChanged();
        this.notifyItemInserted(this.chatMessageList.size() - 1);
    }

    public void addMessage(ChatMessageBean message) {
        this.chatMessageList.addAll(Collections.singletonList(message));
    }

    /**
     * 获取最后一条信息
     * @return
     */
    public ChatMessageBean getLastMsg(){
        if (null != chatMessageList && chatMessageList.size() > 0) {
            return this.chatMessageList.get(chatMessageList.size()-1);
        } else {
            return null;
        }
    }

    public void updateMessageBean(ChatMessageBean message){
        if (null != chatMessageList && chatMessageList.size() > 0) {
            this.chatMessageList.set(chatMessageList.size()-1,message);
            this.notifyDataSetChanged();
            this.notifyItemInserted(this.chatMessageList.size() - 1);
        }
    }

    public void updatePosMessageBean(int pos,ChatMessageBean message){
        if (null != chatMessageList && chatMessageList.size() > 0) {
            this.chatMessageList.set(pos,message);
            this.notifyDataSetChanged();
            this.notifyItemInserted(pos - 1);
        }
    }

    public void updateMovePos(int pos){
        if (null != chatMessageList && chatMessageList.size() > 0) {
            this.chatMessageList.remove(pos);
            this.notifyDataSetChanged();
        }
    }

    public void clear() {
        this.chatMessageList.clear();
        this.notifyDataSetChanged();
    }

    public ChatMessageBean getFirstMessage() {
        if (null != chatMessageList && chatMessageList.size() > 0) {
            return this.chatMessageList.get(0);
        } else {
            return null;
        }
    }

    private boolean fromMe(ChatMessageBean chatMessageBean) {
        int type = chatMessageBean.getType();
        if (type == 0) {
            return false;
        } else {
            return true;
        }
    }

    private boolean shouldShowTime(int position) {
        if (position == 0) {
            return true;
        }
        long lastTime = chatMessageList.get(position - 1).getTime();
        long curTime = chatMessageList.get(position).getTime();
        return curTime - lastTime > TIME_INTERVAL;
    }

    /**
     * 因为 RecyclerView 中的 item 缓存默认最大为 5，造成会重复的 create item 而卡顿
     * 所以这里根据不同的类型设置不同的缓存值，经验值，不同 app 可以根据自己的场景进行更改
     */
    public void resetRecycledViewPoolSize(RecyclerView recyclerView) {
        recyclerView.getRecycledViewPool().setMaxRecycledViews(ITEM_LEFT_TEXT, 25);
        recyclerView.getRecycledViewPool().setMaxRecycledViews(ITEM_LEFT_IMAGE, 10);
        recyclerView.getRecycledViewPool().setMaxRecycledViews(ITEM_LEFT_AUDIO, 15);

        recyclerView.getRecycledViewPool().setMaxRecycledViews(ITEM_RIGHT_TEXT, 25);
        recyclerView.getRecycledViewPool().setMaxRecycledViews(ITEM_RIGHT_IMAGE, 10);
        recyclerView.getRecycledViewPool().setMaxRecycledViews(ITEM_RIGHT_AUDIO, 15);
    }

    public void pausePlayer() {
        if (PlayButtonClickListener.isPlaying)
            PlayButtonClickListener.mCurrentPlayButtonClickListner.stopPlayVoice();
    }

    public interface OnCallBackClickListener{
        void onSendErrorListener(int position,ChatMessageBean bean);//发送消息 失败  按钮事件
        void onLongItemClickListener(View v, int position, ChatMessageBean bean, int type);//item 长按事件 type 0文本  1 语音
        void onClickHeaderListener(ChatMessageBean bean,boolean isLeft);//头像点击事件
    }

}
