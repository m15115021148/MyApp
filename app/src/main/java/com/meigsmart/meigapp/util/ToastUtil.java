package com.meigsmart.meigapp.util;

import android.content.Context;
import android.view.Gravity;
import android.view.WindowManager;
import android.widget.Toast;

import com.meigsmart.meigapp.application.MyApplication;

/**
 *
 * @author chenmeng
 * @Description Toast工具类，分不同量级进行提示
 * @date create at 2017年9月1日 下午4:19:07
 */
public class ToastUtil {

	/**
	 * 长时间显示 位置居中
	 * @param title
	 *            显示的内容
	 */
	public static void showCenterLong(String title) {
		Toast toast = Toast.makeText(MyApplication.getInstance().getApplicationContext(), title, Toast.LENGTH_LONG);
		toast.setGravity(Gravity.CENTER, 0, 0);
		toast.show();
	}

	/**
	 * 显示时间两秒，位置居中
	 *
	 * @param title
	 *            显示的内容
	 */
	public static void showCenterShort(String title) {
		Toast toast = Toast.makeText(MyApplication.getInstance().getApplicationContext(), title, Toast.LENGTH_SHORT);
		toast.setGravity(Gravity.CENTER, 0, 0);
		toast.show();
	}

	/**
	 * 长时间显示 位置居下
	 * 
	 * @param title
	 */
	@SuppressWarnings("deprecation")
	public static void showBottomLong(String title) {
		Toast toast = Toast.makeText(MyApplication.getInstance().getApplicationContext(), title, Toast.LENGTH_LONG);
		WindowManager manager = (WindowManager) MyApplication.getInstance().getSystemService(Context.WINDOW_SERVICE);
		toast.setGravity(Gravity.BOTTOM, 0, manager.getDefaultDisplay()
				.getHeight() / 10);
		toast.show();
	}

	/**
	 * 短时间显示 位置居下
	 * 
	 * @param title
	 */
	@SuppressWarnings("deprecation")
	public static void showBottomShort(String title) {
		Toast toast = Toast.makeText(MyApplication.getInstance().getApplicationContext(), title, Toast.LENGTH_SHORT);
		WindowManager manager = (WindowManager) MyApplication.getInstance().getSystemService(Context.WINDOW_SERVICE);
		toast.setGravity(Gravity.BOTTOM, 0, manager.getDefaultDisplay()
				.getHeight() / 10);
		toast.show();
	}
}
