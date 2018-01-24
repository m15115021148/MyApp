package com.meigsmart.meigapp.model;

/**
 * 发送完成 请求参数实体类
 * Created by chenMeng on 2017/9/18.
 */

public class SendComplete {
    private String uuid ;
    private String url;
    private String upload_key ;
    private String client_uuid ;
    private String latitude;
    private String longitude;
    private String messageType;//类型

    public String getMessageType() {
        return messageType;
    }

    public void setMessageType(String messageType) {
        this.messageType = messageType;
    }

    public String getUuid() {
        return uuid;
    }

    public void setUuid(String uuid) {
        this.uuid = uuid;
    }

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

    public String getClient_uuid() {
        return client_uuid;
    }

    public void setClient_uuid(String client_uuid) {
        this.client_uuid = client_uuid;
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
