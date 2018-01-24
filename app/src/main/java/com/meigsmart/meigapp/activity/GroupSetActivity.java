package com.meigsmart.meigapp.activity;

import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.View;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.GroupMemberAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.model.GroupUpdateModel;
import com.meigsmart.meigapp.model.MemberDelModel;
import com.meigsmart.meigapp.model.MembersModel;
import com.meigsmart.meigapp.scan.QRActivity;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.DialogUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import butterknife.BindView;
import butterknife.BindViews;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 群聊设置 页面
 * @author chenmeng created by 2017/9/16
 */
public class GroupSetActivity extends BaseActivity implements View.OnClickListener,GroupMemberAdapter.OnCallBackClickListener {
    private GroupSetActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    private GroupListModel mGroupModel;//f群聊数据
    @BindView(R.id.recyclerView)
    public RecyclerView mRecyclerView;//recyclerView
    @BindViews({R.id.groupName,R.id.clear,R.id.groupTrack})
    public List<RelativeLayout> mLayoutList;//群组名称布局 清除布局 轨迹布局
    @BindView(R.id.name)
    public TextView mName;//群组名
    @BindView(R.id.exit)
    public TextView mExit;//退出
    private List<MembersModel> mList = new ArrayList<>();//数据
    private EditText mGroupFixName;//修改群组名
    private int currPosition = 0;//当前位置
    private GroupMemberAdapter mAdapter;//适配器
    private boolean isClearData = false;//是否清理过聊天记录
    private boolean isExit = false;//是否退出群  返回列表页面

    @Override
    protected int getLayoutId() {
        return R.layout.activity_group_set;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mLayoutList.get(0).setOnClickListener(this);
        mLayoutList.get(1).setOnClickListener(this);
        mLayoutList.get(2).setOnClickListener(this);
        mExit.setOnClickListener(this);
        mExit.setSelected(true);

        mGroupModel = (GroupListModel) getIntent().getSerializableExtra("GroupListModel");

        mTitle.setText(mGroupModel.getName()+"（"+mGroupModel.getMembers().size()+"）");
        mName.setText(mGroupModel.getName());
        mList = mGroupModel.getMembers();

        mRecyclerView.setLayoutManager(new GridLayoutManager(this,4));

        initMemberData();

    }

    /**
     * 初始化数据
     */
    private void initMemberData(){
        mList.add(null);
        mAdapter = new GroupMemberAdapter(this,mList,this);
        mRecyclerView.setAdapter(mAdapter);
    }

    @Override
    public void onClick(View view) {
        if (view == mBack){
            onBackPressed();
            mContext.finish();
        }
        if (view == mLayoutList.get(0)){//群组名
            if (mGroupModel==null) return;
            final Map<String,Object> map = new HashMap<>();
            map.put("owner_client_uuid",MyApplication.clientsModel.getClient_uuid());
            map.put("station_client_uuid",MyApplication.clientsModel.getClient_uuid());
            map.put("access_key",MyApplication.clientsModel.getApi_password());
            map.put("mac_address",MyApplication.diverId);
            map.put("latitude", MyApplication.lat);
            map.put("longitude",MyApplication.lng);
            map.put("station_serial_number","ABCD01234567890");
            map.put("time_stamp", DateUtil.getCurrentDate());

            View dialog = DialogUtil.customInputDialog(mContext,
                    getResources().getString(R.string.group_set_update_name),
                    getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    if (TextUtils.isEmpty(mGroupFixName.getText().toString().trim())){
                        ToastUtil.showBottomShort(getResources().getString(R.string.group_set_name_no));
                        return;
                    }
                    map.put("name",mGroupFixName.getText().toString());
                    updateGroups(mGroupModel.getGroup_uuid(), JSON.toJSONString(map));
                }
            },null);
            mGroupFixName = (EditText) dialog.findViewById(R.id.dialog_et_txt);
        }
        if (view == mLayoutList.get(1)){//清除聊天记录
            if (mGroupModel==null) return;
            View dialog = DialogUtil.customPromptDialog(mContext, getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    MyApplication.getInstance().mDb.deleteGroupChatData(mGroupModel.getGroup_uuid());
                    isClearData = true;
                    ToastUtil.showBottomShort(getResources().getString(R.string.clear_ok));
                }
            },null);
            TextView t = (TextView) dialog.findViewById(R.id.dialog_tv_txt);
            t.setText(getResources().getString(R.string.group_set_clear_ok));
        }
        if (view == mExit){
            if (mGroupModel==null) return;
            View dialog = DialogUtil.customPromptDialog(mContext, getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    deleteMembers(mGroupModel.getGroup_uuid(),MyApplication.clientsModel.getClient_uuid());
                }
            },null);
            TextView t = (TextView) dialog.findViewById(R.id.dialog_tv_txt);
            t.setText(getResources().getString(R.string.group_set_exit));
        }
        if (view == mLayoutList.get(2)){
            if (mGroupModel==null) return;
            Intent intent = new Intent(mContext,MapActivity.class);
            intent.putExtra("type",1);
            intent.putExtra("groupId",mGroupModel.getGroup_uuid());
            startActivity(intent);
        }
    }

    /**
     * 更新群组
     */
    private void updateGroups(String group_uuid,String body){
        HttpManager.getApiService().updateGroup(group_uuid,HttpManager.getParameter(body))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(mContext,getResources().getString(R.string.loading_title)).<GroupUpdateModel>transformer())
                .subscribe(new BaseObserver<GroupUpdateModel>() {

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
                    protected void onSuccess(GroupUpdateModel model) {
                        if (model.getResult().equals("200")){
                            ToastUtil.showBottomShort(getResources().getString(R.string.group_set_update_success));
                            mName.setText(mGroupFixName.getText().toString());
                            mGroupModel.setGroup_uuid(model.getGroup_uuid());
                            mTitle.setText(mName.getText().toString()+"（"+(mGroupModel.getMembers().size()-1)+"）");
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    /**
     * 删除成员
     * @param group_uuid
     * @param member_client_id
     */
    private void deleteMembers(String group_uuid, final String member_client_id){
        HttpManager.getApiService().deleteMember(group_uuid,member_client_id)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(mContext,getResources().getString(R.string.loading_title)).<MemberDelModel>transformer())
                .subscribe(new BaseObserver<MemberDelModel>() {

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
                    protected void onSuccess(MemberDelModel model) {
                        if (model.getResult().equals("200")){
                            if (member_client_id.equals(MyApplication.clientsModel.getClient_uuid())){
                                isExit = true;
                                Intent intent = new Intent();
                                intent.putExtra("isExit",isExit);
                                setResult(101,intent);
                                mContext.finish();
                            }else{
                                mList.remove(currPosition);
                                mAdapter.notifyDataSetChanged();
                                mTitle.setText(mName.getText().toString()+"（"+(mGroupModel.getMembers().size()-1)+"）");
                                ToastUtil.showBottomShort(getResources().getString(R.string.delete_ok));
                            }
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    @Override
    public void onBackPressed() {
        Intent intent = new Intent();
        intent.putExtra("name",mName.getText().toString());
        intent.putExtra("clear",isClearData);
        intent.putExtra("isExit",isExit);
        setResult(101,intent);
        super.onBackPressed();
    }

    /**
     * 点击添加
     * @param position
     */
    @Override
    public void onClickItemListener(int position) {
        if (position == mList.size()-1){
            Intent intent = new Intent(mContext,QRActivity.class);
            startActivityForResult(intent,112);
        }else{
            if (mList.get(position).getType().equals("0")) {//设备
                Intent intent = new Intent(mContext,UserDeviceInfoActivity.class);
                intent.putExtra("type",0);
                intent.putExtra("deviceSerial",mList.get(position).getName());
                startActivity(intent);
            }else if (mList.get(position).getType().equals("1")){//app
                Intent intent = new Intent(mContext,UserAppInfoActivity.class);
                intent.putExtra("type",1);
                intent.putExtra("client_uuid",mList.get(position).getClient_uuid());
                startActivity(intent);
            }
        }
    }

    /**
     * 长按删除
     * @param position
     */
    @Override
    public void onLongClickItemListener(final int position) {
        currPosition = position;
        if (position == mList.size()-1){
        }else{
            View dialog = DialogUtil.customPromptDialog(mContext, getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialogInterface, int i) {
                    deleteMembers(mGroupModel.getGroup_uuid(),mList.get(position).getClient_uuid());
                }
            },null);
            TextView t = (TextView) dialog.findViewById(R.id.dialog_tv_txt);
            if (mList.get(position).getType().equals("0")){
                t.setText(getResources().getString(R.string.group_set_delete_device_msg));
            }else{
                t.setText(getResources().getString(R.string.group_set_delete_ok));
            }

        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        switch (requestCode) {
            // 二维码扫描结束回调
            case 112:
                if(resultCode == RESULT_OK){
                    try {
                        Bundle bundle = data.getExtras();
                        String msg = bundle.getString(QRActivity.RESULT);
                        LogUtil.v("result", msg);
                        if (msg.contains(RequestCode.QR_HEADER_ROOT)){
                            Intent intent = new Intent(mContext,UserAppInfoActivity.class);
                            intent.putExtra("type",2);
                            intent.putExtra("qr_client_uuid",msg);
                            intent.putExtra("group_uuid",mGroupModel.getGroup_uuid());
                            startActivityForResult(intent,113);
                        }else if (msg.contains(RequestCode.QR_DEVICE_ROOT)){
                            Intent intent = new Intent(mContext,UserDeviceInfoActivity.class);
                            intent.putExtra("type",2);
                            intent.putExtra("qr_serial_num",msg);
                            startActivity(intent);
                        }else{
                            Intent intent = new Intent(mContext,QrOtherActivity.class);
                            intent.putExtra("result",msg);
                            startActivity(intent);
                        }
                    }catch (Exception e){
                        e.printStackTrace();
                    }
                }
                break;
            case 113:
                if (data!=null){
                    MembersModel m = new MembersModel();
                    m.setClient_uuid(data.getStringExtra("client_uuid"));
                    m.setName(data.getStringExtra("name"));
                    m.setOnline("1");//默认在线
                    m.setType("1");//默认是app
                    if (m.getClient_uuid().contains(RequestCode.FLAG_APP_UUID)){
                        m.setType("1");
                    }else if (m.getClient_uuid().contains(RequestCode.FLAG_DEVICE_UUID)){
                        m.setType("0");
                    }
                    mList.add(mList.size()-1,m);
                    mAdapter.notifyDataSetChanged();
                    mTitle.setText(mName.getText().toString()+"（"+(mGroupModel.getMembers().size()-1)+"）");
                }
                break;
        }
    }
}
