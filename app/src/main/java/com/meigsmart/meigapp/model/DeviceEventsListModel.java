package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * 7.3.获取设备事件列表
 * Created by chenMeng on 2017/10/17.
 */

public class DeviceEventsListModel extends BaseModel {
    private List<DeviceEventModel> data;

    public List<DeviceEventModel> getData() {
        return data;
    }

    public void setData(List<DeviceEventModel> data) {
        this.data = data;
    }

    public class DeviceEventModel {
        private String id;
        private String deviceId;
        private String deviceUuid;
        private String eventTime;
        private String eventType;
        private String bat;
        private String geo;
        private String gf;
        private String evn;
        private String vac;
        private long createTime;

        public long getCreateTime() {
            return createTime;
        }

        public void setCreateTime(long createTime) {
            this.createTime = createTime;
        }

        public String getId() {
            return id;
        }

        public void setId(String id) {
            this.id = id;
        }

        public String getDeviceId() {
            return deviceId;
        }

        public void setDeviceId(String deviceId) {
            this.deviceId = deviceId;
        }

        public String getDeviceUuid() {
            return deviceUuid;
        }

        public void setDeviceUuid(String deviceUuid) {
            this.deviceUuid = deviceUuid;
        }

        public String getEventTime() {
            return eventTime;
        }

        public void setEventTime(String eventTime) {
            this.eventTime = eventTime;
        }

        public String getEventType() {
            return eventType;
        }

        public void setEventType(String eventType) {
            this.eventType = eventType;
        }

        public String getBat() {
            return bat;
        }

        public void setBat(String bat) {
            this.bat = bat;
        }

        public String getGeo() {
            return geo;
        }

        public void setGeo(String geo) {
            this.geo = geo;
        }

        public String getGf() {
            return gf;
        }

        public void setGf(String gf) {
            this.gf = gf;
        }

        public String getEvn() {
            return evn;
        }

        public void setEvn(String evn) {
            this.evn = evn;
        }

        public String getVac() {
            return vac;
        }

        public void setVac(String vac) {
            this.vac = vac;
        }
    }

}
