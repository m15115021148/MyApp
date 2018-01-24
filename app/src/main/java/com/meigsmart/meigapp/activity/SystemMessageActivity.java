package com.meigsmart.meigapp.activity;

import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.ExpandRecyclerViewAdapter;
import com.meigsmart.meigapp.adapter.SystemMessageAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.SystemMessageModel;
import com.meigsmart.meigapp.model.SystemMsgBean;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 系统消息
 * @author chenmeng created by 2017/12/11
 */
public class SystemMessageActivity extends BaseActivity implements View.OnClickListener {
    private SystemMessageActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.RecyclerView)
    public RecyclerView mRecycler;//RecyclerView
    private SystemMessageAdapter mAdapter;//适配器
    private List<ExpandRecyclerViewAdapter.DataBean<SystemMsgBean, SystemMsgBean.MessageModel>> mList = new ArrayList<>();//数据

    @Override
    protected int getLayoutId() {
        return R.layout.activity_system_message;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.system_message_title));

        getSystemMsg(MyApplication.clientsModel.getClient_uuid());
        initRecyclerData();
    }

    @Override
    public void onClick(View v) {
        if (v == mBack) mContext.finish();
    }

    private void initRecyclerData(){
        mRecycler.setLayoutManager(new LinearLayoutManager(this));
        mAdapter = new SystemMessageAdapter(this);
        mRecycler.setAdapter(mAdapter);
    }

    /**
     * 获取系统消息
     */
    private void getSystemMsg(String client_uuid){
        HttpManager.getApiService().getSystemMessage(client_uuid)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this,getResources().getString(R.string.loading_title)).<SystemMessageModel>transformer())
                .subscribe(new BaseObserver<SystemMessageModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        }else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                        mAdapter.setEmptyView();
                    }

                    @Override
                    protected void onSuccess(SystemMessageModel model) {
                        if (model.getResult().equals("200")){
                            if (model.getData()!=null && model.getData().size()>0){
                                getData(model);
                                mAdapter.setData(mList);
                            }else {
                                mAdapter.setEmptyView();
                            }
                        }else {
                            mAdapter.setEmptyView();
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    private List<SystemMsgBean> getData(SystemMessageModel model){
        List<SystemMsgBean> list = new ArrayList<>();

        List<SystemMessageModel.Message> data = model.getData();
        final Map<String ,List<SystemMessageModel.Message>> map = new HashMap<>();

        for (int i=0;i<data.size();i++){
            List<SystemMessageModel.Message> value = new ArrayList<>();
            String s1 = DateUtil.formatDateTime(data.get(i).getSendTime(), DateUtil.DF_YYYY_MM_DD);
            for (int j = data.size()-1;j>=0;j--){
                String s2 = DateUtil.formatDateTime(data.get(j).getSendTime(),DateUtil.DF_YYYY_MM_DD);
                if (s1.equals(s2)){
                    value.add(data.get(j));
                    map.put(s1,value);
                }
            }
        }

        Set set = map.entrySet();
        Iterator<Map.Entry> it = set.iterator();

        while (it.hasNext()){
            Map.Entry<String ,List<SystemMessageModel.Message>> entry = (Map.Entry<String ,List<SystemMessageModel.Message>>)it.next();
            SystemMsgBean bean = new SystemMsgBean();
            List<SystemMsgBean.MessageModel> l1 = new ArrayList<>();
            for (SystemMessageModel.Message m : entry.getValue()){
                SystemMsgBean.MessageModel b1 = new SystemMsgBean.MessageModel();
                b1.setTime(DateUtil.formatDateTime(m.getSendTime()).split(" ")[1].substring(0,DateUtil.formatDateTime(m.getSendTime()).split(" ")[1].length()-3));
                b1.setMessage(m.getMessage());
                l1.add(b1);
            }
            bean.setTime(entry.getKey());
            bean.setList(l1);
            list.add(bean);
        }

        //按时间排序
        Collections.sort(list, new Comparator<SystemMsgBean>() {

            public int compare(SystemMsgBean bean1, SystemMsgBean bean2) {
                long time1 = DateUtil.parseDate1(bean1.getTime()).getTime();
                long time2 = DateUtil.parseDate1(bean2.getTime()).getTime();
                return (time1 == time2 ? 0 : (time1 > time2 ? -1 : 1));
            }
        });

        for (SystemMsgBean b : list){
            //按时间排序
            Collections.sort(b.getList(), new Comparator<SystemMsgBean.MessageModel>() {

                public int compare(SystemMsgBean.MessageModel bean1, SystemMsgBean.MessageModel bean2) {
                    long time1 = DateUtil.getLongTime1(bean1.getTime());
                    long time2 = DateUtil.getLongTime1(bean2.getTime());
                    return (time1 == time2 ? 0 : (time1 > time2 ? -1 : 1));
                }
            });
            mList.add(new ExpandRecyclerViewAdapter.DataBean(b,b.getList()));
        }

        return list;
    }

}
