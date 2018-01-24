package com.meigsmart.meigapp.activity;


import android.text.TextUtils;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.DeviceEventLoadingAdapter;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.DeviceEventsListModel;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.recycler.PullLoadMoreRecyclerView;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 设备事件 列表
 *
 * @author chenmeng created by 2017/10/17
 */
public class DeviceEventActivity extends BaseActivity implements View.OnClickListener ,PullLoadMoreRecyclerView.PullLoadMoreListener {
    private DeviceEventActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.recyclerView)
    public PullLoadMoreRecyclerView mRecyclerView;//PullLoadMoreRecyclerView
    public String uuid = "";//设备uuid
    private List<DeviceEventsListModel.DeviceEventModel> mList = new ArrayList<>();//数据
    private DeviceEventLoadingAdapter mAdapter;// 适配器
    private int page = 1;//页数
    private String startTime = "";
    private String endTime = "";

    @Override
    protected int getLayoutId() {
        return R.layout.activity_device_event;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.device_event_title));
        uuid = getIntent().getStringExtra("uuid");
        initListView();
        startTime = DateUtil.getGMTDate(DateUtil.getCurrentAgeTime1(24));
        endTime = DateUtil.getGMTDate(DateUtil.getCurrentDate1());

        getEventsList(uuid,page,startTime,endTime);
    }

    @Override
    public void onClick(View view) {
        if (view == mBack) mContext.finish();
    }

    /**
     * 初始化数据
     */
    private void initListView(){
        mRecyclerView.setLinearLayout();
        mRecyclerView.setFooterViewBackgroundColor(R.color.pop_txt_bg);
        mRecyclerView.setOnPullLoadMoreListener(this);
        mAdapter = new DeviceEventLoadingAdapter(mContext);
        mRecyclerView.setAdapter(mAdapter);
    }

    /**
     * 获取事件列表
     */
    private void getEventsList(String device_uuid,int p,String startTime,String endTime) {
        Map<String,Object> map = new HashMap<>();
        map.put("begin",startTime);
        map.put("end",endTime);

        HttpManager.getApiService().getDeviceEventList(device_uuid,String.valueOf(p),HttpManager.getParameter(JSON.toJSONString(map)))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(mContext, getResources().getString(R.string.loading_title)).<DeviceEventsListModel>transformer())
                .subscribe(new BaseObserver<DeviceEventsListModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                        mRecyclerView.setVisibility(View.GONE);
                    }

                    @Override
                    protected void onSuccess(DeviceEventsListModel model) {
                        if (model.getData()!=null && model.getResult().equals("200")) {
                            mList.clear();
                            mList = model.getData();
                            if (mList.size()>0){
                                page = 1;
                                mAdapter.setData(mList);
                            }else{
                                mRecyclerView.setVisibility(View.GONE);
                                mList.clear();
                            }
                        } else {
                            mRecyclerView.setVisibility(View.GONE);
                            ToastUtil.showBottomShort(TextUtils.isEmpty(model.getReason())?getResources().getString(R.string.empty_title):model.getReason());
                        }
                    }
                });
    }

    /**
     * @param type 0刷新  1 加载更多
     * 获取事件列表
     */
    private void getEventsListMore(String device_uuid, int p, final int type, String startTime, final String endTime) {
        Map<String,Object> map = new HashMap<>();
        map.put("begin",startTime);
        map.put("end",endTime);

        HttpManager.getApiService().getDeviceEventList(device_uuid,String.valueOf(p),HttpManager.getParameter(JSON.toJSONString(map)))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<DeviceEventsListModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                        mRecyclerView.setPullLoadMoreCompleted();
                    }

                    @Override
                    protected void onSuccess(DeviceEventsListModel model) {
                        if (model.getResult().equals("200")) {
                            if (model.getData()!=null && model.getData().size()>0){
                                mRecyclerView.setVisibility(View.VISIBLE);
                                if (type==0){
                                    page = 1;
                                    mList.clear();
                                    mList = model.getData();
                                    mAdapter.setData(mList);
                                    ToastUtil.showBottomShort(getResources().getString(R.string.device_event_refresh_success));
                                }else{
                                    page++;
                                    mList.addAll(model.getData());
                                    mAdapter.setData(mList);
                                }
                            }else{
                                ToastUtil.showBottomShort(getResources().getString(R.string.device_event_load_no_data));
                            }
                            mRecyclerView.setPullLoadMoreCompleted();
                        } else {
                            mRecyclerView.setPullLoadMoreCompleted();
                            ToastUtil.showBottomShort(TextUtils.isEmpty(model.getReason())?getResources().getString(R.string.empty_title):model.getReason());
                        }
                    }
                });
    }

    @Override
    public void onRefresh() {
        page = 1;
        getEventsListMore(uuid,page,0, startTime,endTime);
    }

    @Override
    public void onLoadMore() {
        if (page == 1){
            page = page +1;
        }
        getEventsListMore(uuid,page,1,startTime,endTime);
    }
}
