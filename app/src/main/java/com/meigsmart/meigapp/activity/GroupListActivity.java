package com.meigsmart.meigapp.activity;

import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.GroupListAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.GroupDelModel;
import com.meigsmart.meigapp.model.GroupListDataModel;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.util.DialogUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.LRecyclerView;
import com.meigsmart.meigapp.view.SwipeMenuView;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 群组列表
 * @author chenmeng created by 2017/9/7
 */
public class GroupListActivity extends BaseActivity implements View.OnClickListener,GroupListAdapter.OnCallBackDel {
    private GroupListActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.recyclerView)
    public LRecyclerView mRecyclerView;//RecyclerView
    private List<GroupListModel> mList = new ArrayList<>();//数据
    private GroupListAdapter mAdapter;//适配器
    @BindView(R.id.more)
    public LinearLayout mMore;//右侧按钮
    @BindView(R.id.rightName)
    public TextView rightName;//内容 右侧
    @BindView(R.id.empty)
    public TextView mEmpty;//空布局

    @Override
    protected int getLayoutId() {
        return R.layout.activity_group_list;
    }

    @Override
    protected void onResume() {
        super.onResume();
        getGroupList();
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.group_list_title));
        mMore.setOnClickListener(this);
        mMore.setVisibility(View.VISIBLE);
        rightName.setText(getResources().getString(R.string.group_list_create));

        mRecyclerView.setLayoutManager(new LinearLayoutManager(this));
        initListViewData();
    }

    /**
     * 初始化数据
     */
    private void initListViewData(){
        mAdapter = new GroupListAdapter(mList,this);
        mRecyclerView.setAdapter(mAdapter);
    }


    @Override
    public void onClick(View view) {
        if (view == mBack){
            mContext.finish();
        }
        if (view == mMore){
            Intent intent = new Intent(mContext, CreateGroupActivity.class);
            startActivity(intent);
        }
    }

    /**
     * 获取群组列表
     */
    private void getGroupList(){
        HttpManager.getApiService().groupList(MyApplication.clientsModel.getClient_uuid())
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this,getResources().getString(R.string.loading_title)).<GroupListDataModel>transformer())
                .subscribe(new BaseObserver<GroupListDataModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        }else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                    }

                    @Override
                    protected void onSuccess(GroupListDataModel model) {
                        if (model.getResult().equals("200")){
                            if (model.getGroups()!=null && model.getGroups().size()>0){
                                mRecyclerView.setVisibility(View.VISIBLE);
                                mEmpty.setVisibility(View.GONE);
                                mList = model.getGroups();
                                mAdapter.setData(mList);
                                mAdapter.notifyDataSetChanged();
                            }else{
                                mList.clear();
                                mRecyclerView.setVisibility(View.GONE);
                                mEmpty.setVisibility(View.VISIBLE);
                            }
                        }else{
                            mRecyclerView.setVisibility(View.GONE);
                            mEmpty.setVisibility(View.VISIBLE);
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }


    /**
     * 删除群组请求
     */
    private void delGroup(final String id,final int position){
        HttpManager.getApiService().delGroup(id)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(this,getResources().getString(R.string.loading_title)).<GroupDelModel>transformer())
                .subscribe(new BaseObserver<GroupDelModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        }else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                    }

                    @Override
                    protected void onSuccess(GroupDelModel model) {
                        if (model.getResult().equals("200")){
                            mList.remove(position);
                            mAdapter.notifyDataSetChanged();
                            if (mList.size()==0){
                                mRecyclerView.setVisibility(View.GONE);
                                mEmpty.setVisibility(View.VISIBLE);
                            }
                            MyApplication.getInstance().mDb.deleteGroupChatData(id);
                            ToastUtil.showBottomShort(getResources().getString(R.string.delete_ok));
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });

    }

    /**
     * 删除群组
     * @param position
     */
    @Override
    public void onDel(final SwipeMenuView view,final int position) {
        View dialog = DialogUtil.customPromptDialog(mContext, getResources().getString(R.string.sure), getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                view.quickClose();
                delGroup(mList.get(position).getGroup_uuid(), position);
            }
        }, new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialogInterface, int i) {
                view.quickClose();
            }
        });
        TextView t = (TextView) dialog.findViewById(R.id.dialog_tv_txt);
        t.setText(getResources().getString(R.string.group_list_delete_ok));

    }

    /**
     * 点击事件
     * @param position
     */
    @Override
    public void onItemView(int position) {
        Intent intent = new Intent(mContext, MessageChatActivity.class);
        Bundle b = new Bundle();
        b.putSerializable("GroupListModel",mList.get(position));
        intent.putExtras(b);
        intent.putExtra("type",1);//1列表进入  0 推送进入
        startActivity(intent);
    }
}
