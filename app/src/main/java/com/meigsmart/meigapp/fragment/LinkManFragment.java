package com.meigsmart.meigapp.fragment;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.support.v4.widget.NestedScrollView;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.activity.CreateGroupActivity;
import com.meigsmart.meigapp.activity.UserAppInfoActivity;
import com.meigsmart.meigapp.activity.UserDeviceInfoActivity;
import com.meigsmart.meigapp.adapter.EasyHeaderFooterAdapter;
import com.meigsmart.meigapp.adapter.ExpandRecyclerViewAdapter;
import com.meigsmart.meigapp.adapter.FragmentLinkManAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.DeviceDeleteModel;
import com.meigsmart.meigapp.model.DeviceListModel;
import com.meigsmart.meigapp.model.GroupListDataModel;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.model.LinkManModel;
import com.meigsmart.meigapp.model.MemberDelModel;
import com.meigsmart.meigapp.model.MembersModel;
import com.meigsmart.meigapp.scan.QRActivity;
import com.meigsmart.meigapp.util.DialogUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.GuideView;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 联系人
 * Created by chenMeng on 2017/9/7.
 */

public class LinkManFragment extends BaseFragment implements View.OnClickListener,FragmentLinkManAdapter.OnLinkManCallBack{
    private Context mContext;//父类
    @BindView(R.id.createGroup)
    public RelativeLayout mCreateGroup;//create group
    @BindView(R.id.bindingDevice)
    public RelativeLayout mBindingDevice;//binging device
    @BindView(R.id.RecyclerView)
    public RecyclerView mRecycler;//RecyclerView
    private FragmentLinkManAdapter mAdapter;
    private List<ExpandRecyclerViewAdapter.DataBean<LinkManModel, LinkManModel.Person>> mList = new ArrayList<>();
    private List<ExpandRecyclerViewAdapter.DataBean<LinkManModel, LinkManModel.Person>> currList = new ArrayList<>();
    private EasyHeaderFooterAdapter mHeadAdapter;
    @BindView(R.id.scrollView)
    public NestedScrollView mSv;
    private GuideView guideView = null;

    @Override
    protected int setContentView() {
        return R.layout.fragment_link_man;
    }

    @Override
    protected void startLoad() {
        mList.clear();
        getDeviceList();
    }

    @Override
    public void onResume() {
        super.onResume();
        if (!MyApplication.isGuide ){
            if (guideView !=null){
                guideView.hide();
            }else{
                setGuideView();
            }
            guideView.show();
        }
    }

    @Override
    public void onStop() {
        super.onStop();
    }

    @Override
    protected void initData() {
        mContext = getContext();
        mCreateGroup.setOnClickListener(this);
        mBindingDevice.setOnClickListener(this);
        initRecyclerData();
    }

    private void initRecyclerData(){
        mRecycler.setLayoutManager(new LinearLayoutManager(mContext));
        mRecycler.setNestedScrollingEnabled(false);
        mAdapter = new FragmentLinkManAdapter(mContext,this);
        mRecycler.setAdapter(mAdapter);
        mAdapter.setData(mList);


//        mHeadAdapter = new EasyHeaderFooterAdapter(mAdapter);
//
//        mRecycler.setLayoutManager(new LinearLayoutManager(mContext));
//        mRecycler.setHasFixedSize(true);
//        mRecycler.setAdapter(mHeadAdapter);
//
//        setHead();
    }

    private void setHead(){
        View v = LayoutInflater.from(mContext).inflate(R.layout.fragment_linkman_head_layout, null);
        mCreateGroup = (RelativeLayout) v.findViewById(R.id.createGroup);
        mBindingDevice = (RelativeLayout) v.findViewById(R.id.bindingDevice);
        mCreateGroup.setOnClickListener(this);
        mBindingDevice.setOnClickListener(this);
        mHeadAdapter.setHeader(v);
    }

    /**
     * 获取群组列表
     */
    private void getGroupList(){
        HttpManager.getApiService().groupList(MyApplication.clientsModel.getClient_uuid())
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
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
                        if (mList.size()>0)if (mAdapter!=null)mAdapter.setData(mList);
                    }

                    @Override
                    protected void onSuccess(GroupListDataModel model) {
                        if (model.getResult().equals("200")){
                            if (model.getGroups()!=null && model.getGroups().size()>0){
                                for (GroupListModel m: model.getGroups()){
                                    LinkManModel manModel = new LinkManModel();
                                    manModel.setType(0);
                                    manModel.setName(m.getName());
                                    manModel.setGroupId(m.getGroup_uuid());
                                    List<LinkManModel.Person> list = new ArrayList<>();
                                    for (MembersModel mm : m.getMembers()){
                                        LinkManModel.Person p  = new LinkManModel.Person();
                                        p.setName(mm.getName());
                                        p.setType(Integer.parseInt(mm.getType()));
                                        p.setUuid(mm.getClient_uuid());
                                        p.setSerialNum(mm.getSerialNumber());
                                        p.setGroupUUid(m.getGroup_uuid());
                                        list.add(p);
                                    }
                                    manModel.setList(list);
                                    currList.add(new ExpandRecyclerViewAdapter.DataBean<>(manModel,manModel.getList()));
                                }
                                if (currList.size()>0){
                                    if (mAdapter!=null){
                                        mList.clear();
                                        mList.addAll(currList);
                                        mAdapter.setData(mList);
                                    }
                                }
                            }
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    /**
     * 获取设备列表
     */
    private void getDeviceList(){
        HttpManager.getApiService().getDevicesList()
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<DeviceListModel>() {

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
                    protected void onSuccess(DeviceListModel model) {
                        if (model.getResult().equals("200")){
                            if ( model.getData()!= null && model.getData().size()>0){
                                currList.clear();
                                LinkManModel manModel = new LinkManModel();
                                manModel.setType(1);
                                manModel.setName(getResources().getString(R.string.fragment_linkman_device_list));
                                manModel.setGroupId("");
                                List<LinkManModel.Person> list = new ArrayList<>();
                                for (DeviceListModel.DeviceModel m: model.getData()){
                                    LinkManModel.Person p  = new LinkManModel.Person();
                                    p.setName(m.getName());
                                    p.setType(0);
                                    p.setUuid(m.getUuid());
                                    p.setSerialNum(m.getSerialNumber());
                                    list.add(p);
                                }
                                manModel.setList(list);
                                currList.add(new ExpandRecyclerViewAdapter.DataBean<>(manModel,manModel.getList()));
                            }
                            getGroupList();
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    /**
     * 删除设备
     */
    private void deleteDevice(String device_uuid,final int groupPos,final int subItemPos){
        HttpManager.getApiService().deleteDevice(device_uuid)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(getActivity(),getResources().getString(R.string.loading_title)).<DeviceDeleteModel>transformer())
                .subscribe(new BaseObserver<DeviceDeleteModel>() {

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
                    protected void onSuccess(DeviceDeleteModel model) {
                        if (model.getResult().equals("200")){
                            if (mAdapter!=null)mAdapter.refreshItemData(groupPos,subItemPos);
                            ToastUtil.showBottomShort(getResources().getString(R.string.delete_ok));
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
    private void deleteMembers(String group_uuid, String member_client_id,final int groupPos,final int subItemPos){
        HttpManager.getApiService().deleteMember(group_uuid,member_client_id)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(getActivity(),getResources().getString(R.string.loading_title)).<MemberDelModel>transformer())
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
                            if (mAdapter!=null)mAdapter.refreshItemData(groupPos,subItemPos);
                            ToastUtil.showBottomShort(getResources().getString(R.string.delete_ok));
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    @Override
    public void onClick(View view) {
        if (view == mCreateGroup){
            Intent intent = new Intent(mContext, CreateGroupActivity.class);
            startActivityForResult(intent,111);
        }
        if (view == mBindingDevice){
            Intent intent = new Intent(mContext,QRActivity.class);
            intent.putExtra("type",1);
            startActivityForResult(intent,111);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        LogUtil.e("requestCode:"+requestCode+" resultCode:"+resultCode);
        switch (resultCode){
            case 111:
                startLoad();
                break;
        }
    }

    @Override
    public void onGroupClick(int groupItemIndex) {
//        int type = mList.get(groupItemIndex).getGroupItem().getType();
//        if (type == 0){
//            Intent intent = new Intent(mContext, MessageChatActivity.class);
//            intent.putExtra("groupId",mList.get(groupItemIndex).getGroupItem().getGroupId());
//            intent.putExtra("groupName",mList.get(groupItemIndex).getGroupItem().getName());
//            intent.putExtra("type",0);
//            startActivity(intent);
//        }
    }

    @Override
    public void onSubItemClick(int groupItemIndex, int subItemIndex) {
        if (mList.size()>0){
            int type = mList.get(groupItemIndex).getSubItems().get(subItemIndex).getType();
            if (type == 0){//device
                Intent intent = new Intent(mContext,UserDeviceInfoActivity.class);
                intent.putExtra("type",0);
                intent.putExtra("deviceSerial",mList.get(groupItemIndex).getSubItems().get(subItemIndex).getSerialNum());
                startActivity(intent);
            }else if (type == 1){
                Intent intent = new Intent(mContext,UserAppInfoActivity.class);
                intent.putExtra("type",1);
                intent.putExtra("client_uuid",mList.get(groupItemIndex).getSubItems().get(subItemIndex).getUuid());
                startActivity(intent);
            }
        }
    }

    @Override
    public void onSubItemLongClick(final int groupItemIndex, final int subItemIndex) {
        if (mList.size()>0){
            int type = mList.get(groupItemIndex).getGroupItem().getType();
            if (type == 0){//group
                if (MyApplication.clientsModel!=null){
                    if (MyApplication.clientsModel.getClient_uuid().equals(mList.get(groupItemIndex).getSubItems().get(subItemIndex).getUuid()))return;
                }

                View dialog = DialogUtil.customPromptDialog(mContext, getResources().getString(R.string.sure), getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        deleteMembers(mList.get(groupItemIndex).getGroupItem().getGroupId(),mList.get(groupItemIndex).getSubItems().get(subItemIndex).getUuid(),groupItemIndex,subItemIndex);
                    }
                },null);
                TextView t = (TextView) dialog.findViewById(R.id.dialog_tv_txt);
                if (mList.get(groupItemIndex).getSubItems().get(subItemIndex).getType() == 0){
                    t.setText(getResources().getString(R.string.group_set_delete_device_msg));
                }else{
                    t.setText(getResources().getString(R.string.group_set_delete_ok));
                }
            }else if (type ==1){//device
                View dialog = DialogUtil.customPromptDialog(mContext, getResources().getString(R.string.sure), getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        deleteDevice(mList.get(groupItemIndex).getSubItems().get(subItemIndex).getUuid(),groupItemIndex,subItemIndex);
                    }
                },null);
                TextView t = (TextView) dialog.findViewById(R.id.dialog_tv_txt);
                t.setText(getResources().getString(R.string.group_list_delete_ok));
            }
        }
    }


    private void setGuideView() {
        TextView tv = new TextView(mContext);
        tv.setText(getResources().getString(R.string.guide_first_hint));
        tv.setTextColor(getResources().getColor(R.color.white));
        tv.setTextSize(18);
        tv.setGravity(Gravity.TOP);

        guideView = GuideView.Builder
                .newInstance(mContext)
                .setTargetView(mBindingDevice)//设置目标
                .setCustomGuideView(tv)
                .setDirction(GuideView.Direction.BOTTOM)
                .setShape(GuideView.MyShape.ELLIPSE)   // 设置圆形显示区域，
                .setBgColor(getResources().getColor(R.color.shadow))
                .setOnclickListener(new GuideView.OnClickCallback() {
                    @Override
                    public void onClickedGuideView() {
                        Intent intent = new Intent(mContext,QRActivity.class);
                        intent.putExtra("type",1);
                        startActivityForResult(intent,111);
                        guideView.hide();
                    }
                })
                .build();
    }
}
