package com.meigsmart.meigapp.gps;

public class Point {

    private double lng;
    private double lat;
    private String workPlace = "";//没用
    private String time;//time
    private double unixTime;//time.gettime()/1000


    public double getUnixTime() {
        return unixTime;
    }

    public void setUnixTime(double unixTime) {
        this.unixTime = unixTime;
    }

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }

    public String getWorkPlace() {
        return workPlace;
    }

    public void setWorkPlace(String workPlace) {
        this.workPlace = workPlace;
    }

    public double getLng() {
        return lng;
    }

    public void setLng(double lng) {
        this.lng = lng;
    }

    public double getLat() {
        return lat;
    }

    public void setLat(double lat) {
        this.lat = lat;
    }

    public Point(double lng, double lat) {
        super();
        this.lng = lng;
        this.lat = lat;
    }

    public Point() {
        super();
    }


}
