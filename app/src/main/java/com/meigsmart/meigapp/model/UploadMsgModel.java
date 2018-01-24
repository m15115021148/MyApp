package com.meigsmart.meigapp.model;


/**
 * 4.5.发送语音消息 返回实体类
 * Created by chenMeng on 2017/9/18.
 */

public class UploadMsgModel extends BaseModel {
    private SendVoiceMessageModel send_voice_message;

    public SendVoiceMessageModel getSend_voice_message() {
        return send_voice_message;
    }

    public void setSend_voice_message(SendVoiceMessageModel send_voice_message) {
        this.send_voice_message = send_voice_message;
    }

    public static class SendVoiceMessageModel {

        private String uuid;
        private String url;
        private String upload_key ;
        private String client_uuid ;
        private String latitude ;
        private String longitude;
        private String created ;

        public String getUuid() {
            return uuid;
        }

        public void setUuid(String uuid) {
            this.uuid = uuid;
        }

        public String getUpload_key() {
            return upload_key;
        }

        public void setUpload_key(String upload_key) {
            this.upload_key = upload_key;
        }

        public String getUrl() {
            return url;
        }

        public void setUrl(String url) {
            this.url = url;
        }


        public String getClient_uuid() {
            return client_uuid;
        }

        public void setClient_uuid(String client_uuid) {
            this.client_uuid = client_uuid;
        }

        public String getLatitude() {
            return latitude;
        }

        public void setLatitude(String latitude) {
            this.latitude = latitude;
        }

        public String getLongitude() {
            return longitude;
        }

        public void setLongitude(String longitude) {
            this.longitude = longitude;
        }

        public String getCreated() {
            return created;
        }

        public void setCreated(String created) {
            this.created = created;
        }
    }
}
