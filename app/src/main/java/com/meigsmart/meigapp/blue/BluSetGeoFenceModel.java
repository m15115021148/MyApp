package com.meigsmart.meigapp.blue;

import java.util.List;

/**
 * Created by chenMeng on 2017/11/30.
 */

public class BluSetGeoFenceModel extends BluBaseModel{
    private List<GeoFence> geo_fences;

    public List<GeoFence> getGeo_fences() {
        return geo_fences;
    }

    public void setGeo_fences(List<GeoFence> geo_fences) {
        this.geo_fences = geo_fences;
    }

    public static class GeoFence {
        private double latitude;
        private String name;
        private int radius;
        private double longitude;
        private int index;

        public GeoFence(){}

        public double getLatitude() {
            return latitude;
        }

        public void setLatitude(double latitude) {
            this.latitude = latitude;
        }

        public String getName() {
            return name;
        }

        public void setName(String name) {
            this.name = name;
        }

        public int getRadius() {
            return radius;
        }

        public void setRadius(int radius) {
            this.radius = radius;
        }

        public double getLongitude() {
            return longitude;
        }

        public void setLongitude(double longitude) {
            this.longitude = longitude;
        }

        public int getIndex() {
            return index;
        }

        public void setIndex(int index) {
            this.index = index;
        }
    }
}
