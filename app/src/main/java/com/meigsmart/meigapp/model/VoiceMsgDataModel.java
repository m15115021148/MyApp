package com.meigsmart.meigapp.model;

/**
 * 获取语音消息服务 数据 实体类
 * Created by chenMeng on 2017/9/16.
 */

public class VoiceMsgDataModel extends BaseModel {
    private VoiceMsgServiceModel voice_message_server;

    public VoiceMsgServiceModel getVoice_message_server() {
        return voice_message_server;
    }

    public void setVoice_message_server(VoiceMsgServiceModel voice_message_server) {
        this.voice_message_server = voice_message_server;
    }
}
