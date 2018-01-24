package com.meigsmart.charlibs.db;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import com.meigsmart.charlibs.bean.ChatMessageBean;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by chenMeng on 2017/9/16.
 */

public class CharDataDao {
    private DBOpenHelper locationHelper;
    private SQLiteDatabase locationDb;
    private String TABLE = "char";

    public CharDataDao(Context context) {
        locationHelper = new DBOpenHelper(context);
    }

    /**
     * 添加一条新记录
     *
     * @param model
     */
    public void addNewData(ChatMessageBean model) {
        locationDb = locationHelper.getReadableDatabase();

        ContentValues values = new ContentValues();
        values.put("id",model.getId());
        values.put("UserName", model.getUserName());
        values.put("UserContent",  model.getUserContent());
        values.put("time",  model.getTime());
        values.put("type",  model.getType());
        values.put("messagetype",  model.getMessagetype());
        values.put("UserVoiceTime",model.getUserVoiceTime());
        values.put("UserVoicePath",  model.getUserVoicePath());
        values.put("sendState",model.getSendState());
        values.put("groupId",model.getGroupId());
        values.put("isListened",model.getIsListened()?1:0);
        values.put("deviceType",model.getDeviceType());
        values.put("clientId",model.getUserId());

        locationDb.insert(TABLE, null, values);
        locationDb.close();
    }

    /**
     * 查询所有的记录 按时间排序
     *
     * @return
     */
    public List<ChatMessageBean> getAllDatas() {
        List<ChatMessageBean> dataList = new ArrayList<>();
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery("select * from char order by time asc", null);

        while (cursor.moveToNext()) {
            int chatId = cursor.getInt(cursor.getColumnIndex("chatId"));
            long id = cursor.getLong(cursor.getColumnIndex("id"));
            String UserName = cursor.getString(cursor.getColumnIndex("UserName"));
            String UserContent = cursor.getString(cursor.getColumnIndex("UserContent"));
            long time = cursor.getLong(cursor.getColumnIndex("time"));
            int type = cursor.getInt(cursor.getColumnIndex("type"));
            int messagetype = cursor.getInt(cursor.getColumnIndex("messagetype"));
            float UserVoiceTime = cursor.getFloat(cursor.getColumnIndex("UserVoiceTime"));
            String UserVoicePath = cursor.getString(cursor.getColumnIndex("UserVoicePath"));
            int sendState = cursor.getInt(cursor.getColumnIndex("sendState"));
            String groupId = cursor.getString(cursor.getColumnIndex("groupId"));
            int isListened = cursor.getInt(cursor.getColumnIndex("isListened"));
            int deviceType = cursor.getInt(cursor.getColumnIndex("deviceType"));
            String clientId = cursor.getString(cursor.getColumnIndex("clientId"));
            ChatMessageBean entity = new ChatMessageBean();
            entity.setChatId(chatId);
            entity.setId(id);
            entity.setUserName(UserName);
            entity.setUserContent(UserContent);
            entity.setTime(time);
            entity.setType(type);
            entity.setMessagetype(messagetype);
            entity.setUserVoiceTime(UserVoiceTime);
            entity.setUserVoicePath(UserVoicePath);
            entity.setSendState(sendState);
            entity.setGroupId(groupId);
            entity.setIsListened(isListened == 1);
            entity.setDeviceType(deviceType);
            entity.setUserId(clientId);
            dataList.add(entity);
        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return dataList;
    }

    /**
     * 查询所有的记录 按时间排序 groupId
     *
     * @return
     */
    public List<ChatMessageBean> getAllData(String group_uuid) {
        List<ChatMessageBean> dataList = new ArrayList<>();

        String sql = "select * from char where groupId=? order by time asc";
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery(sql, new String[]{group_uuid});

        while (cursor.moveToNext()) {
            int chatId = cursor.getInt(cursor.getColumnIndex("chatId"));
            long id = cursor.getLong(cursor.getColumnIndex("id"));
            String UserName = cursor.getString(cursor.getColumnIndex("UserName"));
            String UserContent = cursor.getString(cursor.getColumnIndex("UserContent"));
            long time = cursor.getLong(cursor.getColumnIndex("time"));
            int type = cursor.getInt(cursor.getColumnIndex("type"));
            int messagetype = cursor.getInt(cursor.getColumnIndex("messagetype"));
            float UserVoiceTime = cursor.getFloat(cursor.getColumnIndex("UserVoiceTime"));
            String UserVoicePath = cursor.getString(cursor.getColumnIndex("UserVoicePath"));
            int sendState = cursor.getInt(cursor.getColumnIndex("sendState"));
            String groupId = cursor.getString(cursor.getColumnIndex("groupId"));
            int isListened = cursor.getInt(cursor.getColumnIndex("isListened"));
            int deviceType = cursor.getInt(cursor.getColumnIndex("deviceType"));
            String clientId = cursor.getString(cursor.getColumnIndex("clientId"));
            ChatMessageBean entity = new ChatMessageBean();
            entity.setChatId(chatId);
            entity.setId(id);
            entity.setUserName(UserName);
            entity.setUserContent(UserContent);
            entity.setTime(time);
            entity.setType(type);
            entity.setMessagetype(messagetype);
            entity.setUserVoiceTime(UserVoiceTime);
            entity.setUserVoicePath(UserVoicePath);
            entity.setSendState(sendState);
            entity.setGroupId(groupId);
            entity.setIsListened(isListened == 1);
            entity.setDeviceType(deviceType);
            entity.setUserId(clientId);
            dataList.add(entity);
        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return dataList;
    }

    /**
     * 删除指定记录
     *
     * @param lastId
     */
    public void deleteCurData(String lastId) {
        locationDb = locationHelper.getReadableDatabase();
        if (locationDb.isOpen())
            locationDb.delete(TABLE, "id=?", new String[]{lastId});
        Log.e("result","delete:"+"删除成功");
        locationDb.close();
    }

    /**
     * 删除指定群组聊天 记录
     *
     * @param groupId
     */
    public void deleteGroupChatData(String groupId) {
        locationDb = locationHelper.getReadableDatabase();
        if (locationDb.isOpen())
            locationDb.delete(TABLE, "groupId=?", new String[]{groupId});
        Log.e("result","delete:"+"删除成功");
        locationDb.close();
    }

    /**
     * 更新数据  是否 听过
     * @param isListened
     */
    public void updateDataListened(String id,boolean isListened) {
        locationDb = locationHelper.getReadableDatabase();
        String sql = "update char set isListened =" + (isListened?1:0) +" where id="+id;
        if (locationDb.isOpen())
            locationDb.execSQL(sql);
        locationDb.close();
    }

    /**
     * 查询指定的记录 分页查询
     * @param first
     * @param end
     * @return
     */
    public List<ChatMessageBean> getCurrPageData(int first,int end){
        List<ChatMessageBean> list = new ArrayList<>();
        String sql = "select * from char order by chatId asc limit ?,?";
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery(sql, new String[]{String.valueOf(first), String.valueOf(end)} );
        while (cursor.moveToNext()) {
            int chatId = cursor.getInt(cursor.getColumnIndex("chatId"));
            long id = cursor.getLong(cursor.getColumnIndex("id"));
            String UserName = cursor.getString(cursor.getColumnIndex("UserName"));
            String UserContent = cursor.getString(cursor.getColumnIndex("UserContent"));
            long time = cursor.getLong(cursor.getColumnIndex("time"));
            int type = cursor.getInt(cursor.getColumnIndex("type"));
            int messagetype = cursor.getInt(cursor.getColumnIndex("messagetype"));
            float UserVoiceTime = cursor.getFloat(cursor.getColumnIndex("UserVoiceTime"));
            String UserVoicePath = cursor.getString(cursor.getColumnIndex("UserVoicePath"));
            int sendState = cursor.getInt(cursor.getColumnIndex("sendState"));
            String groupId = cursor.getString(cursor.getColumnIndex("groupId"));
            int isListened = cursor.getInt(cursor.getColumnIndex("isListened"));
            int deviceType = cursor.getInt(cursor.getColumnIndex("deviceType"));
            String clientId = cursor.getString(cursor.getColumnIndex("clientId"));
            ChatMessageBean entity = new ChatMessageBean();
            entity.setChatId(chatId);
            entity.setId(id);
            entity.setUserName(UserName);
            entity.setUserContent(UserContent);
            entity.setTime(time);
            entity.setType(type);
            entity.setMessagetype(messagetype);
            entity.setUserVoiceTime(UserVoiceTime);
            entity.setUserVoicePath(UserVoicePath);
            entity.setSendState(sendState);
            entity.setGroupId(groupId);
            entity.setIsListened(isListened == 1);
            entity.setDeviceType(deviceType);
            entity.setUserId(clientId);
            list.add(entity);
        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return list;
    }

    /**
     * 查询指定的记录
     * @return
     */
    public List<ChatMessageBean> getHistoryData(String data,String currTime){
        List<ChatMessageBean> list = new ArrayList<>();
        String sql = "select * from char where time>=? and time <=? order by time asc";
        locationDb = locationHelper.getReadableDatabase();
        Cursor cursor = locationDb.rawQuery(sql, new String[]{data,currTime} );
        while (cursor.moveToNext()) {
            int chatId = cursor.getInt(cursor.getColumnIndex("chatId"));
            long id = cursor.getLong(cursor.getColumnIndex("id"));
            String UserName = cursor.getString(cursor.getColumnIndex("UserName"));
            String UserContent = cursor.getString(cursor.getColumnIndex("UserContent"));
            long time = cursor.getLong(cursor.getColumnIndex("time"));
            int type = cursor.getInt(cursor.getColumnIndex("type"));
            int messagetype = cursor.getInt(cursor.getColumnIndex("messagetype"));
            float UserVoiceTime = cursor.getFloat(cursor.getColumnIndex("UserVoiceTime"));
            String UserVoicePath = cursor.getString(cursor.getColumnIndex("UserVoicePath"));
            int sendState = cursor.getInt(cursor.getColumnIndex("sendState"));
            String groupId = cursor.getString(cursor.getColumnIndex("groupId"));
            int isListened = cursor.getInt(cursor.getColumnIndex("isListened"));
            int deviceType = cursor.getInt(cursor.getColumnIndex("deviceType"));
            String clientId = cursor.getString(cursor.getColumnIndex("clientId"));
            ChatMessageBean entity = new ChatMessageBean();
            entity.setChatId(chatId);
            entity.setId(id);
            entity.setUserName(UserName);
            entity.setUserContent(UserContent);
            entity.setTime(time);
            entity.setType(type);
            entity.setMessagetype(messagetype);
            entity.setUserVoiceTime(UserVoiceTime);
            entity.setUserVoicePath(UserVoicePath);
            entity.setSendState(sendState);
            entity.setGroupId(groupId);
            entity.setIsListened(isListened == 1);
            entity.setDeviceType(deviceType);
            entity.setUserId(clientId);
            list.add(entity);
        }
        cursor.close();
        //关闭数据库
        locationDb.close();
        return list;
    }

}
