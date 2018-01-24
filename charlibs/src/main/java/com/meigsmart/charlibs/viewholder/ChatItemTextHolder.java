package com.meigsmart.charlibs.viewholder;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.meigsmart.charlibs.R;
import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.util.EmotionHelper;


public class ChatItemTextHolder extends ChatItemHolder {

    protected TextView contentView;

    public ChatItemTextHolder(Context context, ViewGroup root, boolean isLeft) {
        super(context, root, isLeft);
    }

    @Override
    public void initView() {
        super.initView();
        if (isLeft) {
            conventLayout.addView(View.inflate(getContext(), R.layout.chat_item_left_text_layout, null));
            contentView = (TextView) itemView.findViewById(R.id.chat_left_text_tv_content);
        } else {
            conventLayout.addView(View.inflate(getContext(), R.layout.chat_item_right_text_layout, null));
            contentView = (TextView) itemView.findViewById(R.id.chat_right_text_tv_content);
        }
    }

    @Override
    public void bindData(Object o) {
        super.bindData(o);
        final ChatMessageBean message = (ChatMessageBean) o;
        if (message != null) {
            contentView.setText(EmotionHelper.replace(getContext(), message.getUserContent()));
        }
        contentView.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (mCallBack!=null){
                    mCallBack.onLongItemClickListener(conventLayout,mPosition,message,0);
                }
                return false;
            }
        });
    }
}
