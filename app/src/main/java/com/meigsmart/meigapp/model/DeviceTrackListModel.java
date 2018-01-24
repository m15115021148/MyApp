package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * 7.2.获取设备轨迹
 * Created by chenMeng on 2017/9/21.
 */

public class DeviceTrackListModel extends BaseModel {

    private List<DeviceTrackModel> data;

    public List<DeviceTrackModel> getData() {
        return data;
    }

    public void setData(List<DeviceTrackModel> data) {
        this.data = data;
    }

    public class DeviceTrackModel {
        private String id;
        private String deviceId;
        private String deviceUuid;
        private String trackTime;
        private String latitude;
        private String longitude;
        private String velocity;
        private String altitude;
        private String direction;
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
