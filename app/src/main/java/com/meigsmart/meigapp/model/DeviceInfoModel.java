package com.meigsmart.meigapp.model;

/**
 * 7.1.4.设备详细信息
 * Created by chenMeng on 2017/9/20.
 */

public class DeviceInfoModel extends BaseModel {
    private DeviceInfo data;

    public DeviceInfo getData() {
        return data;
    }

    public void setData(DeviceInfo data) {
        this.data = data;
    }

    public class DeviceInfo{
        private String id;
        private String name;
        private String token;
        private String bundleId;
        private String createTime;
        private String activeTime;
        private String uuid;
        private String sipUserId;
        private String deviceType;
        private String lang;
        private String serialNumber;
        private String bluetoothPwd;
        private String sipUsername;

        public String getSipUsername() {
            return sipUsername;
        }

        public void setSipUsername(String sipUsername) {
            this.sipUsername = sipUsername;
        }

        public String getId() {
            return id;
        }

        public void setId(String id) {
            this.id = id;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public String getToken() {
            return token;
        }

        public void setToken(String token) {
            this.token = token;
        }

        public String getBundleId() {
            return bundleId;
        }

        public void setBundleId(String bundleId) {
            this.bundleId = bundleId;
        }

        public String getCreateTime() {
            return createTime;
        }

        public void setCreateTime(String createTime) {
            this.createTime = createTime;
        }

        public String getActiveTime() {
            return activeTime;
        }

        public void setActiveTime(String activeTime) {
            this.activeTime = activeTime;
        }

        public String getUuid() {
            return uuid;
        }

        public void setUuid(String uuid) {
            this.uuid = uuid;
        }

        public String getSipUserId() {
            return sipUserId;
        }

        public void setSipUserId(String sipUserId) {
            this.sipUserId = sipUserId;
        }

        public String getDeviceType() {
            return deviceType;
        }

        public void setDeviceType(String deviceType) {
            this.deviceType = deviceType;
        }

        public String getLang() {
            return lang;
        }

        public void setLang(String lang) {
            this.lang = lang;
        }

        public String getSerialNumber() {
            return serialNumber;
        }

        public void setSerialNumber(String serialNumber) {
            this.serialNumber = serialNumber;
        }

        public String getBluetoothPwd() {
            return bluetoothPwd;
        }

        public void setBluetoothPwd(String bluetoothPwd) {
            this.bluetoothPwd = bluetoothPwd;
        }
    }
}
