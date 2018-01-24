package com.meigsmart.meigapp.model;

import java.io.Serializable;

/**
 * 语音消息列表实体类
 * Created by chenMeng on 2017/9/16.
 */

public class VoiceMessageModel implements Serializable {
    private String id;
    private String created;
    private String client_uuid;////uuid
    private String download_url;
    private String latitude;
    private String longitude;
    private String voice_message_url;
    private String messageType;//1 文本  0 语音
    private String name;
    private String voiceTime;
    private String type;//1 app  0 设备

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getVoiceTime() {
        return voiceTime;
    }

    public void setVoiceTime(String voiceTime) {
        this.voiceTime = voiceTime;
    }

    public String getVoice_message_url() {
        return voice_message_url;
    }

    public void setVoice_message_url(String voice_message_url) {
        this.voice_message_url = voice_message_url;
    }

    public String getMessageType() {
        return messageType;
    }

    public void setMessageType(String messageType) {
        this.messageType = messageType;
    }

    public String getCreated() {
        return created;
    }

    public void setCreated(String created) {
        this.created = created;
    }

    public String getClient_uuid() {
        return client_uuid;
    }

    public void setClient_uuid(String client_uuid) {
        this.client_uuid = client_uuid;
    }

    public String getDownload_url() {
        return download_url;
    }

    public void setDownload_url(String download_url) {
        this.download_url = download_url;
    }

    public String getLatitude() {
        return latitude;
    }

    public void setLatitude(String latitude) {
        this.latitude = latitude;
    }

    public String getLongitude() {
        return longitude;
    }

    public void setLongitude(String longitude) {
        this.longitude = longitude;
    }
}
