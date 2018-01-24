package com.meigsmart.meigapp.model;

import android.support.annotation.NonNull;

import java.util.List;

/**
 * Created by chenMeng on 2017/12/12.
 */

public class SystemMessageModel extends BaseModel  {
    private List<Message> data;

    public List<Message> getData() {
        return data;
    }

    public void setData(List<Message> data) {
        this.data = data;
    }

    public static class Message {
        private String message;
        private long sendTime;

        public String getMessage() {
            return message;
        }

        public void setMessage(String message) {
            this.message = message;
        }

        public long getSendTime() {
            return sendTime;
        }

        public void setSendTime(long sendTime) {
            this.sendTime = sendTime;
        }

    }
}
