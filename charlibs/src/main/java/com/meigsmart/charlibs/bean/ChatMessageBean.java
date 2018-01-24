package com.meigsmart.charlibs.bean;


public class ChatMessageBean {
    private int chatId;//数据库保存id
    private long id;//消息获取id
    private String UserId;//uuid
    private String UserName;
    private String UserHeadIcon;
    private String UserContent;
    private long time;
    private int type;
    private int messagetype;
    private float UserVoiceTime;
    private String UserVoicePath;
    private String UserVoiceUrl;
    private int sendState;
    private String imageUrl;
    private String imageIconUrl;
    private String imageLocal;
    private boolean isListened;//数据库 中 1为true 0为false
    private String groupId;//群组id
    private int deviceType;//设备类型  1为app  0为硬件设备
    private int position = -1;//位置

    public int getPosition() {
        return position;
    }

    public void setPosition(int position) {
        this.position = position;
    }

    public ChatMessageBean() {
    }

    public int getDeviceType() {
        return deviceType;
    }

    public void setDeviceType(int deviceType) {
        this.deviceType = deviceType;
    }

    public String getGroupId() {
        return groupId;
    }

    public void setGroupId(String groupId) {
        this.groupId = groupId;
    }

    public int getChatId() {
        return chatId;
    }

    public void setChatId(int chatId) {
        this.chatId = chatId;
    }

    public long getId() {
        return id;
    }

    public void setId(long id) {
        this.id = id;
    }

    public String getUserId() {
        return this.UserId;
    }

    public void setUserId(String UserId) {
        this.UserId = UserId;
    }

    public String getUserName() {
        return this.UserName;
    }

    public void setUserName(String UserName) {
        this.UserName = UserName;
    }

    public String getUserHeadIcon() {
        return this.UserHeadIcon;
    }

    public void setUserHeadIcon(String UserHeadIcon) {
        this.UserHeadIcon = UserHeadIcon;
    }

    public String getUserContent() {
        return this.UserContent;
    }

    public void setUserContent(String UserContent) {
        this.UserContent = UserContent;
    }

    public long getTime() {
        return this.time;
    }

    public void setTime(long time) {
        this.time = time;
    }

    public int getType() {
        return this.type;
    }

    public void setType(int type) {
        this.type = type;
    }

    public int getMessagetype() {
        return this.messagetype;
    }

    public void setMessagetype(int messagetype) {
        this.messagetype = messagetype;
    }

    public float getUserVoiceTime() {
        return this.UserVoiceTime;
    }

    public void setUserVoiceTime(float UserVoiceTime) {
        this.UserVoiceTime = UserVoiceTime;
    }

    public String getUserVoicePath() {
        return this.UserVoicePath;
    }

    public void setUserVoicePath(String UserVoicePath) {
        this.UserVoicePath = UserVoicePath;
    }

    public String getUserVoiceUrl() {
        return this.UserVoiceUrl;
    }

    public void setUserVoiceUrl(String UserVoiceUrl) {
        this.UserVoiceUrl = UserVoiceUrl;
    }

    public int getSendState() {
        return this.sendState;
    }

    public void setSendState(int sendState) {
        this.sendState = sendState;
    }

    public String getImageUrl() {
        return this.imageUrl;
    }

    public void setImageUrl(String imageUrl) {
        this.imageUrl = imageUrl;
    }

    public String getImageIconUrl() {
        return this.imageIconUrl;
    }

    public void setImageIconUrl(String imageIconUrl) {
        this.imageIconUrl = imageIconUrl;
    }

    public String getImageLocal() {
        return this.imageLocal;
    }

    public void setImageLocal(String imageLocal) {
        this.imageLocal = imageLocal;
    }

    public boolean getIsListened() {
        return this.isListened;
    }

    public void setIsListened(boolean isListened) {
        this.isListened = isListened;
    }


}
