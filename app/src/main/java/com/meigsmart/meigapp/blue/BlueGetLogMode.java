package com.meigsmart.meigapp.blue;

/**
 * Created by chenMeng on 2017/11/30.
 */

public class BlueGetLogMode extends BluBaseModel {
    private GetLog log;

    public GetLog getLog() {
        return log;
    }

    public void setLog(GetLog log) {
        this.log = log;
    }

    public static class GetLog {
        private String type;
        private int lines;
        private String data;

        public String getData() {
            return data;
        }

        public void setData(String data) {
            this.data = data;
        }

        public String getType() {
            return type;
        }

        public void setType(String type) {
            this.type = type;
        }

        public int getLines() {
            return lines;
        }

        public void setLines(int lines) {
            this.lines = lines;
        }
    }
}
