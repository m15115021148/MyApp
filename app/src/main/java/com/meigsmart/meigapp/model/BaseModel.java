package com.meigsmart.meigapp.model;

/**
 * 基类 实体类
 * Created by chenMeng on 2017/9/14.
 */

public abstract class BaseModel {
    private String result;
    private String reason;

    public String getResult() {
        return result;
    }

    public void setResult(String result) {
        this.result = result;
    }

    public String getReason() {
        return reason;
    }

    public void setReason(String reason) {
        this.reason = reason;
    }
}
