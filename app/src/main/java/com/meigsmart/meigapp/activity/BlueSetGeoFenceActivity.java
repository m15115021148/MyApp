package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.text.Html;
import android.text.TextUtils;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.ZoomControls;

import com.alibaba.fastjson.JSON;
import com.baidu.mapapi.map.BaiduMap;
import com.baidu.mapapi.map.BitmapDescriptor;
import com.baidu.mapapi.map.BitmapDescriptorFactory;
import com.baidu.mapapi.map.CircleOptions;
import com.baidu.mapapi.map.InfoWindow;
import com.baidu.mapapi.map.MapPoi;
import com.baidu.mapapi.map.MapStatus;
import com.baidu.mapapi.map.MapStatusUpdateFactory;
import com.baidu.mapapi.map.MapView;
import com.baidu.mapapi.map.Marker;
import com.baidu.mapapi.map.MarkerOptions;
import com.baidu.mapapi.map.Overlay;
import com.baidu.mapapi.map.OverlayOptions;
import com.baidu.mapapi.map.Stroke;
import com.baidu.mapapi.model.LatLng;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.blue.BluSetGeoFenceModel;
import com.meigsmart.meigapp.blue.BluetoothConnListener;
import com.meigsmart.meigapp.blue.BluetoothService;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.DrawMapModel;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.GeoFenceDialog;
import com.meigsmart.meigapp.view.MarqueeTextView;
import com.meigsmart.meigapp.view.SimpleArcDialog;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;

/**
 * 设置围栏 set/get
 *
 * @author created by 2017-11-30 on chenmeng
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueSetGeoFenceActivity extends BaseActivity implements View.OnClickListener,BluetoothConnListener,BaiduMap.OnMapLongClickListener,GeoFenceDialog.OnGeoFenceCallBack,BaiduMap.OnMarkerClickListener,BaiduMap.OnMapClickListener{
    private BlueSetGeoFenceActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.more)
    public LinearLayout mAdd;//保存
    @BindView(R.id.rightName)
    public TextView rightName;//名称
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private String serialNum = "";//序列号
    private String password = "";//密码
    private SimpleArcDialog mDialog;//dialog
    private List<BluSetGeoFenceModel.GeoFence> mList = new ArrayList<>();//S数据
    private int type = 0;//类别 0 get ;1 set
    @BindView(R.id.bmapView)
    public MapView mMapView;//mapview
    private BaiduMap mBaiduMap;
    @BindView(R.id.marqueeTv)
    MarqueeTextView mMarqueeTv;
    private GeoFenceDialog mGeoDialog;
    private List<BitmapDescriptor> mBitmap = new ArrayList<>();
    private List<DrawMapModel> mDrawMapList = new ArrayList<>();//画点的集合
    private boolean isCancelType = false;//取消类型
    @BindView(R.id.topLayout)
    public LinearLayout mTopLayout;//layout
    private boolean isFix = false;//是否修改数据了
    private int currNumber = 0;//当前画点的个数
    private int sumNumber = 0;//当前个数sum
    private boolean isDelete = false;//是否删除过点

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_set_geo_fence;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mAdd.setOnClickListener(this);
        rightName.setText(R.string.blue_set_geo_add);

        mBaiduMap = mMapView.getMap();
        mBaiduMap.setOnMarkerClickListener(this);
        mBaiduMap.setOnMapClickListener(this);
        hideView();
        initBitmap();
        setMapStatus(new LatLng(MyApplication.lat,MyApplication.lng));

        mGeoDialog = new GeoFenceDialog(this,R.style.BlueGeoFenceDialog);
        mGeoDialog.setCallBack(this);

        mDialog = new SimpleArcDialog(this, R.style.MyDialogStyle);
        mBlueService = new BluetoothService();
        mBlueService.setmListener(this);

        serialNum = getIntent().getStringExtra("serial_number");
        password = getIntent().getStringExtra("password");
        type = getIntent().getIntExtra("type",0);

        if (!TextUtils.isEmpty(serialNum)){
            if (serialNum.length()<6){
                return;
            }
            blueName = serialNum.substring(serialNum.length() - 6);
        }

        if (type == 1){
            mTitle.setText(R.string.blue_set_geo_title);
            mBaiduMap.setOnMapLongClickListener(this);
            mAdd.setVisibility(View.VISIBLE);
            mTopLayout.setVisibility(View.VISIBLE);
            mMarqueeTv.init(getWindowManager());
            mMarqueeTv.startScroll();
            mMarqueeTv.setEnabled(false);

        }else{
            mTitle.setText(R.string.blue_get_geo_title);
            mAdd.setVisibility(View.GONE);
            getGeoFence();
        }

        mDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mBlueService.stopScanBlu();
            }
        });

        mGeoDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                if (!isCancelType){
                    mBaiduMap.hideInfoWindow();
                }
            }
        });

    }

    @Override
    protected void onPause() {
        mMapView.onPause();
        super.onPause();
    }

    @Override
    protected void onResume() {
        mMapView.onResume();
        super.onResume();
    }

    @Override
    protected void onDestroy() {
        clearMap();
        super.onDestroy();
    }

    public void clearMap() {
        mMapView.getMap().clear();
        mMapView.onDestroy();

        if (mBitmap !=null && mBitmap.size()>0){
            for (BitmapDescriptor bt : mBitmap){
                bt.recycle();
            }
            mBitmap = null;

        }
        mBaiduMap = null;
    }

    private void hideView() {
        View child = mMapView.getChildAt(1);
        if (child != null && (child instanceof ImageView || child instanceof ZoomControls)) {
            child.setVisibility(View.INVISIBLE);
        }
        mMapView.showZoomControls(false);
    }

    private void initBitmap(){
        mBitmap.add(BitmapDescriptorFactory.fromResource(R.drawable.icon_marka));
        mBitmap.add(BitmapDescriptorFactory.fromResource(R.drawable.icon_markb));
        mBitmap.add(BitmapDescriptorFactory.fromResource(R.drawable.icon_markc));
        mBitmap.add(BitmapDescriptorFactory.fromResource(R.drawable.icon_markd));
        mBitmap.add(BitmapDescriptorFactory.fromResource(R.drawable.icon_marke));
    }

    private void setMapStatus(LatLng point) {
        if (mBaiduMap == null) mBaiduMap = mMapView.getMap();
        MapStatus.Builder builder = new MapStatus.Builder();
        MapStatus mapStatus = builder.target(point).zoom(20).build();
        mBaiduMap.setMapStatus(MapStatusUpdateFactory.newMapStatus(mapStatus));
    }

    private Marker drawMarker(LatLng latLng,int index){
        MarkerOptions ooA = new MarkerOptions()
                .position(latLng)
                .icon(mBitmap.get(index));
        Marker marker = (Marker) mBaiduMap.addOverlay(ooA);
        Bundle bundle = new Bundle();
        bundle.putInt("info", index);
        marker.setExtraInfo(bundle);
        return marker;
    }

    private Overlay drawCircle(LatLng latLng,int radius) {
        OverlayOptions ooCircle = new CircleOptions()
                .fillColor(0x5538A5DD)
                .center(latLng)
                .stroke(new Stroke(5, 0xAA38A5DD))
                .radius(radius);
        return mBaiduMap.addOverlay(ooCircle);
    }

    private void drawMap(LatLng latLng,int radius,int index){
        if (index >= 5 || isDelete ){
            List<Integer> l = getIndex();
            if (l.size()>0) index = l.get(0);
        }

        DrawMapModel model = new DrawMapModel();
        Marker marker = drawMarker(latLng,index);
        Overlay overlay = drawCircle(latLng,radius);
        model.setMarker(marker);
        model.setOverlay(overlay);
        model.setBt(mBitmap.get(index));
        model.setIndex(index);
        mDrawMapList.add(index,model);
    }

    private List<Integer> getIndex(){
        List<Integer> sumList = new ArrayList<>();
        List<Integer> exitList = new ArrayList<>();
        List<Integer> list = new ArrayList<>();
        sumList.add(0);sumList.add(1);sumList.add(2);sumList.add(3);sumList.add(4);

        if (mList.size()>0){
            exitList.clear();
            for (int i=0;i<mList.size();i++){
                exitList.add(mList.get(i).getIndex());
            }
        }

        for (Integer ll : sumList){
            if (!exitList.contains(ll)){
                list.add(ll);
            }
        }

        LogUtil.v("result","list:"+JSON.toJSONString(list));
        return list;
    }

    private void removeDrawMap(int index){
        if (mDrawMapList.size()>0){
            mDrawMapList.get(index).getMarker().remove();
            mDrawMapList.get(index).getOverlay().remove();
            mDrawMapList.remove(index);
        }
        LogUtil.v("result","removeDrawMap_size:"+mDrawMapList.size());
    }

    private void updateDrawMap(LatLng latLng,int radius, int index){
        if (mDrawMapList.size()>0){
            mDrawMapList.get(index).getMarker().remove();
            mDrawMapList.get(index).getOverlay().remove();
            mDrawMapList.remove(index);

            if (index >= 5 || isDelete){
                List<Integer> l = getIndex();
                if (l.size()>0) index = l.get(0);
            }


            DrawMapModel model = new DrawMapModel();
            Marker marker = drawMarker(latLng,index);
            Overlay overlay = drawCircle(latLng,radius);
            model.setMarker(marker);
            model.setOverlay(overlay);

            mDrawMapList.set(index,model);
        }
    }

    /**
     * 添加数据
     * @param latLng
     * @param name
     * @param radius
     * @param index
     */
    private void addData(LatLng latLng,String name,int radius,int index){
        BluSetGeoFenceModel.GeoFence m = new BluSetGeoFenceModel.GeoFence();
        m.setName(name);
        m.setLatitude(latLng.latitude);
        m.setLongitude(latLng.longitude);
        m.setRadius(radius);
        m.setIndex(index);
        mList.add(index,m);
        sumNumber = mList.size();
        currNumber++;
        LogUtil.v("result","list:"+JSON.toJSONString(mList));
    }

    /**
     * 更新数据
     * @param position
     */
    private void updateDataList(String name,int radius,int position){
        BluSetGeoFenceModel.GeoFence m = mList.get(position);
        m.setName(name);
        m.setRadius(radius);
        m.setIndex(position);

        mList.set(position,m);

        isFix = false;
        LogUtil.v("result","update_list:"+JSON.toJSONString(mList));
    }


    private void getGeoFence(){
        BluSetGeoFenceModel model = new BluSetGeoFenceModel();
        model.setMethod(RequestCode.BLUE_GET_GEO_FENCE);
        model.setSerial(serialNum);
        model.setPassword(password);
        String req = JSON.toJSONString(model);
        LogUtil.v("result","req:"+req);
        if (mBlueService.startScanBlue(this,req,blueName))mDialog.show();
    }

    @Override
    public void onClick(View v) {
        if (v == mBack){
            mContext.finish();
        }
        if (v == mAdd){
            BluSetGeoFenceModel model = new BluSetGeoFenceModel();
            model.setMethod(RequestCode.BLUE_SET_GEO_FENCE);
            model.setPassword(password);
            model.setSerial(serialNum);
            model.setGeo_fences(mList);

            String req = JSON.toJSONString(model);
            LogUtil.v("result","req:"+req);
            if(mBlueService.startScanBlue(mContext,req,blueName))mDialog.show();
        }
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case 1002:
                    mDialog.dismiss();
                    BluSetGeoFenceModel model = JSON.parseObject(msg.obj.toString(),BluSetGeoFenceModel.class);
                    if (msg.arg1 == 1){
                        if (model.getMethod().equals(RequestCode.BLUE_SET_GEO_FENCE)){
                            if (model.getResult()==200){
                                ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_set_apn_success));
                                mList.clear();
                                mContext.finish();
                            } else {
                                ToastUtil.showBottomShort(model.getReason());
                            }
                        }else{
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_connection_exception));
                        }
                    }else {
                        if (model.getMethod().equals(RequestCode.BLUE_GET_GEO_FENCE)){
                            if (model.getResult() == 200){
                                if (model.getGeo_fences()!=null && model.getGeo_fences().size()>0){
                                    mList.clear();
                                    mList = model.getGeo_fences();
                                    setGeoFenceMap(mList);
                                }else{
                                    ToastUtil.showBottomShort(getResources().getString(R.string.blue_geo_empty_data));
                                }
                            }else{
                                ToastUtil.showBottomShort(model.getReason());
                            }
                        }else{
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_connection_exception));
                        }
                    }
                    mBlueService.stopScanBlu();
                    break;
                case 1003:
                    ToastUtil.showBottomShort(getResources().getString(R.string.device_set_conn_fail));
                    if (mDialog.isShowing())mDialog.dismiss();
                    mBlueService.stopScanBlu();
                    break;
                case 1004:
                    ToastUtil.showBottomShort("最多5个点");
                    break;
                default:
                    break;
            }
        }
    };

    private void setGeoFenceMap(List<BluSetGeoFenceModel.GeoFence> list){
        for (int i=0;i<list.size();i++){
            LatLng latLng = new LatLng(list.get(i).getLatitude(),list.get(i).getLongitude());
            drawMarker(latLng,list.get(i).getIndex());
            drawCircle(latLng,list.get(i).getRadius());
        }
        setMapStatus(new LatLng(list.get(0).getLatitude(),list.get(0).getLongitude()));
    }

    @Override
    public void onSuccessConnect() {
    }

    @Override
    public void onCancelConnect() {
    }

    @Override
    public void onCommunication() {
    }

    @Override
    public void onReceiveData(String data) {
        Message msg = mHandler.obtainMessage();
        msg.what = 1002;
        msg.obj = data;
        msg.arg1 = type;
        mHandler.sendMessage(msg);
    }

    @Override
    public void onStopConnect() {
        mHandler.sendEmptyMessage(1003);
    }


    @Override
    public void onMapLongClick(LatLng latLng) {
        isCancelType = false;
        if (sumNumber >= 5){
            mHandler.sendEmptyMessage(1004);
        }else{
            mGeoDialog.showDialog(latLng,currNumber);
        }
    }

    @Override
    public void onSure(LatLng latLng,String name ,int radius,int index) {
        isCancelType = false;
        mGeoDialog.dismissDialog();
        if (isFix){
            updateDrawMap(latLng,radius,index);

            updateDataList(name,radius,index);
        }else{
            drawMap(latLng,radius,index);

            addData(latLng,name,radius,index);
        }
    }

    @Override
    public void onCancel() {
        mGeoDialog.dismissDialog();
        if (!isCancelType){
            mBaiduMap.hideInfoWindow();
        }
    }

    @Override
    public boolean onMarkerClick(final Marker marker) {
        if (type == 1){
            for (int i=0;i<mList.size();i++){
                View contentView = getLayoutInflater().inflate(R.layout.blue_geo_fence_marker_layout,null);
                TextView fix = (TextView) contentView.findViewById(R.id.fix);
                TextView del = (TextView) contentView.findViewById(R.id.delete);

                final int j = marker.getExtraInfo().getInt("info");
                fix.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        isFix = true;
                        isCancelType = true;
                        mGeoDialog.showDialog(new LatLng(mList.get(j).getLatitude(),mList.get(j).getLongitude()),j);
                        mGeoDialog.setName(mList.get(j).getName());
                        mGeoDialog.setRadius(String.valueOf(mList.get(j).getRadius()));
                        mBaiduMap.hideInfoWindow();
                    }
                });
                del.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        isCancelType = true;
                        removeMapPos(mList.get(j).getIndex());
                    }
                });

                LatLng latLng = marker.getPosition();
                InfoWindow mInfoWindow = new InfoWindow(contentView, latLng, -140);
                mBaiduMap.showInfoWindow(mInfoWindow);
            }
        }else {
            for (int i=0;i<mList.size();i++){
                View contentView = getLayoutInflater().inflate(R.layout.blue_geo_fence_show_marker_layout,null);
                TextView name = (TextView) contentView.findViewById(R.id.name);
                TextView radius = (TextView) contentView.findViewById(R.id.radius);

                int j = marker.getExtraInfo().getInt("info");

                name.setText(Html.fromHtml(getResources().getString(R.string.blue_set_geo_layout_name)+": &nbsp;"+mList.get(j).getName()));
                radius.setText(Html.fromHtml(getResources().getString(R.string.blue_set_geo_layout_radius)+": &nbsp;"+mList.get(j).getRadius()));

                contentView.setOnClickListener(new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        mBaiduMap.hideInfoWindow();
                    }
                });

                LatLng latLng = marker.getPosition();
                InfoWindow mInfoWindow = new InfoWindow(contentView, latLng, -140);
                mBaiduMap.showInfoWindow(mInfoWindow);
            }
        }
        return false;
    }

    @Override
    public void onMapClick(LatLng latLng) {
        mBaiduMap.hideInfoWindow();
    }

    @Override
    public boolean onMapPoiClick(MapPoi mapPoi) {
        return false;
    }

    public void removeMapPos(int pos){
        isDelete = true;
        removeDrawMap(pos);
        mList.remove(pos);
        sumNumber = mList.size();
        mBaiduMap.hideInfoWindow();
    }
}
