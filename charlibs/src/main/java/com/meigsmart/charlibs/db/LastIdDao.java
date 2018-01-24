package com.meigsmart.charlibs.db;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;


import java.util.ArrayList;
import java.util.List;

/**
 * 保存聊天lastId表
 * Created by chenMeng on 2017/11/1.
 */

public class LastIdDao {

    private DBOpenHelper locationHelper;
    private SQLiteDatabase locationDb;
    public static String TABLE = "last";

    public LastIdDao(Context context) {
        locationHelper = new DBOpenHelper(context);
    }

    /**
     * 添加一条新记录
     *
     * @param model
     */
    public void addData(LastIdBean model) {
        locationDb = locationHelper.getReadableDatabase();

        ContentValues values = new ContentValues();
        values.put("lastId",model.getLastId());
        values.put("groupId",model.getGroupId());

        locationDb.insert(TABLE, null, values);
        locationDb.close();
    }

    /**
     * 更新数据
     * @param id
     */
    public void updateLastId(String id,long lastId) {
        locationDb = locationHelper.getReadableDatabase();
        String sql = "update "+TABLE+" set lastId=" + lastId +" where id="+id;
        if (locationDb.isOpen())
            locationDb.execSQL(sql);
        locationDb.close();
    }

    /**
     * 查询所有的记录  groupId
     *
     * @return
     */
    public LastIdBean getData(String group_uuid) {
        LastIdBean bean = new LastIdBean();

        String sql = "select * from "+TABLE+" where groupId=?";
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery(sql, new String[]{group_uuid});

        while (cursor.moveToNext()) {
            int id = cursor.getInt(cursor.getColumnIndex("id"));
            long lastId = cursor.getLong(cursor.getColumnIndex("lastId"));
            String groupId = cursor.getString(cursor.getColumnIndex("groupId"));

            bean.setId(String.valueOf(id));
            bean.setLastId(String.valueOf(lastId));
            bean.setGroupId(groupId);

        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return bean;
    }

}
