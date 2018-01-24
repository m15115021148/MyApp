package com.meigsmart.meigapp.model;

import java.io.Serializable;

/**
 * Created by chenMeng on 2017/9/12.
 */

public class MessageModel implements Serializable{
    private String msg;
    private String name;
    private String time;

    public String getMsg() {
        return msg;
    }

    public void setMsg(String msg) {
        this.msg = msg;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }
}
