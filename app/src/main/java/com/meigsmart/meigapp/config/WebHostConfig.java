package com.meigsmart.meigapp.config;

/**
 * 网络ip 配置
 * Created by chenMeng on 2017/9/4.
 */

public class WebHostConfig {

//    public static final String HOST_ADDRESS = "http://192.168.0.83:8080/";//内网
//    public static final String HOST_ADDRESS = "http://116.247.69.94:7000/";//外网
    public static final String HOST_ADDRESS = "http://116.62.205.204:7000/meiglink/";//aliyu网
//    public static final String HOST_ADDRESS = "https://www.meigelink.com/meiglink/";

    public static final String HOST_NAME = HOST_ADDRESS + "api/v1/";

//    public static final String HOST_NAME = HOST_ADDRESS + "meig-demo-admin/api/v1/";

    public static String getHostAddress() {
        return HOST_ADDRESS;
    }

    public static String getHostName() {
        return HOST_NAME;
    }

}
