package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * 群组列表 获取的网络数据 实体类
 * Created by chenMeng on 2017/9/14.
 */

public class GroupListDataModel extends BaseModel{
    private List<GroupListModel> groups;

    public List<GroupListModel> getGroups() {
        return groups;
    }

    public void setGroups(List<GroupListModel> groups) {
        this.groups = groups;
    }
}
