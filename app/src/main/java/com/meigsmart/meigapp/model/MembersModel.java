package com.meigsmart.meigapp.model;

import java.io.Serializable;

/**
 * 成员实体类
 * Created by chenMeng on 2017/9/14.
 */

public class MembersModel implements Serializable{
    private String name;
    private String client_uuid;
    private String type;//1app 0设备
    private String online;//0 不在线，1 在线
    private String serialNumber;
    private String sipUsername;

    public String getSerialNumber() {
        return serialNumber;
    }

    public void setSerialNumber(String serialNumber) {
        this.serialNumber = serialNumber;
    }

    public String getSipUsername() {
        return sipUsername;
    }

    public void setSipUsername(String sipUsername) {
        this.sipUsername = sipUsername;
    }

    public String getOnline() {
        return online;
    }

    public void setOnline(String online) {
        this.online = online;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getClient_uuid() {
        return client_uuid;
    }

    public void setClient_uuid(String client_uuid) {
        this.client_uuid = client_uuid;
    }
}
