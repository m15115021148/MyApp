package com.meigsmart.meigapp.model;


import java.io.Serializable;

/**
 * 设备/app 激活返回实体类
 * Created by chenMeng on 2017/9/5.
 */
public class ClientsModel extends BaseModel implements Serializable{
    private String client_uuid;
    private String api_password;
    private SipModel sip;

    public String getClient_uuid() {
        return client_uuid;
    }

    public void setClient_uuid(String client_uuid) {
        this.client_uuid = client_uuid;
    }

    public String getApi_password() {
        return api_password;
    }

    public void setApi_password(String api_password) {
        this.api_password = api_password;
    }

    public SipModel getSip() {
        return sip;
    }

    public void setSip(SipModel sip) {
        this.sip = sip;
    }

    public static class SipModel implements Serializable{
        private String username;
        private String domain;
        private String password;
        private String proxy;
        private String registrar;

        public String getUsername() {
            return username;
        }

        public void setUsername(String username) {
            this.username = username;
        }

        public String getDomain() {
            return domain;
        }

        public void setDomain(String domain) {
            this.domain = domain;
        }

        public String getPassword() {
            return password;
        }

        public void setPassword(String password) {
            this.password = password;
        }

        public String getProxy() {
            return proxy;
        }

        public void setProxy(String proxy) {
            this.proxy = proxy;
        }

        public String getRegistrar() {
            return registrar;
        }

        public void setRegistrar(String registrar) {
            this.registrar = registrar;
        }
    }
}
