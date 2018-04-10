package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2018/4/3.
 */

public class BluSetIntervalsModel extends BluBaseModel {
    private Intervals Intervals;

    public Intervals getIntervals() {
        return Intervals;
    }

    public void setIntervals(Intervals intervals) {
        this.Intervals = intervals;
    }

    public static class Intervals{
        private int active;
        private int passive;

        public int getActive() {
            return active;
        }

        public void setActive(int active) {
            this.active = active;
        }

        public int getPassive() {
            return passive;
        }

        public void setPassive(int passive) {
            this.passive = passive;
        }
    }
}
