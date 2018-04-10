package com.meigsmart.meigapp.fragment;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.activity.MapActivity;
import com.meigsmart.meigapp.activity.MapGaoDeActivity;
import com.meigsmart.meigapp.activity.MapGoogleActivity;
import com.meigsmart.meigapp.activity.MessageChatActivity;
import com.meigsmart.meigapp.adapter.ExpandRecyclerViewAdapter;
import com.meigsmart.meigapp.adapter.FragmentMessageAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.GroupListDataModel;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.model.MembersModel;
import com.meigsmart.meigapp.util.PreferencesUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;


/**
 * 消息fragment
 * Created by chenMeng on 2017/9/7.
 */
public class MessageFragment extends BaseFragment implements FragmentMessageAdapter.onMessageCallBack {
    private Context mContext;//本类
    @BindView(R.id.RecyclerView)
    public RecyclerView mRecycler;//RecyclerView
    private FragmentMessageAdapter mAdapter;
    private List<ExpandRecyclerViewAdapter.DataBean<GroupListModel, MembersModel>> mList = new ArrayList<>();

    @Override
    protected int setContentView() {
        return R.layout.fragment_message;
    }

    @Override
    protected void startLoad() {
        getGroupList();
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        mList.clear();
    }

    @Override
    protected void initData() {
        mContext = getContext();
        initRecyclerData();
    }

    private void initRecyclerData() {
        mRecycler.setLayoutManager(new LinearLayoutManager(mContext));
        mAdapter = new FragmentMessageAdapter(mContext, this);
        mRecycler.setAdapter(mAdapter);
        mAdapter.setData(mList);
    }

    /**
     * 获取群组列表
     */
    private void getGroupList() {
        HttpManager.getApiService().groupList(MyApplication.clientsModel.getClient_uuid())
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<GroupListDataModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        } else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                        if (mAdapter != null) mAdapter.setEmptyView();
                    }

                    @Override
                    protected void onSuccess(GroupListDataModel model) {
                        if (model.getResult().equals("200")) {
                            if (model.getGroups()!=null && model.getGroups().size() > 0) {
                                List<GroupListModel> groups = model.getGroups();
                                for (int i=0;i<groups.size();i++){
                                    if (mList.size()>0 && mList.size() == groups.size()){
                                        groups.get(i).setSelect(mList.get(i).getGroupItem().getSelect());
                                    }
                                }
                                mList.clear();
                                for (GroupListModel m : groups) {
                                    List<MembersModel> list = new ArrayList<>();
                                    for (MembersModel mm : m.getMembers()) {
                                        if ("0".equals(mm.getType())) {
                                            list.add(mm);
                                        }
                                    }
                                    mList.add(new ExpandRecyclerViewAdapter.DataBean<>(m, list));
                                }
                                if (mAdapter != null) mAdapter.setData(mList);
                            } else {
                                if (mAdapter != null) mAdapter.setEmptyView();
                            }
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                            if (mAdapter != null) mAdapter.setEmptyView();
                        }
                    }
                });
    }

    @Override
    public void onGroupClick(int groupItemIndex) {
        if (mList.size() > 0) {
            Intent intent = new Intent(mContext, MessageChatActivity.class);
            Bundle b = new Bundle();
            b.putSerializable("GroupListModel", mList.get(groupItemIndex).getGroupItem());
            intent.putExtras(b);
            intent.putExtra("type", 1);//1列表进入  0 推送进入
            startActivityForResult(intent, 111);
        }
    }

    @Override
    public void onSubItemClick(int groupItemIndex, int subItemIndex) {
        if (mList.size() > 0 && mList.get(groupItemIndex).getSubItems().get(subItemIndex).getType().equals("0")) {
            Intent intent = new Intent();
            if ("3".equals(PreferencesUtil.getStringData(mContext,"mapType"))){
                intent.setClass(mContext,MapGoogleActivity.class);
            }else{
                intent.setClass(mContext,MapActivity.class);
            }
            intent.putExtra("uuid", mList.get(groupItemIndex).getSubItems().get(subItemIndex).getClient_uuid());
            intent.putExtra("sipUserName", mList.get(groupItemIndex).getSubItems().get(subItemIndex).getSipUsername());
            intent.putExtra("type", 0);
            startActivity(intent);
        }
    }

    @Override
    public void onSubItemLongClick(int groupItemIndex, int subItemIndex) {
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            case 111:
                startLoad();
                break;
        }
    }
}
