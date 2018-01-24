package com.meigsmart.meigapp.gps;

import java.text.DecimalFormat;

/**
 * 计算两个点之间的距离
 * 
 * @author chenmeng
 * 
 */
public class Distance {
	private static final double EARTH_RADIUS = 6378137;
	private static DecimalFormat df = new DecimalFormat("######0.00");

	private static double rad(double d) {
		return d * Math.PI / 180.0;
	}

	/**
	 * 根据两点的经纬度计算距离,单位:m
	 * 
	 * @param lng1
	 * @param lat1
	 * @param lng2
	 * @param lat2
	 * @return
	 */
	public static String getDistance(double lng1, double lat1, double lng2,
			double lat2) {
		double radLat1 = rad(lat1);
		double radLat2 = rad(lat2);
		double a = radLat1 - radLat2;
		double b = rad(lng1) - rad(lng2);
		double s = 2 * Math.asin(Math.sqrt(Math.pow(Math.sin(a / 2), 2)
				+ Math.cos(radLat1) * Math.cos(radLat2)
				* Math.pow(Math.sin(b / 2), 2)));
		s = s * EARTH_RADIUS;
		s = Math.round(s * 10000) / 10000;
		return df.format(s);
	}
	
	
	/**
	 * 根据两点的经纬度计算距离,单位:m
	 * 
	 * @param lng1
	 * @param lat1
	 * @param lng2
	 * @param lat2
	 * @return
	 */
	public static Double getDistance1(double lng1, double lat1, double lng2,
			double lat2) {
		double radLat1 = rad(lat1);
		double radLat2 = rad(lat2);
		double a = radLat1 - radLat2;
		double b = rad(lng1) - rad(lng2);
		double s = 2 * Math.asin(Math.sqrt(Math.pow(Math.sin(a / 2), 2)
				+ Math.cos(radLat1) * Math.cos(radLat2)
				* Math.pow(Math.sin(b / 2), 2)));
		s = s * EARTH_RADIUS;
		s = Math.round(s * 10000) / 10000;
		return s;
	}
	/**
	 * 点到线段的最短距离
	 * @param x
	 * @param y
	 * @param x1
	 * @param y1
	 * @param x2
	 * @param y2
	 * @return
	 */
	public static double PointToSegDist(double x, double y, double x1, double y1, double x2, double y2)  {  
		double cross = (x2 - x1) * (x - x1) + (y2 - y1) * (y - y1);  
		if (cross <= 0) return Math.sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));  
		  
		double d2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);  
		if (cross >= d2) return Math.sqrt((x - x2) * (x - x2) + (y - y2) * (y - y2));  
		   
		double r = cross / d2;  
		double px = x1 + (x2 - x1) * r;  
		double py = y1 + (y2 - y1) * r;  
		return Math.sqrt((x - px) * (x - px) + (py - y1) * (py - y1));  
	}
	
	
}
