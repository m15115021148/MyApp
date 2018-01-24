package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2017/12/1.
 */

public class BlueDeviceStatusModel extends BluBaseModel {
    private StatusModel Status;

    public StatusModel getStatus() {
        return Status;
    }

    public void setStatus(StatusModel Status) {
        this.Status = Status;
    }

    public static class StatusModel {

        private GeneralModel General;
        private CallsModel Calls;
        private ValNetworkModel val_Network;
        private GPSModel GPS;
        private BatteryModel Battery;
        private GroupModel Group;

        public GeneralModel getGeneral() {
            return General;
        }

        public void setGeneral(GeneralModel General) {
            this.General = General;
        }

        public CallsModel getCalls() {
            return Calls;
        }

        public void setCalls(CallsModel Calls) {
            this.Calls = Calls;
        }

        public ValNetworkModel getVal_Network() {
            return val_Network;
        }

        public void setVal_Network(ValNetworkModel val_Network) {
            this.val_Network = val_Network;
        }

        public GPSModel getGPS() {
            return GPS;
        }

        public void setGPS(GPSModel GPS) {
            this.GPS = GPS;
        }

        public BatteryModel getBattery() {
            return Battery;
        }

        public void setBattery(BatteryModel Battery) {
            this.Battery = Battery;
        }

        public GroupModel getGroup() {
            return Group;
        }

        public void setGroup(GroupModel Group) {
            this.Group = Group;
        }

        public static class GeneralModel {

            private int firm;
            private String mac;
            private String server_url;

            public int getFirm() {
                return firm;
            }

            public void setFirm(int firm) {
                this.firm = firm;
            }

            public String getMac() {
                return mac;
            }

            public void setMac(String mac) {
                this.mac = mac;
            }

            public String getServer_url() {
                return server_url;
            }

            public void setServer_url(String server_url) {
                this.server_url = server_url;
            }
        }

        public static class CallsModel {


            private String reg_status;
            private String call_status;

            public String getReg_status() {
                return reg_status;
            }

            public void setReg_status(String reg_status) {
                this.reg_status = reg_status;
            }

            public String getCall_status() {
                return call_status;
            }

            public void setCall_status(String call_status) {
                this.call_status = call_status;
            }
        }

        public static class ValNetworkModel {

            private String sim_card_status;
            private int signal_level;
            private String ipv4_status;
            private String ipv6_status;
            private String mcc;
            private String mnc;

            public String getSim_card_status() {
                return sim_card_status;
            }

            public void setSim_card_status(String sim_card_status) {
                this.sim_card_status = sim_card_status;
            }

            public int getSignal_level() {
                return signal_level;
            }

            public void setSignal_level(int signal_level) {
                this.signal_level = signal_level;
            }

            public String getIpv4_status() {
                return ipv4_status;
            }

            public void setIpv4_status(String ipv4_status) {
                this.ipv4_status = ipv4_status;
            }

            public String getIpv6_status() {
                return ipv6_status;
            }

            public void setIpv6_status(String ipv6_status) {
                this.ipv6_status = ipv6_status;
            }

            public String getMcc() {
                return mcc;
            }

            public void setMcc(String mcc) {
                this.mcc = mcc;
            }

            public String getMnc() {
                return mnc;
            }

            public void setMnc(String mnc) {
                this.mnc = mnc;
            }
        }

        public static class GPSModel {


            private String status;
            private int signal_level;
            private double current_latitude;
            private double current_longitude;

            public String getStatus() {
                return status;
            }

            public void setStatus(String status) {
                this.status = status;
            }

            public int getSignal_level() {
                return signal_level;
            }

            public void setSignal_level(int signal_level) {
                this.signal_level = signal_level;
            }

            public double getCurrent_latitude() {
                return current_latitude;
            }

            public void setCurrent_latitude(double current_latitude) {
                this.current_latitude = current_latitude;
            }

            public double getCurrent_longitude() {
                return current_longitude;
            }

            public void setCurrent_longitude(double current_longitude) {
                this.current_longitude = current_longitude;
            }
        }

        public static class BatteryModel {

            private double voltage;
            private int level;
            private String charger_status;

            public double getVoltage() {
                return voltage;
            }

            public void setVoltage(double voltage) {
                this.voltage = voltage;
            }

            public int getLevel() {
                return level;
            }

            public void setLevel(int level) {
                this.level = level;
            }

            public String getCharger_status() {
                return charger_status;
            }

            public void setCharger_status(String charger_status) {
                this.charger_status = charger_status;
            }
        }

        public static class GroupModel {

            private String status;
            private String uuid;

            public String getStatus() {
                return status;
            }

            public void setStatus(String status) {
                this.status = status;
            }

            public String getUuid() {
                return uuid;
            }

            public void setUuid(String uuid) {
                this.uuid = uuid;
            }
        }
    }
}
