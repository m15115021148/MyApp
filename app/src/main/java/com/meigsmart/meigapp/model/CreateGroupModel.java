package com.meigsmart.meigapp.model;

/**
 * 创建群组
 * Created by chenMeng on 2017/9/14.
 */

public class CreateGroupModel extends BaseModel{
    private String group_uuid;//群组UUID

    public String getGroup_uuid() {
        return group_uuid;
    }

    public void setGroup_uuid(String group_uuid) {
        this.group_uuid = group_uuid;
    }
}
