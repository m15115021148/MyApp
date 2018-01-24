package com.meigsmart.meigapp.model;

import com.baidu.mapapi.map.BitmapDescriptor;
import com.baidu.mapapi.map.Marker;
import com.baidu.mapapi.map.Overlay;

/**
 * Created by chenMeng on 2017/12/23.
 */

public class DrawMapModel {
    private Marker marker;
    private Overlay overlay;
    private BitmapDescriptor bt;
    private int index;

    public int getIndex() {
        return index;
    }

    public void setIndex(int index) {
        this.index = index;
    }

    public Marker getMarker() {
        return marker;
    }

    public void setMarker(Marker marker) {
        this.marker = marker;
    }

    public Overlay getOverlay() {
        return overlay;
    }

    public void setOverlay(Overlay overlay) {
        this.overlay = overlay;
    }

    public BitmapDescriptor getBt() {
        return bt;
    }

    public void setBt(BitmapDescriptor bt) {
        this.bt = bt;
    }
}
