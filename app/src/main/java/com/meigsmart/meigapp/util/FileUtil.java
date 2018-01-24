package com.meigsmart.meigapp.util;

import android.app.Activity;
import android.content.Context;
import android.os.Environment;

import com.meigsmart.meigapp.log.LogUtil;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * 文件操作
 * Created by chenMeng on 2017/9/18.
 */

public class FileUtil {

    /**
     * 创建根目录 自动区分是否在sd卡 还是内部储存
     * @param fileName 根目录 名称
     * @return
     */
    public static File createRootDirectory(String fileName){
        String filePath =  Environment.getExternalStorageDirectory().toString() + "/";
        File file = new File(filePath +  fileName);
        file.mkdir();
        return file;
    }

    /**
     * 遍历文件夹下所以的文件
     *
     * @param file 文件夹
     * @return 返回文件的路径 集合
     */
    public static List<String> getListFiles(File file) {
        List<String> list = new ArrayList<>();
        File[] fileArray = file.listFiles();
        if (fileArray==null){
            return new ArrayList<>();
        }
        if (fileArray.length == 0) {
            return new ArrayList<>();
        } else {
            // 遍历该目录
            for (File currentFile : fileArray) {
                if (currentFile.isFile()) {
                    // 文件则
                    list.add(currentFile.getPath());
                } else {
                    // 递归
                    getListFiles(currentFile);
                }
            }
        }
        return list;
    }

    /**
     * 遍历文件夹下所以的文件
     *
     * @param file 文件夹
     * @return 返回文件名 集合
     */
    public static List<String> getListFilesName(File file) {
        List<String> list = new ArrayList<>();
        File[] fileArray = file.listFiles();
        if (fileArray==null){
            return new ArrayList<>();
        }
        if (fileArray.length == 0) {
            return new ArrayList<>();
        } else {
            // 遍历该目录
            for (File currentFile : fileArray) {
                if (currentFile.isFile()) {
                    // 文件则
                    list.add(currentFile.getName());
                } else {
                    // 递归
                    getListFilesName(currentFile);
                }
            }
        }
        return list;
    }

    /**
     * 删除文件
     *
     * @param filePath
     * @return
     */
    public static boolean deleteFilePath(String filePath) {
        File file = new File(filePath);
        if (file == null || !file.exists() || file.isDirectory())
            return false;
        file.delete();
        return true;
    }

    /**
     * 保存写入的文件到sd卡目录下
     *
     * @param fileName
     * @param txt
     */
    public static String saveFilePath(File parentFile, String fileName, String txt, boolean save) {
        try {
            File file = new File(parentFile, fileName);
            LogUtil.w("result", "创建的文件的路径:" + file.getPath());
            FileOutputStream fos = new FileOutputStream(file, save);//false每次写入都会替换内容
            byte[] b = txt.getBytes();
            fos.write(b);
            fos.close();
            return file.getPath();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "";
    }

    /**
     * 读取文件的内容
     *
     * @param filePath 文件路径
     * @param fileName 文件名称
     * @return
     */
    public static String readFilePath(String filePath, String fileName) {
        try {
            File file = new File(filePath, fileName);
            if (!file.exists()) {
                return null;
            }
            FileInputStream fis = new FileInputStream(file);
            BufferedInputStream bis = new BufferedInputStream(fis);
            ByteArrayOutputStream arrayOutputStream = new ByteArrayOutputStream();
            byte[] b = new byte[1024];
            int len = bis.read(b);//表示每一次读取1024个字节放到byte数组里面
            while (len != -1) {
                len = bis.read(b);
                arrayOutputStream.write(b, 0, b.length);
            }
            arrayOutputStream.close();
            String content = new String(arrayOutputStream.toByteArray());
            LogUtil.v("result", "读出来的内容:" + content);
            return content;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }


    /**
     * 文件的保存
     *
     * @param context  类
     * @param fileName 文件的名称
     * @param txt      内容
     */
    public static void saveFile(Context context, String fileName, String txt) {
        try {
            /*
             * 第一个参数，代表文件名称，注意这里的文件名称不能包括任何的/或者/这种分隔符，只能是文件名
             *          该文件会被保存在/data/data/应用名称/files/sss.txt
             * 第二个参数，代表文件的操作模式
             *          MODE_PRIVATE 私有（只能创建它的应用访问） 重复写入时会文件覆盖
             *          MODE_APPEND  私有   重复写入时会在文件的末尾进行追加，而不是覆盖掉原来的文件
             *          MODE_WORLD_READABLE 公用  可读
             *          MODE_WORLD_WRITEABLE 公用 可读写
             *  */
            FileOutputStream outputStream = context.openFileOutput(fileName, Activity.MODE_PRIVATE);
            outputStream.write(txt.getBytes());
            outputStream.flush();
            outputStream.close();
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    /**
     * 文件的读取
     *
     * @param context  类
     * @param fileName 文件的名称
     * @return
     */
    public static String readFile(Context context, String fileName) {
        try {
            FileInputStream inputStream = context.openFileInput(fileName);
            byte[] bytes = new byte[1024];
            ByteArrayOutputStream arrayOutputStream = new ByteArrayOutputStream();
            while (inputStream.read(bytes) != -1) {
                arrayOutputStream.write(bytes, 0, bytes.length);
            }
            inputStream.close();
            arrayOutputStream.close();
            String content = new String(arrayOutputStream.toByteArray());
            LogUtil.v("result", "读出来的内容:" + content);
            return content;
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return "暂无内容";
    }


}
