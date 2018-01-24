package com.meigsmart.meigapp.model;

/**
 * 添加成员
 * Created by chenMeng on 2017/9/14.
 */

public class MemberAddModel extends BaseModel{
    private String group_uuid;
    private String client_uuid;

    public String getGroup_uuid() {
        return group_uuid;
    }

    public void setGroup_uuid(String group_uuid) {
        this.group_uuid = group_uuid;
    }

    public String getClient_uuid() {
        return client_uuid;
    }

    public void setClient_uuid(String client_uuid) {
        this.client_uuid = client_uuid;
    }
}
