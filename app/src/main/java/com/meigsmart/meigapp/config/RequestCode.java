package com.meigsmart.meigapp.config;

/**
 * 常量配置
 * Created by chenMeng on 2017/9/1.
 */

public class RequestCode {

    //	string类型常量
    public static final String ERRORINFO = "服务器连接超时";//网络连接错误信息
    public static final String NONETWORK = "网络不可用";//网络无法连接
    public static final String QR_HEADER_ROOT = "RA:";//app 二维码生成命名头部统一
    public static final String QR_CHARACTER = ":";//二维码分隔符
    public static final String QR_DEVICE_ROOT = "e3mamot";//设备 二维码 命名 头部包含的内容
    public static final String QR_DEVICE_HEADER = "e3";
    public static final String FLAG_APP_UUID = "a_";//app uuid 头命名规则
    public static final String FLAG_DEVICE_UUID = "d_";//设备 uuid 头命名规则
    public static final String UPLOAD_APP_LOG = "appLog.log";
    public static final String UPLOAD_DEVICE_LOG = "deviceLog.log";
    public static final String UPLOAD_SAVE_FILE_NAME = "MeigsmartLog";

    public static final String USER_LANG = "cn";// 登录的 cn
    public static final String USER_BUNDLE_ID = "info.e3phone.iPhone";//登录 bundle——id
    public static final String APP_KEY = "2017fbd152bf43c796219ad494cc010d";
    public static final String APP_SECRET = "22f7f12b2348444aadb8aeb9ecd7a359";

    //blue set
    public static final String BLUE_SET_PASSWORD = "SetPassword";
    public static final String BLUE_GROUP_INVITATION = "GroupInvitation";
    public static final String BLUE_SET_SERVER_URL = "SetServerUrl";
    public static final String BLUE_SET_APN_SETTINGS = "SetApnSettings";
    public static final String BLUE_SET_THRESHOLDS = "SetThresholds";
    public static final String BLUE_SET_GEO_FENCE = "SetGeoFence";
    public static final String BLUE_SET_INTERVALS = "SetIntervals";
    public static final String BLUE_GET_DEVICE_STATUS = "GetDeviceStatus";
    public static final String BLUE_GET_LOG = "GetLog";
    public static final String BLUE_GET_APN_SETTINGS = "GetApnSettings";
    public static final String BLUE_GET_THRESHOLDS = "GetThresholds";
    public static final String BLUE_GET_GEO_FENCE = "GetGeoFence";
    public static final String BLUE_GET_INTERVALS = "GetIntervals";
    public static final String BLUE_UPGRADE_FIRMWARE = "UpgradeFirmware";//升级设备 蓝牙


    /**注册规则*/
    public static final String REGISTERTOOT = "密码不符合规范";

    //	int类型常量
    public static final int REGISTER = 0x001;//注册常量
    public static final int LOGIN = 0x002;//登录常量

    public static final int SEND_VOICE = 0x003;//发送语音消息
    public static final int SEND_TXT = 0x004;//发送文本
    public static final int UPDATE_MSG = 0x005;
    public static final int SHOW_HISTORY_TXT = 0x006;
    public static final int SHOW_HISTORY_VOICE = 0x007;

    public static final int REGISTER_STATUS_OK = 0x008;//sip注册状态
    public static final int REGISTER_STATUS_FAILED = 0x009;//sip注册状态
    public static final int SEND_REST_LOGIN = 0x010;//sip发送message notify
}
