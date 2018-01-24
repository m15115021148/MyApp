package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * 取每个群成员的最近一个位置的列表
 * Created by chenMeng on 2017/10/17.
 */

public class GroupMembersTrackModel extends BaseModel{
    private List<MembersTrackModel> data;

    public List<MembersTrackModel> getData() {
        return data;
    }

    public void setData(List<MembersTrackModel> data) {
        this.data = data;
    }

    public class MembersTrackModel{
        private String id;
        private String deviceId;
        private String deviceUuid;
        private String deviceName;
        private String trackTime;
        private String latitude;
        private String longitude;
        private String velocity;
        private String altitude;
        private String direction;
        private String online;

        public String getOnline() {
            return online;
        }

        public void setOnline(String online) {
            this.online = online;
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

        public String getDeviceName() {
            return deviceName;
        }

        public void setDeviceName(String deviceName) {
            this.deviceName = deviceName;
        }

        public String getTrackTime() {
            return trackTime;
        }

        public void setTrackTime(String trackTime) {
            this.trackTime = trackTime;
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

        public String getVelocity() {
            return velocity;
        }

        public void setVelocity(String velocity) {
            this.velocity = velocity;
        }

        public String getAltitude() {
            return altitude;
        }

        public void setAltitude(String altitude) {
            this.altitude = altitude;
        }

        public String getDirection() {
            return direction;
        }

        public void setDirection(String direction) {
            this.direction = direction;
        }
    }
}
