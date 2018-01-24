package com.meigsmart.charlibs.db;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by chenMeng on 2017/11/29.
 */

public class BluetoothDao {
    private DBOpenHelper locationHelper;
    private SQLiteDatabase locationDb;
    public static String TABLE = "bluetooth";

    public BluetoothDao(Context context) {
        locationHelper = new DBOpenHelper(context);
    }

    /**
     * 添加一条新记录
     *
     * @param model
     */
    public void addData(BluetoothBean model) {
        locationDb = locationHelper.getReadableDatabase();

        ContentValues values = new ContentValues();
        values.put("blueName",model.getBlueName());
        values.put("serialNum",model.getSerialNum());
        values.put("isFirstSetPsw",model.getIsFirstSetPsw());
        values.put("password",model.getPassword());

        locationDb.insert(TABLE, null, values);
        locationDb.close();
    }

    /**
     * 更新数据
     * @param id
     */
    public void updatePsw(String id,String password) {
        locationDb = locationHelper.getReadableDatabase();
        String sql = "update "+TABLE+" set password='" + password +"' where id="+id;
        if (locationDb.isOpen())
            locationDb.execSQL(sql);
        locationDb.close();
    }

    /**
     * 查询所有的记录  serialNum
     *
     * @return
     */
    public BluetoothBean getData(String serialNum) {
        BluetoothBean bean = new BluetoothBean();

        String sql = "select * from "+TABLE+" where serialNum=?";
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery(sql, new String[]{serialNum});

        while (cursor.moveToNext()) {
            int id = cursor.getInt(cursor.getColumnIndex("id"));
            String blueName = cursor.getString(cursor.getColumnIndex("blueName"));
            String isFirstSetPsw = cursor.getString(cursor.getColumnIndex("isFirstSetPsw"));
            String password = cursor.getString(cursor.getColumnIndex("password"));

            bean.setId(String.valueOf(id));
            bean.setBlueName(blueName);
            bean.setIsFirstSetPsw(isFirstSetPsw);
            bean.setPassword(password);
            bean.setSerialNum(serialNum);
        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return bean;
    }

    /**
     * 查询所有的记录  serialNum
     *
     * @return
     */
    public List<BluetoothBean> getAllData() {
        List<BluetoothBean> list = new ArrayList<>();


        String sql = "select * from "+TABLE ;
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery(sql, null);

        while (cursor.moveToNext()) {
            int id = cursor.getInt(cursor.getColumnIndex("id"));
            String blueName = cursor.getString(cursor.getColumnIndex("blueName"));
            String isFirstSetPsw = cursor.getString(cursor.getColumnIndex("isFirstSetPsw"));
            String password = cursor.getString(cursor.getColumnIndex("password"));
            String serialNum = cursor.getString(cursor.getColumnIndex("serialNum"));

            BluetoothBean bean = new BluetoothBean();
            bean.setId(String.valueOf(id));
            bean.setBlueName(blueName);
            bean.setIsFirstSetPsw(isFirstSetPsw);
            bean.setPassword(password);
            bean.setSerialNum(serialNum);
            list.add(bean);
        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return list;
    }
}
