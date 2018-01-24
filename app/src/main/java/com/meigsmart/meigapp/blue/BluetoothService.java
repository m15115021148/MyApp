package com.meigsmart.meigapp.blue;

import android.app.Activity;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;
import android.os.SystemClock;
import android.support.annotation.RequiresApi;
import android.text.TextUtils;
import android.util.Base64;

import com.meigsmart.meigapp.log.LogUtil;

import java.security.Key;
import java.security.spec.AlgorithmParameterSpec;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.Vector;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;


/**
 * Created by chenMeng on 2017/11/28.
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BluetoothService extends Service {
    private final static String TAG = BluetoothService.class.getSimpleName();

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothGatt mBluetoothGatt;
    private BluetoothLeScanner mBlueScanner = null;
    private boolean isStartBlu = false;//是否在扫描

    public BluetoothGattCharacteristic mNotifyCharacteristic;

    private static UUID m_taget_uuid_service = UUID.fromString("5b9face6-f758-4c1c-81ce-31f4089d9716");

    private static UUID m_taget_uuid_attr_reqData = UUID.fromString("af98a91b-32d6-4b05-9e90-43037294ef26");
    private static UUID m_taget_uuid_attr_reqStart = UUID.fromString("b1af41c8-3b8b-4787-a900-be084b88998c");
    private static UUID m_taget_uuid_attr_reqEnd = UUID.fromString("528716d5-99dc-44e9-9edb-4092bfc9b183");

    private static UUID m_taget_uuid_attr_rspData = UUID.fromString("a0d52b37-b645-4e9a-98af-5ced96280393");
    private static UUID m_taget_uuid_attr_rspStart = UUID.fromString("e56763de-d2c6-477e-a059-c72c4d06c4de");
    private static UUID m_taget_uuid_attr_rspInc = UUID.fromString("983b59d7-4c81-4933-98d8-0903c5dfd1a8");

    private static final String PRESHARED_KEY_REQUEST = "D74P48KzHxv4oYF50+KRr9OcbwSSPYbuheOC8ejQjAE=";
    private static final String PRESHARED_KEY_RESPONSE = "ouP8j5AnGpJvvNTBgJvc6r5RVxT+8gqbTW8iIwmBlk8=";
    private static byte[] m_encrypt_key = Base64.decode(PRESHARED_KEY_REQUEST, Base64.DEFAULT);
    private static byte[] m_decrypt_key = Base64.decode(PRESHARED_KEY_RESPONSE, Base64.DEFAULT);

    private final boolean DONT_WAIT_RESP = true;
    private final boolean RECV_RESP_ONLY = false;
    private int m_bt_gatt_retry_count = 0;
    private int m_bt_gatt_write_offset = 0;
    private final int TEST_CHUNK_SZ = 10;
    private static byte[] m_dummy_data = new byte[1];
    private static byte[] m_test_data = null; //new byte[TEST_DATA_SZ];
    private byte[] m_resp_data = null;
    private int m_bt_chunk_sz = TEST_CHUNK_SZ;
    private Vector<BluetoothGatt> m_bt_connect_list = new Vector<BluetoothGatt>();
    private BluetoothConnListener mListener;
    private String blueName = "";

    public void setmListener(BluetoothConnListener mListener) {
        this.mListener = mListener;
    }

    public void WriteValue(String strValue) {
        mNotifyCharacteristic.setValue(strValue.getBytes());
        mBluetoothGatt.writeCharacteristic(mNotifyCharacteristic);
    }

    /**
     * Implements callback methods for GATT events that the app cares about.  For example,
     * connection change and services discovered.
     */
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {

        private void do_retry(BluetoothGatt gatt) {
//            if (mListener!=null) mListener.onStopConnect();

            if (gatt == null) {
                return;
            }

            m_bt_gatt_retry_count++;
            LogUtil.w(TAG, "m_bt_gatt_retry_count = " + m_bt_gatt_retry_count);
            gatt.disconnect();

            if (m_bt_gatt_retry_count > 5) {
                LogUtil.w(TAG, "all retry failed, no more !");
                return;
            }

            SystemClock.sleep(200);
            gatt.connect();
        }

        private BluetoothGattCharacteristic getAttr(BluetoothGatt gatt, UUID uuid) {
            if (gatt == null) {
                return null;
            }

            BluetoothGattService targetservice = gatt.getService(m_taget_uuid_service);
            if (targetservice == null) {
                LogUtil.w(TAG, "no target service found in device:" + gatt.getDevice().getAddress());
                do_retry(gatt);
                return null;
            }
            return targetservice.getCharacteristic(uuid);
        }

        private boolean writeAttr(BluetoothGatt gatt, UUID uuid, byte[] val) {
            BluetoothGattCharacteristic attr = getAttr(gatt, uuid);

            if (attr == null) {
                LogUtil.w(TAG, "no found target attr :" + uuid);
                return false;
            }

            attr.setValue(val);
            if (!gatt.writeCharacteristic(attr)) {
                LogUtil.w(TAG, " writeAttr failed !");
                return false;
            }
            return true;
        }

        private boolean readAttr(BluetoothGatt gatt, UUID uuid) {
            BluetoothGattCharacteristic attr = getAttr(gatt, uuid);

            if (attr == null) {
                LogUtil.w(TAG, "no found target attr :" + uuid);
                return false;
            }

            if (!gatt.readCharacteristic(attr)) {
                LogUtil.e(TAG, " readAttr failed !");
                return false;
            }
            return true;
        }

        private boolean writeData(BluetoothGatt gatt) {
            int step_size = m_bt_chunk_sz;
            LogUtil.w("result","m_rest_data.length:"+m_test_data.length);
            LogUtil.w("result","m_bt_gatt_write_offset:"+m_bt_gatt_write_offset);
            if (m_test_data.length - m_bt_gatt_write_offset < m_bt_chunk_sz) {
                step_size = m_test_data.length - m_bt_gatt_write_offset;
            }

            byte[] data = new byte[step_size];
            System.arraycopy(m_test_data, m_bt_gatt_write_offset, data, 0, step_size);

            LogUtil.w(TAG, "writeData offset=" + m_bt_gatt_write_offset + " size=" + step_size);
            LogUtil.w(TAG, "data: " + byte2HexStr(data));

            if (!writeAttr(gatt, m_taget_uuid_attr_reqData, data)) {
                return false;
            }
            m_bt_gatt_write_offset += step_size;
            return true;
        }

        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            LogUtil.w(TAG, " onConnectionStateChange");
            switch (newState) {
                case BluetoothProfile.STATE_CONNECTED:
                    LogUtil.w(TAG, "STATE_CONNECTED:" + gatt.getDevice().getAddress());
                    gatt.discoverServices();
                    if (mListener!=null) mListener.onSuccessConnect();
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    LogUtil.w(TAG, "STATE_DISCONNECTED:" + gatt.getDevice().getAddress());
                    if (mListener!=null) mListener.onCancelConnect();
                    break;
                case BluetoothProfile.STATE_CONNECTING:
                    LogUtil.w(TAG, "STATE_CONNECTING:" + gatt.getDevice().getAddress());
                    if (mListener!=null) mListener.onCommunication();
                    break;
                case BluetoothProfile.STATE_DISCONNECTING:
                    LogUtil.w(TAG, "STATE_DISCONNECTING:" + gatt.getDevice().getAddress());
                    if (mListener!=null) mListener.onCancelConnect();
                    break;
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            LogUtil.w(TAG, " + gatt:" + gatt);
            BluetoothGattService targetservice = gatt.getService(m_taget_uuid_service);
            if (targetservice == null) {
                LogUtil.e(TAG, "no target service found in device:" + gatt.getDevice().getAddress());
                do_retry(gatt);
                return;
            }

            LogUtil.w(TAG, "do test on service ===>");
            LogUtil.w(TAG, " write reqStart");
            if (RECV_RESP_ONLY) {
                if (!readAttr(gatt, m_taget_uuid_attr_rspStart)) {
                    LogUtil.e(TAG, " read rspStart failed !");
                    do_retry(gatt);
                }
            } else {
                if (!writeAttr(gatt, m_taget_uuid_attr_reqStart, m_dummy_data)) {
                    LogUtil.e(TAG, " write reqStart failed !");
                    do_retry(gatt);
                }
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (characteristic.getUuid().equals(m_taget_uuid_attr_rspStart)) {
                    byte[] val = characteristic.getValue();
                    if (val.length != 5 || val[0] != 'r' || val[1] != 'e' || val[2] != 'a' || val[3] != 'd' || val[4] != 'y') {
                        LogUtil.w(TAG, "read rspStart NOT READY");
                        if (mListener!=null)mListener.onStopConnect();
                        if (!DONT_WAIT_RESP) {
                            SystemClock.sleep(500);
                            if (!readAttr(gatt, m_taget_uuid_attr_rspStart)) {
                                LogUtil.e(TAG, "read Response Start Failed");
                                do_retry(gatt);
                            }
                        }
                    } else {
                        LogUtil.w(TAG, "read rspData 1");
                        m_resp_data = null;
                        if (!readAttr(gatt, m_taget_uuid_attr_rspData)) {
                            LogUtil.e(TAG, "read Response Data Failed");
                            do_retry(gatt);
                        }
                    }
                }

                if (characteristic.getUuid().equals(m_taget_uuid_attr_rspData)) {
                    byte[] val = characteristic.getValue();
                    if (val.length <= 0) {
                        LogUtil.w(TAG, "DATA_END: " + byte2HexStr(m_resp_data));
                        LogUtil.w(TAG,"m_rsp_data:"+m_resp_data);
                        if (m_resp_data != null && m_resp_data.length > 0) {
                            try {
                                byte[] iv = Arrays.copyOf(m_resp_data, 16);
                                byte[] data = Arrays.copyOfRange(m_resp_data, 16, m_resp_data.length);
                                byte[] resp = decrypt(data, iv);
                                String s = new String(resp, "UTF-8");
                                LogUtil.v(TAG, "Response:" + s);
                                if (mListener!=null) mListener.onReceiveData(s);
                            } catch (Exception e) {
                                if (mListener!=null) mListener.onCancelConnect();
                                e.printStackTrace();
                            }
                        }
                    } else {
                        LogUtil.w(TAG, "DATA_READ:" + byte2HexStr(val));
                        LogUtil.w(TAG, "write rspInc");
                        SystemClock.sleep(1);
                        m_resp_data = mergeByte(m_resp_data, val);
                        if (!writeAttr(gatt, m_taget_uuid_attr_rspInc, m_dummy_data)) {
                            LogUtil.e(TAG, "write response increment failed !");
                            do_retry(gatt);
                        }
                    }
                }

            }
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            LogUtil.w(TAG, "onCharacteristicChanged");
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt, BluetoothGattCharacteristic characteristic,
                                          int status) {
            LogUtil.w(TAG, "OnCharacteristicWrite");
            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (characteristic.getUuid().equals(m_taget_uuid_attr_reqStart)) {
                    m_bt_gatt_write_offset = 0;
                    if (!writeData(gatt)) {
                        LogUtil.e(TAG, "write request Data failed !");
                        do_retry(gatt);
                    }
                }

                if (characteristic.getUuid().equals(m_taget_uuid_attr_reqData)) {
                    if (m_bt_gatt_write_offset < m_test_data.length) {
                        if (!writeData(gatt)) {
                            LogUtil.e(TAG, "write request Data failed !");
                            do_retry(gatt);
                        }
                    } else {
                        LogUtil.w(TAG, "write reqEnd");
                        if (!writeAttr(gatt, m_taget_uuid_attr_reqEnd, m_dummy_data)) {
                            LogUtil.e(TAG, "write request End failed !");
                            do_retry(gatt);
                        }
                    }
                }

                if (characteristic.getUuid().equals(m_taget_uuid_attr_reqEnd)) {
                    LogUtil.w(TAG, "readtart");
                    if (DONT_WAIT_RESP) {
                        SystemClock.sleep(1000);
                    }
                    if (!readAttr(gatt, m_taget_uuid_attr_rspStart)) {
                        LogUtil.e(TAG, "read request Start failed !");
                        do_retry(gatt);
                    }
                }

                if (characteristic.getUuid().equals(m_taget_uuid_attr_rspInc)) {
                    LogUtil.w(TAG, "read rspData n");
                    if (!readAttr(gatt, m_taget_uuid_attr_rspData)) {
                        LogUtil.e(TAG, "read request Data failed !");
                        do_retry(gatt);
                    }
                }
            } else {
                LogUtil.e(TAG, "write attr failed");
                do_retry(gatt);
            }
        }

        @Override
        public void onDescriptorRead(BluetoothGatt gatt,
                                     BluetoothGattDescriptor bd,
                                     int status) {
        }

        @Override
        public void onDescriptorWrite(BluetoothGatt gatt,
                                      BluetoothGattDescriptor bd,
                                      int status) {
        }

        @Override
        public void onReadRemoteRssi(BluetoothGatt gatt, int a, int b) {
        }

        @Override
        public void onReliableWriteCompleted(BluetoothGatt gatt, int a) {
        }

    };

    public boolean startScanBlue(Activity activity,String test_json_req,String name) {
        LogUtil.w(TAG, "================ startScanBlue =================");
        if(!initialize(activity,test_json_req))return false;
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null)return false;

        if (!mBluetoothAdapter.isEnabled()) {
            mBluetoothAdapter.enable();
            return false;
        }

        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.KITKAT) {
            mBlueScanner = mBluetoothAdapter.getBluetoothLeScanner();
            if (mBlueScanner == null)return false;
            blueName = name;
            isStartBlu = true;
            mBlueScanner.startScan(mScanCallBack);
        }
        return true;
    }

    public void stopScanBlu() {
        LogUtil.w(TAG, "================ stopScanBlu =================");
        if (isStartBlu)mBlueScanner.stopScan(mScanCallBack);
        disconnect();
        close();
        isStartBlu = false;
    }

    private ScanCallback mScanCallBack = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            if (result.getRssi() < -90 ) {
                return;
            }

            if (result.getDevice() != null) {
                LogUtil.w(TAG, " scanner deviceCore:  " + result.getDevice());
                LogUtil.w(TAG, " scanner deviceName:  " + result.getDevice().getName());
            }
            if (!TextUtils.isEmpty(blueName) && blueName.equals(result.getDevice().getName())) {
                connect(result.getDevice());
            }
        }
    };

    public class LocalBinder extends Binder {
        public BluetoothService getService() {
            return BluetoothService.this;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        close();
        return super.onUnbind(intent);
    }

    private final IBinder mBinder = new LocalBinder();

    public boolean initialize(Activity activity,String req) {
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) activity.getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                LogUtil.e(TAG, "Unable to initialize BluetoothManager.");
                return false;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            LogUtil.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return false;
        }

        try {
            m_test_data = encrypt(req.getBytes("UTF-8"));
        } catch (Exception e) {
            e.printStackTrace();
        }
        m_dummy_data[0] = (byte) 1;
        return true;
    }

    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     * is reported asynchronously through the
     * {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     * callback.
     */
    public boolean connect(BluetoothDevice device) {
        if (mBluetoothAdapter == null) {
            LogUtil.e(TAG, "BluetoothAdapter not initialized or unspecified address.");
            return false;
        }

//        BluetoothDevice device = mBluetoothAdapter.getRemoteDevice(address);
        if (device == null) {
            LogUtil.e(TAG, "Device not found.  Unable to connect.");
            return false;
        }
        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        if (mBluetoothGatt != null) {
            mBluetoothGatt.close();
            mBluetoothGatt = null;
        }
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);
        m_bt_connect_list.add(mBluetoothGatt);

        LogUtil.w(TAG, "Trying to create a new connection.");
        return true;
    }

    /**
     * Disconnects an existing connection or cancel a pending connection. The disconnection result
     * is reported asynchronously through the
     * {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     * callback.
     */
    public void disconnect() {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            LogUtil.e(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.disconnect();
    }

    /**
     * After using a given BLE device, the app must call this method to ensure resources are
     * released properly.
     */
    public void close() {
        if (mBluetoothGatt == null) {
            return;
        }
        mBluetoothGatt.close();
        mBluetoothGatt = null;
    }

    /**
     * Request a read on a given {@code BluetoothGattCharacteristic}. The read result is reported
     * asynchronously through the {@code BluetoothGattCallback#onCharacteristicRead(android.bluetooth.BluetoothGatt, android.bluetooth.BluetoothGattCharacteristic, int)}
     * callback.
     *
     * @param characteristic The characteristic to read from.
     */
    public void readCharacteristic(BluetoothGattCharacteristic characteristic) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            LogUtil.e(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.readCharacteristic(characteristic);
    }

    /**
     * Enables or disables notification on a give characteristic.
     *
     * @param characteristic Characteristic to act on.
     * @param enabled        If true, enable notification.  False otherwise.
     */
    public void setCharacteristicNotification(BluetoothGattCharacteristic characteristic,
                                              boolean enabled) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            LogUtil.e(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.setCharacteristicNotification(characteristic, enabled);
    }

    /**
     * Retrieves a list of supported GATT services on the connected device. This should be
     * invoked only after {@code BluetoothGatt#discoverServices()} completes successfully.
     *
     * @return A {@code List} of supported services.
     */
    public List<BluetoothGattService> getSupportedGattServices() {
        if (mBluetoothGatt == null) return null;

        return mBluetoothGatt.getServices();
    }

    public String byte2HexStr(byte[] bs) {
        if (bs == null) return "";

        char[] chars = "0123456789ABCDEF".toCharArray();
        StringBuilder sb = new StringBuilder("");

        int bit;

        for (int i = 0; i < bs.length; i++) {
            bit = (bs[i] & 0x0f0) >> 4;
            sb.append(chars[bit]);
            bit = bs[i] & 0x0f;
            sb.append(chars[bit]);
            sb.append(' ');
        }
        return sb.toString().trim();
    }

    public static byte[] mergeByte(byte[] byte_1, byte[] byte_2) {
        if (byte_1 == null) {
            return byte_2;
        } else if (byte_2 == null) {
            return byte_1;
        } else {
            byte[] byte_3 = new byte[byte_1.length + byte_2.length];
            System.arraycopy(byte_1, 0, byte_3, 0, byte_1.length);
            System.arraycopy(byte_2, 0, byte_3, byte_1.length, byte_2.length);
            return byte_3;
        }
    }

    private static byte[] encrypt(byte[] data) throws Exception {
        return encrypt(data, null, null);
    }

    private static byte[] encrypt(byte[] data, byte[] key) throws Exception {
        return encrypt(data, key, null);
    }

    private static byte[] encrypt(byte[] data, byte[] key, byte[] iv) throws Exception {
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7Padding");

        Key key_spec = null;
        if (key == null) {
            key = m_encrypt_key;
        }
        key_spec = new SecretKeySpec(key, "AES");

        if (iv == null) {
            cipher.init(Cipher.ENCRYPT_MODE, key_spec);
        } else {
            cipher.init(Cipher.ENCRYPT_MODE, key_spec, new IvParameterSpec(iv));
        }
        byte[] encrypted = cipher.doFinal(data);
        return mergeByte(cipher.getIV(), encrypted);
    }

    private static byte[] decrypt(byte[] encrypted_data, byte[] iv) throws Exception {
        return decrypt(encrypted_data, null, iv);
    }

    private static byte[] decrypt(byte[] encrypted_data, byte[] key, byte[] iv) throws Exception {
        Key key_spec = null;
        if (key == null) {
            key = m_decrypt_key;
        }
        key_spec = new SecretKeySpec(key, "AES");

        AlgorithmParameterSpec iv_spec = new IvParameterSpec(iv);
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS7Padding");
        cipher.init(Cipher.DECRYPT_MODE, key_spec, iv_spec);
        return cipher.doFinal(encrypted_data);
    }
}
