package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * Created by chenMeng on 2017/12/12.
 */

public class SystemMsgBean extends BaseModel {
    private String time;
    private List<MessageModel> list;
    private boolean isExpand;

    public boolean isExpand() {
        return isExpand;
    }

    public void setExpand(boolean expand) {
        isExpand = expand;
    }

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }

    public List<MessageModel> getList() {
        return list;
    }

    public void setList(List<MessageModel> list) {
        this.list = list;
    }

    public static class MessageModel {
        private String time;
        private String message;

        public String getTime() {
            return time;
        }

        public void setTime(String time) {
            this.time = time;
        }

        public String getMessage() {
            return message;
        }

        public void setMessage(String message) {
            this.message = message;
        }

    }

}
