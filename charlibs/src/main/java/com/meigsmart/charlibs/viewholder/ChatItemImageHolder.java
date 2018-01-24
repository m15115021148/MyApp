package com.meigsmart.charlibs.viewholder;

import android.content.Context;
import android.view.View;
import android.view.ViewGroup;

import com.meigsmart.charlibs.R;
import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.util.PhotoUtils;

import io.github.leibnik.chatimageview.ChatImageView;


public class ChatItemImageHolder extends ChatItemHolder {

    protected ChatImageView contentView;

    public ChatItemImageHolder(Context context, ViewGroup root, boolean isLeft) {
        super(context, root, isLeft);
    }

    @Override
    public void initView() {
        super.initView();
        if (isLeft) {
            conventLayout.addView(View.inflate(getContext(), R.layout.chat_item_image_layout, null));
        } else {
            conventLayout.addView(View.inflate(getContext(), R.layout.chat_item_right_image_layout, null));
        }
        contentView = (ChatImageView) itemView.findViewById(R.id.chat_item_image_view);

        contentView.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
//                Intent intent = new Intent(getContext(), ImageBrowserActivity.class);
//                intent.putExtra(Constants.IMAGE_LOCAL_PATH, message.getImageLocal());
//                intent.putExtra(Constants.IMAGE_URL, message.getImageUrl());
//                getContext().startActivity(intent);
            }
        });
    }

    @Override
    public void bindData(Object o) {
        super.bindData(o);
        ChatMessageBean message = (ChatMessageBean) o;
        if (message != null) {
            String localFilePath = message.getImageLocal();
            PhotoUtils.displayImageCacheElseNetwork(contentView, localFilePath, message.getImageUrl());
        }
    }
}