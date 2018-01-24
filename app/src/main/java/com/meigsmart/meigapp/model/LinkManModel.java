package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * Created by chenMeng on 2017/12/29.
 */

public class LinkManModel {
    private int type;//0 group list   1 device list
    private String name;
    private String groupId;
    private List<Person> list;

    public int getType() {
        return type;
    }

    public void setType(int type) {
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public List<Person> getList() {
        return list;
    }

    public void setList(List<Person> list) {
        this.list = list;
    }

    public String getGroupId() {
        return groupId;
    }

    public void setGroupId(String groupId) {
        this.groupId = groupId;
    }

    public static class Person{
        private int type;//1 app  0 device
        private String name;
        private String uuid;//
        private String groupUUid;
        private String serialNum;

        public String getSerialNum() {
            return serialNum;
        }

        public void setSerialNum(String serialNum) {
            this.serialNum = serialNum;
        }

        public int getType() {
            return type;
        }

        public void setType(int type) {
            this.type = type;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public String getUuid() {
            return uuid;
        }

        public void setUuid(String uuid) {
            this.uuid = uuid;
        }

        public String getGroupUUid() {
            return groupUUid;
        }

        public void setGroupUUid(String groupUUid) {
            this.groupUUid = groupUUid;
        }
    }
}
