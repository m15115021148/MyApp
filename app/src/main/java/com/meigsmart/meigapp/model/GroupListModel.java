package com.meigsmart.meigapp.model;

import java.io.Serializable;
import java.util.List;

/**
 * 群组列表
 * Created by chenMeng on 2017/9/14.
 */

public class GroupListModel extends BaseModel implements Serializable{
    private String name;
    private String group_uuid;
    private String station_serial_number;
    private String station_client_uuid;
    private List<MembersModel> members;
    private int select = 0;//是否选择 0 没有  1 选择

    public int getSelect() {
        return select;
    }

    public void setSelect(int select) {
        this.select = select;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getGroup_uuid() {
        return group_uuid;
    }

    public void setGroup_uuid(String group_uuid) {
        this.group_uuid = group_uuid;
    }

    public String getStation_serial_number() {
        return station_serial_number;
    }

    public void setStation_serial_number(String station_serial_number) {
        this.station_serial_number = station_serial_number;
    }

    public String getStation_client_uuid() {
        return station_client_uuid;
    }

    public void setStation_client_uuid(String station_client_uuid) {
        this.station_client_uuid = station_client_uuid;
    }

    public List<MembersModel> getMembers() {
        return members;
    }

    public void setMembers(List<MembersModel> members) {
        this.members = members;
    }
}
