package com.meigsmart.meigapp.model;

/**
 * 语音消息实体类
 * Created by chenMeng on 2017/9/16.
 */

public class VoiceMsgServiceModel{
    private String url;
    private String upload_key;

    public String getUrl() {
        return url;
    }

    public void setUrl(String url) {
        this.url = url;
    }

    public String getUpload_key() {
        return upload_key;
    }

    public void setUpload_key(String upload_key) {
        this.upload_key = upload_key;
    }
}
