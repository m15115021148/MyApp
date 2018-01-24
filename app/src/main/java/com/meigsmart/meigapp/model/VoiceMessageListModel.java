package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * 4.7.获取语音消息列表
 * Created by chenMeng on 2017/9/16.
 */

public class VoiceMessageListModel extends BaseModel {
    private List<VoiceMessageModel> voice_messages;

    public List<VoiceMessageModel> getVoice_messages() {
        return voice_messages;
    }

    public void setVoice_messages(List<VoiceMessageModel> voice_messages) {
        this.voice_messages = voice_messages;
    }
}
