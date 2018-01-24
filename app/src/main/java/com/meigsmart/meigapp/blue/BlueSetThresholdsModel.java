package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2017/11/30.
 */

public class BlueSetThresholdsModel extends BluBaseModel {
    private Thresholds thresholds;

    public Thresholds getThresholds() {
        return thresholds;
    }

    public void setThresholds(Thresholds thresholds) {
        this.thresholds = thresholds;
    }

    public static class Thresholds {
        private float shock_x;
        private float shock_y;
        private float shock_z;
        private float temperature_upper;
        private float temperature_lower;
        private float humidity_upper;
        private float humidity_lower;
        private float pressure_upper;
        private float pressure_lower;

        public float getShock_x() {
            return shock_x;
        }

        public void setShock_x(float shock_x) {
            this.shock_x = shock_x;
        }

        public float getShock_y() {
            return shock_y;
        }

        public void setShock_y(float shock_y) {
            this.shock_y = shock_y;
        }

        public float getShock_z() {
            return shock_z;
        }

        public void setShock_z(float shock_z) {
            this.shock_z = shock_z;
        }

        public float getTemperature_upper() {
            return temperature_upper;
        }

        public void setTemperature_upper(float temperature_upper) {
            this.temperature_upper = temperature_upper;
        }

        public float getTemperature_lower() {
            return temperature_lower;
        }

        public void setTemperature_lower(float temperature_lower) {
            this.temperature_lower = temperature_lower;
        }

        public float getHumidity_upper() {
            return humidity_upper;
        }

        public void setHumidity_upper(float humidity_upper) {
            this.humidity_upper = humidity_upper;
        }

        public float getHumidity_lower() {
            return humidity_lower;
        }

        public void setHumidity_lower(float humidity_lower) {
            this.humidity_lower = humidity_lower;
        }

        public float getPressure_upper() {
            return pressure_upper;
        }

        public void setPressure_upper(float pressure_upper) {
            this.pressure_upper = pressure_upper;
        }

        public float getPressure_lower() {
            return pressure_lower;
        }

        public void setPressure_lower(float pressure_lower) {
            this.pressure_lower = pressure_lower;
        }
    }

}
