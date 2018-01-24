package com.meigsmart.meigapp.sip;

/**
 * Created by chenMeng on 2017/12/11.
 */

public enum MessageType {
    SYSTEM_MESSAGE("system_message"),//系统消息
    TEXT_MESSAGE("text_message"), // 文本消息
    VOICE_MESSAGE("voice_message"), // 语音消息
    JOIN_GROUP("join_group"), // 加入组
    ACCEPTED_INVITE("accepted_invite"), // 同意加组
    NEW_MEMBER("new_member"), // 新成员
    EVENT("event"), // 事件
    NEW_FIRMWARE("​new_firmware"), //
    SEND_PASSWORD("send_password"), //
    PSTN_PUSH_CALL("​pstn_push_call"), //
    MEMBER_PUSH_CALL("​member_push_call"), //
    CANCEL_PUSH_CALL("cancel_​push_call"), //
    COMMAND("command"), // 命令
    CALLING("calling"); // 电话

    MessageType(String name) {
        this.name = name;
    }

    private String name;

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

}