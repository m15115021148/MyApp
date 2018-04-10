package com.meigsmart.meigapp.activity;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Dialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.location.Location;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.content.ContextCompat;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.baidu.mapapi.map.MapView;
import com.baidu.mapapi.model.LatLng;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.maps.CameraUpdate;
import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.CircleOptions;
import com.google.android.gms.maps.model.MarkerOptions;
import com.google.android.gms.maps.model.PolylineOptions;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.gps.Gps;
import com.meigsmart.meigapp.gps.MapModel;
import com.meigsmart.meigapp.gps.Point;
import com.meigsmart.meigapp.gps.PositionUtil;
import com.meigsmart.meigapp.gps.SmoothTrack;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.DeviceTrackListModel;
import com.meigsmart.meigapp.model.GroupMembersTrackModel;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.MapUtil;
import com.meigsmart.meigapp.util.PermissionUtils;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.CustomCalendar;
import com.meigsmart.siplibs.SipServiceCommand;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Spliterator;
import java.util.Timer;
import java.util.TimerTask;
import java.util.function.Consumer;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

public class MapGoogleActivity extends BaseActivity implements View.OnClickListener ,
            GoogleMap.OnMyLocationButtonClickListener,
            GoogleMap.OnMyLocationClickListener,
            OnMapReadyCallback ,
            GoogleApiClient.ConnectionCallbacks,
            GoogleApiClient.OnConnectionFailedListener{
    private MapGoogleActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    private int type = 0;//进入类型
    private TimerTask timerTask;//定时任务
    private Timer timer;//定时器
    private String sipUserName = "";//sipUserName
    private String reqJson = "{\"meigmsg\":{\"message_type\":\"active_mode\"},\"messageType\":\"active_mode\"}";
    private String deviceUUid = "";//设备uuid
    private String mStartTime = "";//开始时间
    private String mEndTime = "";//终止时间
    @BindView(R.id.time)
    public TextView mTime;//time
    @BindView(R.id.layout)
    public RelativeLayout mLayout;
    @BindView(R.id.left)
    public LinearLayout mLeft;
    @BindView(R.id.right)
    public LinearLayout mRight;
    private final int SEND_TRACK = 0x001;
    private final int SEND_MEMBER_TRACK = 0x002;
    private GoogleMap mMap;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_map_google;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTime.setOnClickListener(this);
        mLeft.setOnClickListener(this);
        mRight.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.map_title));

        SupportMapFragment mapFragment = (SupportMapFragment) getSupportFragmentManager()
                .findFragmentById(R.id.bmapView);

        mapFragment.getMapAsync(this);

        type = getIntent().getIntExtra("type", 0);

        timer = new Timer();

        mStartTime = DateUtil.getGMTDate(DateUtil.startOfTodDay(new Date()));
        mEndTime = DateUtil.getGMTDate(DateUtil.endOfTodDay(new Date()));

        mTime.setText(DateUtil.getCurrentTime());

        if (type == 0) {
            mLayout.setVisibility(View.VISIBLE);
            sipUserName = getIntent().getStringExtra("sipUserName");
            deviceUUid = getIntent().getStringExtra("uuid");

            mHandler.sendEmptyMessage(SEND_TRACK);

            timerTask = new TimerTask() {
                @Override
                public void run() {
                    if (!TextUtils.isEmpty(sipUserName))
                        SipServiceCommand.sendMessage(mContext, MyApplication.sipAccount.getIdUri(),reqJson,MyApplication.sipAccount.getRemoteUri(sipUserName));
                }
            };

            timer.schedule(timerTask, 0, 1000 * 60);

        } else if (type == 1) {
            mLayout.setVisibility(View.GONE);
            mHandler.sendEmptyMessage(SEND_MEMBER_TRACK);
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (timerTask != null) timerTask.cancel();
        if (timer != null) timer.cancel();
        if (mHandler!=null){
            mHandler.removeMessages(SEND_TRACK);
            mHandler.removeMessages(SEND_MEMBER_TRACK);
        }
    }

    @Override
    public void onClick(View view) {
        if (view == mBack) mContext.finish();
        if (view == mLeft){
            mTime.setText(DateUtil.dateToStr(DateUtil.getBeforeDay(DateUtil.strToDate(mTime.getText().toString()))));

            mStartTime = DateUtil.getGMTDate(DateUtil.startOfTodDay(DateUtil.strToDate(mTime.getText().toString())));
            mEndTime = DateUtil.getGMTDate(DateUtil.endOfTodDay(DateUtil.strToDate(mTime.getText().toString())));

            mHandler.sendEmptyMessage(SEND_TRACK);
        }
        if (view == mRight){
            mTime.setText(DateUtil.dateToStr(DateUtil.getNextDay(DateUtil.strToDate(mTime.getText().toString()))));

            mStartTime = DateUtil.getGMTDate(DateUtil.startOfTodDay(DateUtil.strToDate(mTime.getText().toString())));
            mEndTime = DateUtil.getGMTDate(DateUtil.endOfTodDay(DateUtil.strToDate(mTime.getText().toString())));

            mHandler.sendEmptyMessage(SEND_TRACK);
        }
        if (view == mTime) {
            final Dialog dialog = new Dialog(this);
            dialog.requestWindowFeature(Window.FEATURE_NO_TITLE);
            View v = getLayoutInflater().inflate(R.layout.date_time_layout,null);
            final CustomCalendar custom = (CustomCalendar) v.findViewById(R.id.cal);
            custom.setListener(new CustomCalendar.OnCalendarClickListener() {
                @Override
                public void onLeftRowClick() {
                    custom.monthChange(-1);
                }

                @Override
                public void onRightRowClick() {
                    custom.monthChange(1);
                }

                @Override
                public void onTitleClick(String monthStr, Date month) {
                }

                @Override
                public void onWeekClick(int weekIndex, String weekStr) {
                }

                @Override
                public void onDayClick(int day, String dayStr, CustomCalendar.DayFinish finish) {
                    mTime.setText(dayStr);

                    mStartTime = DateUtil.getGMTDate(DateUtil.startOfTodDay(DateUtil.strToDate(dayStr)));
                    mEndTime = DateUtil.getGMTDate(DateUtil.endOfTodDay(DateUtil.strToDate(dayStr)));

                    deviceTrack(deviceUUid, mStartTime, mEndTime);
                    dialog.dismiss();
                }
            });
            dialog.setContentView(v);
            dialog.show();
        }
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case SEND_TRACK:
                    deviceTrack(deviceUUid, mStartTime, mEndTime);
                    break;
                case SEND_MEMBER_TRACK:
                    getMembersTrack(getIntent().getStringExtra("groupId"));
                    break;
            }
        }
    };

    /**
     * 获取每个群成员的最近一个位置的列表
     */
    private void getMembersTrack(String group_uuid) {
        HttpManager.getApiService().getMembersTrack(group_uuid)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this, getResources().getString(R.string.loading_title)).<GroupMembersTrackModel>transformer())
                .subscribe(new BaseObserver<GroupMembersTrackModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                    }

                    @Override
                    protected void onSuccess(GroupMembersTrackModel model) {
                        if (model.getResult().equals("200")) {
                            if (model.getData() != null && model.getData().size() > 0) {
                                List<GroupMembersTrackModel.MembersTrackModel> list = model.getData();
                                List<MapModel> points = new ArrayList<>();
                                for (GroupMembersTrackModel.MembersTrackModel m : list) {

                                }
                            }
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    /**
     * 设备轨迹
     */
    private void deviceTrack(String device_uuid, String startTime, String endTime) {
        final Map<String, Object> map = new HashMap<>();
        map.put("begin", startTime);
        map.put("end", endTime);

        HttpManager.getApiService().getDeviceTrackList(device_uuid, HttpManager.getParameter(JSON.toJSONString(map)))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this, getResources().getString(R.string.loading_title)).<DeviceTrackListModel>transformer())
                .subscribe(new BaseObserver<DeviceTrackListModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                    }

                    @Override
                    protected void onSuccess(DeviceTrackListModel model) {
                        if (model.getResult().equals("200")) {
                            List<LatLng> list = new ArrayList<>();
                            List<Point> pointList = new ArrayList<>();//轨迹点纠偏数据;
                            if (model.getData() != null && model.getData().size() > 0) {
                                try {
                                    for (DeviceTrackListModel.DeviceTrackModel m : model.getData()) {
                                        if (!isZeroPoint(Double.parseDouble(m.getLatitude()), Double.parseDouble(m.getLongitude()))) {
                                            Point p = new Point();
                                            p.setLat(Double.parseDouble(m.getLatitude()));
                                            p.setLng(Double.parseDouble(m.getLongitude()));
                                            p.setTime(DateUtil.formatDateTime(m.getCreateTime()));
                                            p.setUnixTime(DateUtil.parseDate(DateUtil.formatDateTime(Long.parseLong(m.getTrackTime()))).getTime() / 1000);
                                            pointList.add(p);
                                        }
                                    }

                                    //按时间排序
                                    Collections.sort(pointList, new Comparator<Point>() {

                                        public int compare(Point p1, Point p2) {
                                            long time1 = DateUtil.parseDate(p1.getTime()).getTime();
                                            long time2 = DateUtil.parseDate(p2.getTime()).getTime();
                                            return (time1 == time2 ? 0 : (time1 > time2 ? 1 : -1));
                                        }
                                    });

                                    List<Point> points = SmoothTrack.doSmooth(pointList, 0.1, 1);
                                    List<Point> points1 = SmoothTrack.correcteZShape(points);
                                    List<Point> points2 = SmoothTrack.doSmooth(points1, 0.1, 1);

                                    for (Point p : points2) {
                                       // Gps gps = PositionUtil.gps_to_bd09(p.getLat(), p.getLng());
                                        list.add(new LatLng(p.getLat(),p.getLng()));
                                    }
                                } catch (Exception e) {
                                    e.printStackTrace();
                                } finally {
                                    if (list.size() > 0) {
                                        ArrayList<com.google.android.gms.maps.model.LatLng> l = new ArrayList<>();
                                        for (int i=0;i<list.size();i++){
                                            l.add(new com.google.android.gms.maps.model.LatLng(list.get(i).latitude,list.get(i).longitude));
                                        }

                                        mMap.addPolyline(
                                                new PolylineOptions()
                                                        .addAll(new MyIte(l))
                                                        .width(10)
                                                        .geodesic(true)
                                                        .color(Color.RED)
                                        );

                                    } else {
                                        ToastUtil.showBottomShort(getResources().getString(R.string.main_no_track));
                                    }

                                }
                            } else {
                                ToastUtil.showBottomShort(getResources().getString(R.string.main_no_track));
                            }

                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }



    public boolean isZeroPoint(double lat,double lng){
        if (lat!=5e-324 && lng!=5e-324 &&lat!=4.9E-324 && lng!=4.9E-324 && lat!=0 && lng!=0 && lat!=-3.067393572659021E-8 && lng!=2.890871144776878E-9){
            return false;
        }
        return true;
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == 1001) {
            if (data != null) {
                mStartTime = data.getStringExtra("startTime");
                mEndTime = data.getStringExtra("endTime");

                deviceTrack(deviceUUid, mStartTime, mEndTime);
            }
        }

    }

    @Override
    public void onMapReady(GoogleMap googleMap) {
        mMap = googleMap;

        mMap.setOnMyLocationButtonClickListener(this);
        mMap.setOnMyLocationClickListener(this);
        enableMyLocation();
//        CameraUpdate zoom = CameraUpdateFactory.zoomTo(13);
//        googleMap.animateCamera(zoom);

    }

    private void enableMyLocation() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            PermissionUtils.requestPermission(this, 1,
                    Manifest.permission.ACCESS_FINE_LOCATION, true);
        } else if (mMap != null) {
            mMap.setMyLocationEnabled(true);
        }
    }

    @Override
    public boolean onMyLocationButtonClick() {
        return false;
    }

    @Override
    public void onMyLocationClick(@NonNull Location location) {

    }

    @Override
    public void onConnected(@Nullable Bundle bundle) {

    }

    @Override
    public void onConnectionSuspended(int i) {

    }

    @Override
    public void onConnectionFailed(@NonNull ConnectionResult connectionResult) {

    }

    public class MyIte implements Iterable{
        private ArrayList<com.google.android.gms.maps.model.LatLng> ml ;

        public MyIte(ArrayList<com.google.android.gms.maps.model.LatLng> list){
            ml = list;
        }

        @NonNull
        @Override
        public Iterator iterator() {
            return ml.iterator();
        }
    }
}
