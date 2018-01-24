package com.meigsmart.siplibs;

import android.os.Parcel;
import android.os.Parcelable;
import android.text.TextUtils;
import android.util.Log;

import org.pjsip.pjsua2.AccountConfig;
import org.pjsip.pjsua2.AuthCredInfo;
import org.pjsip.pjsua2.AuthCredInfoVector;
import org.pjsip.pjsua2.StringVector;
import org.pjsip.pjsua2.pj_qos_type;
import org.pjsip.pjsua2.pj_turn_tp_type;

import java.io.Serializable;

/**
 * Created by chenMeng on 2018/1/11.
 */
public class SipAccountData implements Parcelable {
    public static final String SIPS = "sips:";
    public static final String SIP = "sip:";
    private static final String STUN_SERVER = "120.24.77.212:3478";
    public static final String AUTH_TYPE_DIGEST = "digest";
    public static final String AUTH_TYPE_PLAIN = "plain";
    public static String head = SIP;

    private String username;
    private String password;
    private String domain;
    private long port = 5060;
    private boolean tcpTransport = false;
    private String authenticationType = AUTH_TYPE_DIGEST;
    private String protocol = "TCP";//UDP-> sip port udp     TLS-> sips port tcp  else(TCP) ->sip port tcp

    public SipAccountData() { }

    // This is used to regenerate the object.
    // All Parcelables must have a CREATOR that implements these two methods
    public static final Parcelable.Creator<SipAccountData> CREATOR =
            new Parcelable.Creator<SipAccountData>() {
                @Override
                public SipAccountData createFromParcel(final Parcel in) {
                    return new SipAccountData(in);
                }

                @Override
                public SipAccountData[] newArray(final int size) {
                    return new SipAccountData[size];
                }
            };

    @Override
    public void writeToParcel(Parcel parcel, int arg1) {
        parcel.writeString(username);
        parcel.writeString(password);
        parcel.writeString(domain);
        parcel.writeLong(port);
        parcel.writeByte((byte) (tcpTransport ? 1 : 0));
        parcel.writeString(authenticationType);
        parcel.writeString(protocol);
    }

    private SipAccountData(Parcel in) {
        username = in.readString();
        password = in.readString();
        domain = in.readString();
        port = in.readLong();
        tcpTransport = in.readByte() == 1;
        authenticationType = in.readString();
        protocol = in.readString();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    public String getUsername() {
        return username;
    }

    public SipAccountData setUsername(String username) {
        this.username = username;
        return this;
    }

    public String getPassword() {
        return password;
    }

    public SipAccountData setPassword(String password) {
        this.password = password;
        return this;
    }

    public String getDomain() {
        return domain;
    }

    public SipAccountData setDomain(String domain) {
        this.domain = domain;
        return this;
    }

    public long getPort() {
        return port;
    }

    public SipAccountData setPort(long port) {
        this.port = port;
        return this;
    }

    public boolean isTcpTransport() {
        return tcpTransport;
    }

    public SipAccountData setTcpTransport(boolean tcpTransport) {
        this.tcpTransport = tcpTransport;
        return this;
    }

    public String getProtocol() {
        return protocol;
    }

    public SipAccountData setProtocol(String protocol) {
        this.protocol = protocol;
        if (protocol.equals("TLS")){
            head = SIPS;
        }
        return this;
    }

    public String getIdUri() {
        StringBuilder proxyUri = new StringBuilder();
        String uri = head + username + "@" + domain ;
        proxyUri.append(uri);

        if (tcpTransport){
            proxyUri.append(";transport=tcp");
        }else{
            proxyUri.append(";transport=udp");
        }
        return uri;
    }

    public String getRegistrarUri() {
        return head + domain;
    }

    public String getProxyUri() {
        StringBuilder proxyUri = new StringBuilder();

        proxyUri.append(head).append(domain);

        if (tcpTransport) {
            proxyUri.append(";transport=tcp");
        }

        return proxyUri.toString();
    }

    public boolean isValid() {
        return ((username != null) && !username.isEmpty()
                && (password != null) && !password.isEmpty()
                && (domain != null) && !domain.isEmpty());
    }

    protected AccountConfig getAccountConfig() {
        String proxy = "";
        AccountConfig accountConfig = new AccountConfig();
        accountConfig.getMediaConfig().getTransportConfig().setQosType(pj_qos_type.PJ_QOS_TYPE_VOICE);
        accountConfig.setIdUri(getIdUri());
        accountConfig.getRegConfig().setRegistrarUri(getRegistrarUri());
        AuthCredInfoVector creds = accountConfig.getSipConfig().getAuthCreds();
        creds.clear();
        if (!TextUtils.isEmpty(username)) {
            creds.add(new AuthCredInfo(authenticationType, "*", getUsername(), 0, getPassword()));
        }

        StringVector proxies = accountConfig.getSipConfig().getProxies();
        proxies.clear();
        if (!TextUtils.isEmpty(proxy)) {
            proxies.add(proxy);
        }
        /* Enable ICE */

        accountConfig.getNatConfig().setIceEnabled(true);

        accountConfig.getNatConfig().setTurnEnabled(true);
        accountConfig.getNatConfig().setTurnConnType(pj_turn_tp_type.PJ_TURN_TP_UDP);
        accountConfig.getNatConfig().setTurnServer(STUN_SERVER);

        accountConfig.getRegConfig().setTimeoutSec(500);

        return accountConfig;
    }

    public String getAuthenticationType() {
        return authenticationType;
    }

    public SipAccountData setAuthenticationType(String authenticationType) {
        this.authenticationType = authenticationType;
        return this;
    }

    public String getRemoteUri(String sipNumber){
        StringBuilder proxyUri = new StringBuilder();
        proxyUri.append(head).append(sipNumber).append("@").append(domain);
        return proxyUri.toString();
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        if (o == null || getClass() != o.getClass()) return false;

        SipAccountData that = (SipAccountData) o;

        return getIdUri().equals(that.getIdUri());

    }

    @Override
    public int hashCode() {
        int result = username.hashCode();
        result = 31 * result + password.hashCode();
        result = 31 * result + domain.hashCode();
        result = 31 * result + (int) (port ^ (port >>> 32));
        result = 31 * result + (tcpTransport ? 1 : 0);
        result = 31 * result + protocol.hashCode();
        return result;
    }
}

