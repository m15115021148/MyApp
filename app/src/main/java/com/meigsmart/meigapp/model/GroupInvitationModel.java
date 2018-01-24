package com.meigsmart.meigapp.model;

/**
 * Created by chenMeng on 2017/12/11.
 */

public class GroupInvitationModel extends BaseModel {
    private String token;
    private String expires;

    public String getToken() {
        return token;
    }

    public void setToken(String token) {
        this.token = token;
    }

    public String getExpires() {
        return expires;
    }

    public void setExpires(String expires) {
        this.expires = expires;
    }
}
