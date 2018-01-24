package com.meigsmart.meigapp.log;

import android.util.Log;

/**
 * Created by chenMeng on 2018/1/11.
 */

public class LogUtil {
    private static boolean isPrintln = true;
    private static String TAG = "LogUtil";
    private static int LOG_LEVEL_V = 0;
    private static int LOG_LEVEL_D = 1;
    private static int LOG_LEVEL_I = 2;
    private static int LOG_LEVEL_W = 3;
    private static int LOG_LEVEL_E = 4;

    public static void v(String tag,String msg){
        TAG = tag;
        if (isPrintln)println(LOG_LEVEL_V,msg);
    }

    public static void v(String msg){
        if (isPrintln)println(LOG_LEVEL_V,msg);
    }

    public static void d(String tag,String msg){
        TAG = tag;
        if (isPrintln)println(LOG_LEVEL_D,msg);
    }

    public static void d(String msg){
        if (isPrintln)println(LOG_LEVEL_D,msg);
    }

    public static void i(String tag,String msg){
        TAG = tag;
        if (isPrintln)println(LOG_LEVEL_I,msg);
    }

    public static void i(String msg){
        if (isPrintln)println(LOG_LEVEL_I,msg);
    }

    public static void w(String tag,String msg){
        TAG = tag;
        if (isPrintln)println(LOG_LEVEL_W,msg);
    }

    public static void w(String msg){
        if (isPrintln)println(LOG_LEVEL_W,msg);
    }

    public static void e(String tag,String msg){
        TAG = tag;
        if (isPrintln)println(LOG_LEVEL_E,msg);
    }

    public static void e(String msg){
        if (isPrintln)println(LOG_LEVEL_E,msg);
    }

    private static void println(int level,String msg){
        switch (level){
            case 0:
                Log.i(TAG,msg);
                break;
            case 1:
                Log.d(TAG,msg);
                break;
            case 2:
                Log.i(TAG,msg);
                break;
            case 3:
                Log.w(TAG,msg);
                break;
            case 4:
                Log.e(TAG,msg);
                break;
            default:
                Log.e(TAG,msg);
                break;
        }
    }

}
