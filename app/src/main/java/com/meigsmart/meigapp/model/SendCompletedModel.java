package com.meigsmart.meigapp.model;

/**
 * 4.6.发送完成  实体类
 * Created by chenMeng on 2017/9/18.
 */

public class SendCompletedModel extends BaseModel{
    private SendModel send_voice_message;

    public SendModel getVoice_message_compleated() {
        return send_voice_message;
    }

    public void setVoice_message_compleated(SendModel voice_message_compleated) {
        this.send_voice_message = voice_message_compleated;
    }
}
