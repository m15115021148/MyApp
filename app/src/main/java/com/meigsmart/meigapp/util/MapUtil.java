package com.meigsmart.meigapp.util;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Point;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.widget.ZoomControls;

import com.alibaba.fastjson.JSON;
import com.baidu.mapapi.map.BaiduMap;
import com.baidu.mapapi.map.BitmapDescriptor;
import com.baidu.mapapi.map.BitmapDescriptorFactory;
import com.baidu.mapapi.map.InfoWindow;
import com.baidu.mapapi.map.MapStatus;
import com.baidu.mapapi.map.MapStatusUpdate;
import com.baidu.mapapi.map.MapStatusUpdateFactory;
import com.baidu.mapapi.map.MapView;
import com.baidu.mapapi.map.Marker;
import com.baidu.mapapi.map.MarkerOptions;
import com.baidu.mapapi.map.Overlay;
import com.baidu.mapapi.map.OverlayOptions;
import com.baidu.mapapi.map.PolygonOptions;
import com.baidu.mapapi.map.Polyline;
import com.baidu.mapapi.map.PolylineOptions;
import com.baidu.mapapi.map.Stroke;
import com.baidu.mapapi.model.LatLng;
import com.baidu.mapapi.model.LatLngBounds;
import com.baidu.mapapi.utils.CoordinateConverter;
import com.baidu.mapapi.utils.DistanceUtil;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.gps.Distance;
import com.meigsmart.meigapp.gps.IndexModel;
import com.meigsmart.meigapp.gps.MapModel;
import com.meigsmart.meigapp.log.LogUtil;

import java.util.ArrayList;
import java.util.List;

/**
 * @desc 地图处理 工具类
 * Created by chenmeng on 2017/9/5.
 */

public class MapUtil {
    private Context mContext;
    private MapView mMapView;
    private BaiduMap mBaiduMap;//百度地图对象
    private BaiduMap.OnMarkerClickListener markClick;// 地图标注点击事件
    private float level = 15;//地图显示等级
    /**
     * 路线覆盖物
     */
    public Overlay polylineOverlay = null;
    private MapStatus mapStatus = null;
    private Marker mMoveMarker = null;
    public LatLng lastPoint = null;

    public MapUtil(Context context,MapView mMapView){
        this.mContext = context;
        this.mMapView = mMapView;
        this.mBaiduMap = mMapView.getMap();
    }

    /**
     * 隐藏图标
     */
    public void hidezoomView() {
        // 隐藏logo
        View child = mMapView.getChildAt(1);
        if (child != null && (child instanceof ImageView || child instanceof ZoomControls)) {
            child.setVisibility(View.INVISIBLE);
        }
        //地图上比例尺
        mMapView.showScaleControl(false);
        // 隐藏缩放控件
        mMapView.showZoomControls(false);
    }

    public void updateStatus(LatLng currentPoint, boolean showMarker) {
        if (currentPoint == null) {
            return;
        }
        if (null == mBaiduMap ) {
            this.mBaiduMap = mMapView.getMap();
        }

        if (null != mBaiduMap.getProjection()) {
            Point screenPoint = mBaiduMap.getProjection().toScreenLocation(currentPoint);
            // 点在屏幕上的坐标超过限制范围，则重新聚焦底图
            if (screenPoint.y < 200 || screenPoint.y > MyApplication.screenHeight - 500
                    || screenPoint.x < 200 || screenPoint.x > MyApplication.screenWidth - 200
                    || null == mapStatus) {
                animateMapStatus(currentPoint, level);
            }
        } else if (null == mapStatus) {
            // 第一次定位时，聚焦底图
            setMapStatus(currentPoint, level);
        }

        if (showMarker) {
            addMarker(currentPoint);
        }

    }

    /**
     * update地图的状态与变化
     *
     * @param point
     */
    public void updateStatus(LatLng point,int zoom) {
        MapStatus mMapStatus = new MapStatus.Builder().target(point).zoom(zoom).build();
        // 定义MapStatusUpdate对象，以便描述地图状态将要发生的变化
        MapStatusUpdate mMapStatusUpdate = MapStatusUpdateFactory.newMapStatus(mMapStatus);
        // 改变地图状态
        mBaiduMap.setMapStatus(mMapStatusUpdate);
    }

    public void clearHistory(){
        if (null != polylineOverlay) {
            polylineOverlay.remove();
        }
        if (null != mMoveMarker) {
            mMoveMarker.remove();
            mMoveMarker = null;
        }
        if (null != mBaiduMap) {
            mBaiduMap.clear();
        }
    }

    public void clear(){
        if (null != mMoveMarker) {
            mMoveMarker.remove();
        }
        if (null != polylineOverlay) {
            polylineOverlay.remove();
        }
        if (null != mBaiduMap) {
            mBaiduMap.clear();
        }
    }

    public void setMapStatus(LatLng point, float zoom) {
        MapStatus.Builder builder = new MapStatus.Builder();
        mapStatus = builder.target(point).zoom(zoom).build();
        mBaiduMap.setMapStatus(MapStatusUpdateFactory.newMapStatus(mapStatus));
    }

    /**
     * 添加地图覆盖物
     */
    public void addMarker(LatLng currentPoint) {
        if (null == mMoveMarker) {
            BitmapDescriptor bitmap = BitmapDescriptorFactory.fromResource(R.mipmap.point);
            mMoveMarker = addOverlay(currentPoint, bitmap, null);
            return;
        }

        if (null != lastPoint) {
            moveLooper(currentPoint);
        } else {
            lastPoint = currentPoint;
            mMoveMarker.setPosition(currentPoint);
        }
    }

    /**
     * 移动逻辑
     */
    public void moveLooper(LatLng endPoint) {

        mMoveMarker.setPosition(lastPoint);
        mMoveMarker.setRotate((float)getAngle(lastPoint, endPoint));

        double slope = getSlope(lastPoint, endPoint);
        // 是不是正向的标示（向上设为正向）
        boolean isReverse = (lastPoint.latitude > endPoint.latitude);
        double intercept = getInterception(slope, lastPoint);
        double xMoveDistance = isReverse ? getXMoveDistance(slope) : -1 * getXMoveDistance(slope);

        for (double latitude = lastPoint.latitude; latitude > endPoint.latitude == isReverse; latitude =
                latitude - xMoveDistance) {
            LatLng latLng;
            if (slope != Double.MAX_VALUE) {
                latLng = new LatLng(latitude, (latitude - intercept) / slope);
            } else {
                latLng = new LatLng(latitude, lastPoint.longitude);
            }
            mMoveMarker.setPosition(latLng);
        }
    }

    public Marker addOverlay(LatLng currentPoint, BitmapDescriptor icon, Bundle bundle) {
        OverlayOptions overlayOptions = new MarkerOptions().position(currentPoint)
                .icon(icon).zIndex(9).draggable(true);
        Marker marker = (Marker) mBaiduMap.addOverlay(overlayOptions);
        if (null != bundle) {
            marker.setExtraInfo(bundle);
        }
        return marker;
    }

    /**
     * 画矩形
     * @param top       左上角
     * @param left    左下角
     * @param right  右上角
     * @param bottom  右下角
     */
    public void drawRectangle(LatLng top,LatLng left,LatLng right,LatLng bottom){
        //矩形的点
        List<LatLng> pts = new ArrayList<>();
        pts.add(top);
        pts.add(right);
        pts.add(bottom);
        pts.add(left);

        //构建用户绘制多边形的Option对象
        OverlayOptions polygonOption = new PolygonOptions()
                .points(pts)
                .stroke(new Stroke(6, 0xAA00FF00))
                .fillColor(0xAAFFFF00);
        //在地图上添加多边形Option，用于显示
        mBaiduMap.addOverlay(polygonOption);
    }

    public boolean isZeroPoint(double lat,double lng){
        if (lat!=5e-324 && lng!=5e-324 &&lat!=4.9E-324 && lng!=4.9E-324 && lat!=0 && lng!=0 && lat!=-3.067393572659021E-8 && lng!=2.890871144776878E-9){
            return false;
        }
        return true;
    }

    /**
     * 别的地图转百度坐标
     *
     * @param ll
     * @return
     */
    public LatLng changeBaidu(LatLng ll) {
        // 将google地图、soso地图、aliyun地图、mapabc地图和amap地图// 所用坐标转换成百度坐标
        CoordinateConverter converter = new CoordinateConverter();
        converter.from(CoordinateConverter.CoordType.COMMON);
        // sourceLatLng待转换坐标
        converter.coord(ll);
        LatLng desLatLng = converter.convert();
        LogUtil.i("TAG", "latitude:" + desLatLng.latitude + "longitude:" + desLatLng.longitude);
        return desLatLng;
    }

    /**
     * 原始GPS转百度地图
     * @param ll
     * @return
     */
    public LatLng changeBaiduByGPS(LatLng ll) {
        // 将GPS设备采集的原始GPS坐标转换成百度坐标
        CoordinateConverter converter = new CoordinateConverter();
        converter.from(CoordinateConverter.CoordType.GPS);
        // sourceLatLng待转换坐标
        converter.coord(ll);
        LatLng desLatLng = converter.convert();
        return desLatLng;
    }

    /**
     * 绘制点
     */
    public void drawPoint(List<MapModel> list){
        if (list == null) return;
        for (int i=0;i<list.size();i++){
            LatLng latLng = new LatLng(Double.parseDouble(list.get(i).getLat()),Double.parseDouble(list.get(i).getLng()));
            // 构建Marker图标
            BitmapDescriptor bitmap = null;
            if (list.get(i).getOnline().equals("0")){
                bitmap = BitmapDescriptorFactory.fromResource(R.mipmap.point_offline);
            }else{
                bitmap = BitmapDescriptorFactory.fromResource(R.mipmap.point);
            }

            OverlayOptions option = new MarkerOptions()
                    .position(latLng)
                    .icon(bitmap)
                    .zIndex(2);
            Marker marker = (Marker) mBaiduMap.addOverlay(option);
            Bundle bundle = new Bundle();
            bundle.putSerializable("mapModel",list.get(i));
            marker.setExtraInfo(bundle);

            updateStatus(latLng,15);
        }
        markClick = new BaiduMap.OnMarkerClickListener() {
            @Override
            public boolean onMarkerClick(Marker marker) {
                if (marker.getZIndex() == 3) {
                    return false;
                }
                Bundle b = marker.getExtraInfo();
                if (b == null){
                    return false;
                }
                MapModel model = (MapModel) b.get("mapModel");

                View view = LayoutInflater.from(mContext).inflate(R.layout.map_marker_layout,null);
                TextView name = (TextView) view.findViewById(R.id.name);
                TextView altitude = (TextView) view.findViewById(R.id.altitude);
                TextView time = (TextView) view.findViewById(R.id.time);

                name.setText(mContext.getResources().getString(R.string.map_layout_name)+model.getName());
                altitude.setText(mContext.getResources().getString(R.string.map_layout_altitude)+model.getAltitude());
                time.setText(mContext.getResources().getString(R.string.map_layout_time)+model.getTime());

                view.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mBaiduMap.hideInfoWindow();
                    }
                });
                // 构建对话框用于显示
                // 创建InfoWindow , 传入 view， 地理坐标， y 轴偏移量
                InfoWindow mInfoWindow = new InfoWindow(view,
                        new LatLng(Double.parseDouble(model.getLat()),Double.parseDouble(model.getLng())),
                        -20);
                // 显示InfoWindow
                mBaiduMap.showInfoWindow(mInfoWindow);
                return false;
            }
        };
        mBaiduMap.setOnMarkerClickListener(markClick);
    }

    /**
     * 绘制轨迹
     */
    public void drawHistoryTrack(List<LatLng> points) {
        LogUtil.v("result","pointsSize:"+points.size());
        // 绘制新覆盖物前，清空之前的覆盖物
        mBaiduMap.clear();
        if (points == null || points.size() == 0) {
            if (null != polylineOverlay) {
                polylineOverlay.remove();
                polylineOverlay = null;
            }
            return;
        }
        // 构建Marker图标
        BitmapDescriptor bitmap = BitmapDescriptorFactory.fromResource(R.mipmap.point);

        if (points.size() == 1) {
            OverlayOptions startOptions = new MarkerOptions().position(points.get(0)).icon(bitmap)
                    .zIndex(9).draggable(true);
            mBaiduMap.addOverlay(startOptions);
            animateMapStatus(points.get(0), level);
            return;
        }

        LatLng startPoint;
        LatLng endPoint;
        int startIndex;
        int endIndex;
        int lastIndex = 0;

        // 构建Marker图标
        BitmapDescriptor startBt = BitmapDescriptorFactory.fromResource(R.mipmap.icon_start);
        BitmapDescriptor endBt = BitmapDescriptorFactory.fromResource(R.mipmap.icon_end);

        List<Integer> colorValue = new ArrayList<Integer>();
        colorValue.add(0xFF32A5E3);
        colorValue.add(0xFFFF9D01);
        colorValue.add(0xFF007AFF);

        List<IndexModel> index = getIndex(points);

        if (index.size()>0){
            OverlayOptions polylineOptions = null;
            OverlayOptions startOptions = null;
            OverlayOptions endOptions = null;

            for (int i=0;i<index.size();i++){

                if (i == 0){
                    startPoint = points.get(0);
                    startIndex = 0;
                } else{
                    startPoint = points.get(index.get(i).getStart());
                    startIndex = index.get(i).getStart();
                }
                endPoint = points.get(index.get(i).getEnd());
                endIndex = index.get(i).getEnd();
                lastIndex  = index.get(index.size()-1).getStart();

                List<LatLng> list = getCurrData(points,startIndex,endIndex);

                if (list.size()>2){

                    // 添加起点图标
                    startOptions = new MarkerOptions().position(startPoint).icon(startBt).zIndex(9).draggable(true);
                    // 添加终点图标
                    endOptions = new MarkerOptions().position(endPoint).icon(endBt).zIndex(9).draggable(true);
                    // 添加路线（轨迹）
                    polylineOptions = new PolylineOptions().width(10).color(colorValue.get(i%2)).points(list);

                    mBaiduMap.addOverlay(startOptions);
                    mBaiduMap.addOverlay(endOptions);
                    polylineOverlay = mBaiduMap.addOverlay(polylineOptions);

                }
            }

            if (points.size()-1-lastIndex>2){
                // 添加起点图标
                startOptions = new MarkerOptions().position(points.get(lastIndex)).icon(startBt).zIndex(9).draggable(true);
                // 添加终点图标
                endOptions = new MarkerOptions().position(points.get(points.size()-1)).icon(endBt).zIndex(9).draggable(true);
                // 添加路线（轨迹）
                polylineOptions = new PolylineOptions().width(10).color(Color.GREEN).points(getCurrData(points,lastIndex,points.size()-1));

                mBaiduMap.addOverlay(startOptions);
                mBaiduMap.addOverlay(endOptions);
                polylineOverlay = mBaiduMap.addOverlay(polylineOptions);
            }

        }else{
            startPoint = points.get(0);
            endPoint = points.get(points.size() - 1);

            // 添加起点图标
            OverlayOptions startOptions = new MarkerOptions()
                    .position(startPoint).icon(startBt)
                    .zIndex(9).draggable(true);
            // 添加终点图标
            OverlayOptions endOptions = new MarkerOptions().position(endPoint)
                    .icon(endBt).zIndex(9).draggable(true);
            // 添加路线（轨迹）
            OverlayOptions polylineOptions = new PolylineOptions().width(10)
                    .color(colorValue.get(0)).points(points);

            mBaiduMap.addOverlay(startOptions);
            mBaiduMap.addOverlay(endOptions);
            polylineOverlay = mBaiduMap.addOverlay(polylineOptions);

        }


//        Polyline mPolyline = (Polyline) polylineOverlay;
//        mPolyline.setDottedLine(true);

        animateMapStatus(points);
    }

    private List<LatLng> getCurrData(List<LatLng> points,int start,int end){
        LogUtil.v("result","points_size:"+points.size());
        LogUtil.v("result","start:"+start+"\nend:"+end);
        List<LatLng> list = new ArrayList<>();
        for (int i=0;i<points.size();i++){
            if (i>=start && i<=end){
                LatLng latLng = new LatLng(points.get(i).latitude,points.get(i).longitude);
                list.add(latLng);
            }
        }
        LogUtil.v("result","getCurrData:"+list.size());
        return list;
    }

    private List<IndexModel> getIndex(List<LatLng> points){
        List<IndexModel> list = new ArrayList<>();

        for (int i=0;i<points.size();i++){
            if (i < points.size()-1){
                LatLng l1 = points.get(i);
                LatLng l2 = points.get(i+1);
                if (Distance.getDistance1(l2.longitude,l2.latitude,l1.longitude,l1.latitude)>1000){
                    IndexModel model = new IndexModel();
                    model.setOff(Distance.getDistance1(l2.longitude,l2.latitude,l1.longitude,l1.latitude));
                    model.setEnd(i);
                    model.setStart(i+1);
                    list.add(model);
                }
            }
        }
        LogUtil.v("result","index:"+list.size());
        LogUtil.v("result","index:"+JSON.toJSONString(list));
        return list;
    }

    public void animateMapStatus(List<LatLng> points) {
        if (null == points || points.isEmpty()) {
            return;
        }
        LatLngBounds.Builder builder = new LatLngBounds.Builder();
        for (LatLng point : points) {
            builder.include(point);
        }
        MapStatusUpdate msUpdate = MapStatusUpdateFactory.newLatLngBounds(builder.build());
        mBaiduMap.animateMapStatus(msUpdate);
    }

    public void animateMapStatus(LatLng point, float zoom) {
        MapStatus.Builder builder = new MapStatus.Builder();
        mapStatus = builder.target(point).zoom(zoom).build();
        mBaiduMap.animateMapStatus(MapStatusUpdateFactory.newMapStatus(mapStatus));
    }

    /**
     * 根据两点算取图标转的角度
     */
    private double getAngle(LatLng fromPoint, LatLng toPoint) {
        double slope = getSlope(fromPoint, toPoint);
        if (slope == Double.MAX_VALUE) {
            if (toPoint.latitude > fromPoint.latitude) {
                return 0;
            } else {
                return 180;
            }
        }
        float deltAngle = 0;
        if ((toPoint.latitude - fromPoint.latitude) * slope < 0) {
            deltAngle = 180;
        }
        double radio = Math.atan(slope);
        return 180 * (radio / Math.PI) + deltAngle - 90;
    }

    /**
     * 算斜率
     */
    private double getSlope(LatLng fromPoint, LatLng toPoint) {
        if (toPoint.longitude == fromPoint.longitude) {
            return Double.MAX_VALUE;
        }
        return (toPoint.latitude - fromPoint.latitude) / (toPoint.longitude - fromPoint.longitude);
    }

    /**
     * 根据点和斜率算取截距
     */
    private double getInterception(double slope, LatLng point) {
        return point.latitude - slope * point.longitude;
    }

    /**
     * 计算x方向每次移动的距离
     */
    private double getXMoveDistance(double slope) {
        if (slope == Double.MAX_VALUE) {
            return 0.0001;
        }
        return Math.abs((0.0001 * slope) / Math.sqrt(1 + slope * slope));
    }

    public float getLevel() {
        return level;
    }

    public void setLevel(float level) {
        this.level = level;
    }

}
