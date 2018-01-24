package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2017/11/28.
 */

public interface BluetoothConnListener {
    void onSuccessConnect();//连接成功
    void onCancelConnect();//链接中断
    void onCommunication();//数据通信
    void onReceiveData(String data);//接受到数据
    void onStopConnect();//停止连接
}
