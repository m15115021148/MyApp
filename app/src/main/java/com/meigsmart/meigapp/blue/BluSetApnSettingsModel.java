package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2017/11/29.
 */

public class BluSetApnSettingsModel extends BluBaseModel {
    private ApnSettings ApnProfile;

    public ApnSettings getApnProfile() {
        return ApnProfile;
    }

    public void setApnProfile(ApnSettings apnProfile) {
        ApnProfile = apnProfile;
    }

    public static class ApnSettings{
        private String profile_name;
        private String apn;
        private String auth_type;
        private String user_name;
        private String password;
        private String protocol;
        private String ipv4_primary_dns;
        private String ipv4_secondary_dns;
        private String ipv6_primary_dns;
        private String ipv6_secondary_dns;

        public String getProfile_name() {
            return profile_name;
        }

        public void setProfile_name(String profile_name) {
            this.profile_name = profile_name;
        }

        public String getApn() {
            return apn;
        }

        public void setApn(String apn) {
            this.apn = apn;
        }

        public String getAuth_type() {
            return auth_type;
        }

        public void setAuth_type(String auth_type) {
            this.auth_type = auth_type;
        }

        public String getUser_name() {
            return user_name;
        }

        public void setUser_name(String user_name) {
            this.user_name = user_name;
        }

        public String getPassword() {
            return password;
        }

        public void setPassword(String password) {
            this.password = password;
        }

        public String getProtocol() {
            return protocol;
        }

        public void setProtocol(String protocol) {
            this.protocol = protocol;
        }

        public String getIpv4_primary_dns() {
            return ipv4_primary_dns;
        }

        public void setIpv4_primary_dns(String ipv4_primary_dns) {
            this.ipv4_primary_dns = ipv4_primary_dns;
        }

        public String getIpv4_secondary_dns() {
            return ipv4_secondary_dns;
        }

        public void setIpv4_secondary_dns(String ipv4_secondary_dns) {
            this.ipv4_secondary_dns = ipv4_secondary_dns;
        }

        public String getIpv6_primary_dns() {
            return ipv6_primary_dns;
        }

        public void setIpv6_primary_dns(String ipv6_primary_dns) {
            this.ipv6_primary_dns = ipv6_primary_dns;
        }

        public String getIpv6_secondary_dns() {
            return ipv6_secondary_dns;
        }

        public void setIpv6_secondary_dns(String ipv6_secondary_dns) {
            this.ipv6_secondary_dns = ipv6_secondary_dns;
        }
    }
}
