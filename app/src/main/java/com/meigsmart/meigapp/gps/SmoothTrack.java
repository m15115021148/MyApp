package com.meigsmart.meigapp.gps;

import java.util.ArrayList;
import java.util.List;


public class SmoothTrack {

    private static double walkSpeed = 3; //默认最快3m/s
    private static double driveSpeed = 40; //默认最快40m/s

    /**
     * 轨迹曲线纠偏
     *
     * @param points 要纠偏的点
     * @param ratio  通过多次调整ratio的值, 可以消除大部分的锯齿 0.5
     * @param type   1为步行，2为驾车
     * @return
     */
    public static List<Point> doSmooth(List<Point> points, Double ratio, int type) {

        int count = points.size();
        List<Point> tmp = new ArrayList<Point>();
        for (int i = 0; i < count; i++) {
            if (i > 0 && i < count - 1) {
                Point point_a = tmp.get(tmp.size() - 1);
                Point point_b = points.get(i);
                Point point_c = points.get(i + 1);
                Double a = Distance.getDistance1(point_b.getLng(), point_b.getLat(), point_c.getLng(), point_c.getLat());
                Double b = Distance.getDistance1(point_a.getLng(), point_a.getLat(), point_c.getLng(), point_c.getLat());
                Double c = Distance.getDistance1(point_a.getLng(), point_a.getLat(), point_b.getLng(), point_b.getLat());
                if ((a + c) < 1 || a == 0 || c == 0) {
                    if (a == 0) {
                        tmp.get(tmp.size() - 1).setUnixTime(points.get(i).getUnixTime());
                    }
                    continue;
                }
                double defaultSpeed = 100;
                if (type == 1) {
                    defaultSpeed = walkSpeed;
                } else if (type == 2) {
                    defaultSpeed = driveSpeed;
                }
                double currentSpeed = c / (point_b.getUnixTime() - point_a.getUnixTime());
                if (currentSpeed > defaultSpeed) {
                    continue;
                }
                Double angle_b = Math.acos(((a * a) + (c * c) - (b * b)) / (2 * a * c));
                if (angle_b <= 0) {
                    System.out.println(angle_b);
                }
                if (angle_b < Math.PI / 6) {
//					tmp.add(point_a);
                    continue;
                } else if (angle_b < Math.PI / 3) {
                    Point p = new Point((point_a.getLng() + point_c.getLng()) / 2, (point_a.getLat() + point_c.getLat()) / 2);
                    p.setTime(points.get(i).getTime());
                    p.setWorkPlace(points.get(i).getWorkPlace());
                    p.setUnixTime(points.get(i).getUnixTime());
                    tmp.add(p);
                    continue;
                } else if (angle_b < Math.PI * ratio) {
                    Point p = new Point((point_a.getLng() + point_b.getLng() + point_c.getLng()) / 3, (point_a.getLat() + point_b.getLat() + point_c.getLat()) / 3);
                    p.setTime(points.get(i).getTime());
                    p.setWorkPlace(points.get(i).getWorkPlace());
                    p.setUnixTime(points.get(i).getUnixTime());
                    tmp.add(p);
                } else {
                    tmp.add(point_b);
                }
            } else {
                tmp.add(points.get(i));
            }
        }
        return tmp;
    }

    /**
     * 2点中点
     *
     * @param point_a
     * @param point_c
     * @return
     */
    public static Point midLocationFromLocation1(Point point_a, Point point_c) {
        Point p = new Point((point_a.getLng() + point_c.getLng()) / 2, (point_a.getLat() + point_c.getLat()) / 2);
        p.setTime(point_a.getTime());
        p.setWorkPlace(point_a.getWorkPlace());
        p.setUnixTime(point_a.getUnixTime());
        return p;
    }

    public static List<Point> correcteZShape(List<Point> points) {

        int count = points.size();
        if (count <= 5) {
            return points;
        }
        List<Point> tmp = new ArrayList<Point>();
        for (int i = 0; i < count - 4; i++) {
            if (i > 0) {
                Point point_a = tmp.get(tmp.size() - 1);
                Point point_b = points.get(i);
                Point point_c = points.get(i + 1);
                Point point_d = points.get(i + 2);
                double angle_b = 0;
                double angle_c = 0;
                Double a = Distance.getDistance1(point_b.getLng(), point_b.getLat(), point_c.getLng(), point_c.getLat());
                Double b = Distance.getDistance1(point_a.getLng(), point_a.getLat(), point_c.getLng(), point_c.getLat());
                Double c = Distance.getDistance1(point_a.getLng(), point_a.getLat(), point_b.getLng(), point_b.getLat());
                if (a > 0 && c > 0) {
                    angle_b = Math.acos(((a * a) + (c * c) - (b * b)) / (2 * a * c));
                }
                Double dd = Distance.getDistance1(point_b.getLng(), point_b.getLat(), point_c.getLng(), point_c.getLat());
                Double bb = Distance.getDistance1(point_d.getLng(), point_d.getLat(), point_c.getLng(), point_c.getLat());
                Double cc = Distance.getDistance1(point_d.getLng(), point_d.getLat(), point_b.getLng(), point_b.getLat());
                if (bb > 0 && dd > 0) {
                    angle_c = Math.acos(((dd * dd) + (bb * bb) - (cc * cc)) / (2 * dd * bb));
                }
                Double r = (angle_b + angle_c);
                if (r < Math.PI && r > 0) {
                    Point point = midLocationFromLocation1(point_a, point_d);
                    tmp.add(point);
                    tmp.add(point);
                    i++;
                } else {
                    tmp.add(point_b);
                }
            } else {
                tmp.add(points.get(i));
            }
        }
        tmp.add(points.get(count - 4));
        tmp.add(points.get(count - 3));
        tmp.add(points.get(count - 2));
        tmp.add(points.get(count - 1));
        return tmp;
    }
}
