package com.meigsmart.charlibs.db;

/**
 * Created by chenMeng on 2017/11/29.
 */

public class BluetoothBean {
    private String id;
    private String blueName;
    private String serialNum;
    private String isFirstSetPsw;
    private String password;

    public String getId() {
        return id;
    }

    public void setId(String id) {
        this.id = id;
    }

    public String getBlueName() {
        return blueName;
    }

    public void setBlueName(String blueName) {
        this.blueName = blueName;
    }

    public String getSerialNum() {
        return serialNum;
    }

    public void setSerialNum(String serialNum) {
        this.serialNum = serialNum;
    }

    public String getIsFirstSetPsw() {
        return isFirstSetPsw;
    }

    public void setIsFirstSetPsw(String isFirstSetPsw) {
        this.isFirstSetPsw = isFirstSetPsw;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }
}
