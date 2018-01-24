package com.meigsmart.charlibs.viewholder;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.meigsmart.charlibs.R;
import com.meigsmart.charlibs.adapter.ChatRecyclerAdapter;
import com.meigsmart.charlibs.bean.ChatConst;
import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.util.Utils;


public class ChatItemHolder extends CommonViewHolder {
    protected boolean isLeft;

    protected ImageView avatarView;
    protected TextView timeView;
    protected TextView nameView;
    protected LinearLayout conventLayout;
    protected FrameLayout statusLayout;
    protected ProgressBar progressBar;
    protected TextView statusView;
    protected ImageView errorView;
    protected ChatMessageBean message;

    protected ChatRecyclerAdapter.OnCallBackClickListener mCallBack;
    protected int mPosition;

    public ChatItemHolder(Context context, ViewGroup root, boolean isLeft) {
        super(context, root, isLeft ? R.layout.chat_item_left_layout : R.layout.chat_item_right_layout);
        this.isLeft = isLeft;
        initView();
        setOnClickListener();
    }

    public void initView() {
        if (isLeft) {
            avatarView = (ImageView) itemView.findViewById(R.id.chat_left_iv_avatar);
            timeView = (TextView) itemView.findViewById(R.id.chat_left_tv_time);
            nameView = (TextView) itemView.findViewById(R.id.chat_left_tv_name);
            conventLayout = (LinearLayout) itemView.findViewById(R.id.chat_left_layout_content);
            statusLayout = (FrameLayout) itemView.findViewById(R.id.chat_left_layout_status);
            statusView = (TextView) itemView.findViewById(R.id.chat_left_tv_status);
            progressBar = (ProgressBar) itemView.findViewById(R.id.chat_left_progressbar);
            errorView = (ImageView) itemView.findViewById(R.id.chat_left_tv_error);
        } else {
            avatarView = (ImageView) itemView.findViewById(R.id.chat_right_iv_avatar);
            timeView = (TextView) itemView.findViewById(R.id.chat_right_tv_time);
            nameView = (TextView) itemView.findViewById(R.id.chat_right_tv_name);
            conventLayout = (LinearLayout) itemView.findViewById(R.id.chat_right_layout_content);
            statusLayout = (FrameLayout) itemView.findViewById(R.id.chat_right_layout_status);
            progressBar = (ProgressBar) itemView.findViewById(R.id.chat_right_progressbar);
            errorView = (ImageView) itemView.findViewById(R.id.chat_right_tv_error);
            statusView = (TextView) itemView.findViewById(R.id.chat_right_tv_status);
        }
    }

    @Override
    public void bindData(Object o) {
        message = (ChatMessageBean) o;
        timeView.setText(Utils.millisecsToDateString(message.getTime()));
        nameView.setText(message.getUserName());

        if (isLeft) {
            avatarView.setImageResource(R.mipmap.ic_head_01);
        } else {
            avatarView.setImageResource(R.mipmap.ic_head_02);
        }
        if (message.getSendState() == ChatConst.SENDING){
            statusLayout.setVisibility(View.VISIBLE);
            progressBar.setVisibility(View.VISIBLE);
            statusView.setVisibility(View.GONE);
            errorView.setVisibility(View.GONE);
        }else if (message.getSendState() == ChatConst.COMPLETED){
            statusLayout.setVisibility(View.VISIBLE);
            progressBar.setVisibility(View.GONE);
            statusView.setVisibility(View.VISIBLE);
            statusView.setVisibility(View.GONE);
            errorView.setVisibility(View.GONE);
        }else if (message.getSendState() == ChatConst.SENDERROR){
            statusLayout.setVisibility(View.VISIBLE);
            progressBar.setVisibility(View.GONE);
            statusView.setVisibility(View.GONE);
            errorView.setVisibility(View.VISIBLE);
        }
    }

    public void showTimeView(boolean isShow) {
        timeView.setVisibility(isShow ? View.VISIBLE : View.GONE);
    }

    public void showUserName(boolean isShow) {
        if (isLeft){
            nameView.setVisibility(isShow ? View.VISIBLE : View.GONE);
        }else{
            nameView.setVisibility(View.GONE);
        }
    }

    public void setCallBackPosition(ChatRecyclerAdapter.OnCallBackClickListener callBack, int position){
        mCallBack = callBack;
        mPosition = position;
    }

    public void setOnClickListener(){
        errorView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mCallBack!=null){
                    mCallBack.onSendErrorListener(mPosition,message);
                }
            }
        });
        avatarView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mCallBack!=null){
                    mCallBack.onClickHeaderListener(message,isLeft);
                }
            }
        });
//        conventLayout.setOnLongClickListener(new View.OnLongClickListener() {
//            @Override
//            public boolean onLongClick(View v) {
//                if (mCallBack!=null){
//                    mCallBack.onLongItemClickListener(mPosition,message,0);
//                }
//                return false;
//            }
//        });
    }
}
