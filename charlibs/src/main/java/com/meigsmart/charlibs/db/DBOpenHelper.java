package com.meigsmart.charlibs.db;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;

public class DBOpenHelper extends SQLiteOpenHelper {
    private static final String DB_NAME = "MeigsmartDb2.db"; //数据库名称
    private static final int DB_VERSION = 1;//数据库版本,大于0

    //用于聊天表
    private static final String CREATE_LOCATION = "create table char ("
            + "chatId INTEGER PRIMARY KEY AUTOINCREMENT, "
            + "id long, "
            + "UserName TEXT, "
            + "UserContent TEXT, "
            + "time long, "
            + "type int, "
            + "messagetype int, "
            + "UserVoiceTime float, "
            + "UserVoicePath TEXT, "
            + "groupId TEXT, "
            + "isListened int, "
            + "deviceType int, "
            + "clientId TEXT, "
            +"sendState int)";

    //保存lastId表
    private static final String CREATE_SAVE_LAST_ID = "create table "+LastIdDao.TABLE+" ("
            + "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            + "lastId long, "
            +"groupId TEXT)";

    //蓝牙
    private static final String CREATE_BLUETOOTH = "create table "+BluetoothDao.TABLE+" ("
            + "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            + "blueName TEXT, "
            + "password TEXT, "
            + "serialNum TEXT, "
            +"isFirstSetPsw TEXT)";//0 第一次  1 否

    public DBOpenHelper(Context context) {
        super(context, DB_NAME, null,DB_VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
        db.execSQL(CREATE_LOCATION);//执行有更改的sql语句
        db.execSQL(CREATE_SAVE_LAST_ID);
        db.execSQL(CREATE_BLUETOOTH);
        Log.e("result","---------create_table_sql-------------------");
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

    }
}
