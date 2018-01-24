package com.meigsmart.meigapp.model;

import java.util.List;

/**
 * 7.1.1.获取设备列表
 * Created by chenMeng on 2017/9/19.
 */

public class DeviceListModel extends BaseModel {
    private List<DeviceModel> data;

    public List<DeviceModel> getData() {
        return data;
    }

    public void setData(List<DeviceModel> data) {
        this.data = data;
    }

    public class DeviceModel {

        private String id;
        private String name;
        private String token;
        private String bundleId;
        private String createTime;
        private String activeTime;
        private String uuid;//device_uuid
        private String sipUserId;
        private String deviceType;
        private String lang;
        private String serialNumber;
        private String bluetoothPwd;
        private String sipUsername;
        private boolean isSelect = false;//是否选择

        public String getSipUsername() {
            return sipUsername;
        }

        public void setSipUsername(String sipUsername) {
            this.sipUsername = sipUsername;
        }

        public boolean isSelect() {
            return isSelect;
        }

        public void setSelect(boolean select) {
            isSelect = select;
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
