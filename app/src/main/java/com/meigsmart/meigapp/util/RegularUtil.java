package com.meigsmart.meigapp.util;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 *
 * @author chenmeng
 * @Description 常用的正则表达式
 * @date create at 2017年10月31日 上午10:01:34
 */
public class RegularUtil {

	/**
	 * 必须是数字和字母组合(长度在6-16位) 密码的设置规则
	 * 
	 * @param pwd
	 * @return
	 */
	public static boolean isPWD(String pwd) {
		// Pattern p = Pattern.compile("^[A-Za-z0-9|_]{6,16}$");
		Pattern p = Pattern
				.compile("^(?![0-9]+$)(?![a-zA-Z]+$)[0-9A-Za-z]{6,16}$");
		Matcher m = p.matcher(pwd);
		return m.matches();
	}

	/**
	 * 只能是数字 字母 _  组合
	 * @param str
	 * @return
	 */
	public static boolean isInputCharOrNum(String str){
		Pattern p = Pattern.compile("^([0-9a-zA-Z_]+)|(^\\s*)$");
		Matcher m = p.matcher(str);
		return m.matches();
	}

	/**
	 * 验证手机号
	 * 
	 * @param mobiles
	 * @return
	 */
	public static boolean isMobileNO(String mobiles) {
		Pattern p = Pattern
				.compile("^((13[0-9])|(15[^4,\\D])|170|(18[0,5-9]))\\d{8}$");
		Matcher m = p.matcher(mobiles);
		return m.matches();
	}

	/**
	 * 验证邮箱
	 * @param email
	 * @return
	 */
	public static boolean isEmail(String email) {
		String str = "^([a-zA-Z0-9_\\-\\.]+)@((\\[[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.)|(([a-zA-Z0-9\\-]+\\.)+))([a-zA-Z]{2,4}|[0-9]{1,3})(\\]?)$";
		Pattern p = Pattern.compile(str);
		Matcher m = p.matcher(email);
		return m.matches();
	}
	
	/**
	 * 验证身份证是否规范
	 * @param idCard
	 * @return
	 */
	public static boolean isIdCard(String idCard){
		String regx = "[0-9]{17}X";
        String reg1 = "[0-9]{15}";
        String regex = "[0-9]{18}";
		return idCard.matches(regx) || idCard.matches(reg1) || idCard.matches(regex);
	}

}
