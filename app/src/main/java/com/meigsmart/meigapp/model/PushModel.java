package com.meigsmart.meigapp.model;

/**
 * 消息 实体类 推送的
 * Created by chenMeng on 2017/10/13.
 */

public class PushModel {
    private Model meigmsg;
    private String messageType;

    public String getMessageType() {
        return messageType;
    }

    public void setMessageType(String messageType) {
        this.messageType = messageType;
    }

    public Model getMeigmsg() {
        return meigmsg;
    }

    public void setMeigmsg(Model meigmsg) {
        this.meigmsg = meigmsg;
    }

    public class Model{
        //{"meigmsg":{"from_client_uuid":"a_f4de6af552b3443ca749db97a69fa5b2","created":"2017-12-11T17:39:31+0800","lastId":"1836238411408448","message_type":"text_message","group_uuid":"b8bc0f3088104236940b7ea517e88cb1","from_name":"测试","voice_message_url":""}}
       // {"meigmsg":{"client_uuid":"d_058d187469cd462fab5b3f6915e73e47","message_type":"new_member","group_uuid":"b8bc0f3088104236940b7ea517e88cb1"}}
        //{'meigmsg':
        // {'group_uuid':'b8bc0f3088104236940b7ea517e88cb1','
        // from_client_uuid':'a_f4de6af552b3443ca749db97a69fa5b2','
        // from_name':'测试','message_type':'voice_message','
        // created':'2017-11-28T17:55:28+0800','
        // voice_message_url':'','
        // lastId':'1827044996498496'}
        // }
        private String group_uuid;
        private String from_client_uuid;
        private String from_name;
        private String message_type;
        private String created;
        private String voice_message_url;
        private String lastId;

        public String getGroup_uuid() {
            return group_uuid;
        }

        public void setGroup_uuid(String group_uuid) {
            this.group_uuid = group_uuid;
        }

        public String getFrom_client_uuid() {
            return from_client_uuid;
        }

        public void setFrom_client_uuid(String from_client_uuid) {
            this.from_client_uuid = from_client_uuid;
        }

        public String getFrom_name() {
            return from_name;
        }

        public void setFrom_name(String from_name) {
            this.from_name = from_name;
        }

        public String getMessage_type() {
            return message_type;
        }

        public void setMessage_type(String message_type) {
            this.message_type = message_type;
        }

        public String getCreated() {
            return created;
        }

        public void setCreated(String created) {
            this.created = created;
        }

        public String getVoice_message_url() {
            return voice_message_url;
        }

        public void setVoice_message_url(String voice_message_url) {
            this.voice_message_url = voice_message_url;
        }

        public String getLastId() {
            return lastId;
        }

        public void setLastId(String lastId) {
            this.lastId = lastId;
        }
    }
}
