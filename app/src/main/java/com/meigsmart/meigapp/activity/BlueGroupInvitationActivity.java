package com.meigsmart.meigapp.activity;

import android.annotation.SuppressLint;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.os.Message;
import android.support.annotation.RequiresApi;
import android.text.TextUtils;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.GroupInvitationAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.blue.BluGroupInvitationModel;
import com.meigsmart.meigapp.blue.BluetoothConnListener;
import com.meigsmart.meigapp.blue.BluetoothService;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.GroupInvitationModel;
import com.meigsmart.meigapp.model.GroupListDataModel;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.SimpleArcDialog;

import java.net.ConnectException;
import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;

/**
 * 加入群组 页面
 *
 * @author chenmeng created by 2017/9/6
 */
@RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
public class BlueGroupInvitationActivity extends BaseActivity implements View.OnClickListener,BluetoothConnListener,GroupInvitationAdapter.OnGroupInvitationListener {
    private BlueGroupInvitationActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.listView)
    public ListView mLv;//listView
    private BluetoothService mBlueService;
    private String blueName = "";//蓝牙名称
    private String serialNum = "";//序列号
    private String password = "";//密码
    private SimpleArcDialog mDialog;//dialog
    private List<GroupListModel> mGroupList = new ArrayList<>();//群组数据
    private GroupInvitationAdapter mAdapter;//适配器
    private TextView mGroupJoin;//加入
    private int currPosition = 0;//当前选择的群组
    @BindView(R.id.more)
    public LinearLayout mMore;//右侧按钮
    @BindView(R.id.rightName)
    public TextView rightName;//内容 右侧

    @Override
    protected int getLayoutId() {
        return R.layout.activity_blue_group_invitation;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.blue_group_invitation_title));
        mMore.setOnClickListener(this);
        mMore.setVisibility(View.VISIBLE);
        rightName.setText(getResources().getString(R.string.group_list_create));

        serialNum = getIntent().getStringExtra("serial_number");
        password = getIntent().getStringExtra("password");

        if (!TextUtils.isEmpty(serialNum)){
            if (serialNum.length()<6){
                return;
            }
            blueName = serialNum.substring(serialNum.length() - 6);
        }
        mDialog = new SimpleArcDialog(this, R.style.MyDialogStyle);

        mBlueService = new BluetoothService();
        mBlueService.setmListener(this);

        initListView();

        mDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mBlueService.stopScanBlu();
            }
        });

    }

    @Override
    protected void onResume() {
        super.onResume();
        getGroupList();
    }

    private void initListView(){
        mAdapter = new GroupInvitationAdapter(this);
        mLv.setAdapter(mAdapter);
    }

    private void addFooter(){
        View view = getLayoutInflater().inflate(R.layout.blue_geo_fence_footer_layout,null);
        mGroupJoin = (TextView) view.findViewById(R.id.save);
        mGroupJoin.setText(getResources().getString(R.string.blue_group_invitation_invite));
        mGroupJoin.setOnClickListener(this);
        mLv.addFooterView(view);
    }

    @Override
    public void onClick(View v) {
        if (v == mBack)mContext.finish();
        if (v == mGroupJoin){
            if (mGroupJoin.isSelected()){
                if (!mDialog.isShowing()) mDialog.show();
                groupInvitation(mAdapter.getList().get(currPosition).getGroup_uuid());
            }else{
                ToastUtil.showBottomShort(getResources().getString(R.string.blue_group_invitation_please_group));
            }
        }
        if (v == mMore){
            Intent intent = new Intent(mContext, CreateGroupActivity.class);
            startActivity(intent);
        }
    }

    /**
     * 获取群组列表
     */
    private void groupInvitation(String group_uuid){
        HttpManager.getApiService().groupInvitation(group_uuid)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<GroupInvitationModel>() {

                    @Override
                    public void onError(Throwable e) {
                        if (e instanceof HttpException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_error));
                        }else if (e instanceof ConnectException) {
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_noNet));
                        } else {//其他或者没网会走这里
                            ToastUtil.showBottomShort(getResources().getString(R.string.http_exception));
                        }
                        MyApplication.setEmptyShowText(mContext,mLv,getResources().getString(R.string.empty_title));
                        if (mDialog.isShowing())mDialog.dismiss();
                    }

                    @Override
                    protected void onSuccess(GroupInvitationModel model) {
                        if (model.getResult().equals("200")){
                            if (model.getReason().equals("Accepted")){
                                BluGroupInvitationModel m = new BluGroupInvitationModel();

                                BluGroupInvitationModel.GroupInvitation g = new BluGroupInvitationModel.GroupInvitation();
                                g.setGroup_uuid(mAdapter.getList().get(currPosition).getGroup_uuid());
                                g.setToken(model.getToken());

                                m.setMethod(RequestCode.BLUE_GROUP_INVITATION);
                                m.setSerial(serialNum);
                                m.setPassword(password);
                                m.setGroupInvitation(g);

                                String req = JSON.toJSONString(m).replace("groupInvitation","GroupInvitation");
                                LogUtil.v("result","req:"+req);

                                mBlueService.startScanBlue(mContext,req,blueName);
                            }
                        }else{
                            if (mDialog.isShowing())mDialog.dismiss();
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
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
                        MyApplication.setEmptyShowText(mContext,mLv,getResources().getString(R.string.empty_title));
                    }

                    @Override
                    protected void onSuccess(GroupListDataModel model) {
                        if (model.getResult().equals("200")){
                            if (model.getGroups()!=null && model.getGroups().size()>0){
                                mGroupList = model.getGroups();
                                mAdapter.setList(mGroupList);
                                mAdapter.notifyDataSetChanged();
                                addFooter();
                            }else{
                                MyApplication.setEmptyShowText(mContext,mLv,getResources().getString(R.string.empty_title));
                            }
                        }else{
                            ToastUtil.showBottomShort(model.getReason());
                            MyApplication.setEmptyShowText(mContext,mLv,getResources().getString(R.string.empty_title));
                        }
                    }
                });
    }

    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler(){
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what){
                case 1002:
                    if (mDialog.isShowing())mDialog.dismiss();
                    BluGroupInvitationModel model = JSON.parseObject(msg.obj.toString(),BluGroupInvitationModel.class);
                    if (model.getMethod().equals(RequestCode.BLUE_GROUP_INVITATION)){
                        if (model.getResult()==200){
                            ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_group_invitation_success));
                            mContext.finish();
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }else{
                        ToastUtil.showBottomShort(mContext.getResources().getString(R.string.blue_connection_exception));
                    }
                    mBlueService.stopScanBlu();
                    break;
                case 1003:
                    ToastUtil.showBottomShort(getResources().getString(R.string.device_set_conn_fail));
                    if (mDialog.isShowing())mDialog.dismiss();
                    mBlueService.stopScanBlu();
                    break;
                default:
                    break;
            }
        }
    };

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
        mHandler.sendMessage(msg);
    }

    @Override
    public void onStopConnect() {
        mHandler.sendEmptyMessage(1003);
    }

    @Override
    public void onItemClick(int position) {
        currPosition = position;
        if (mGroupList.get(position).getSelect() == 0){
            for (GroupListModel model : mGroupList){
                model.setSelect(0);
            }
            mGroupList.get(position).setSelect(1);
            mGroupJoin.setSelected(true);
        }else{
            mGroupJoin.setSelected(false);
            mGroupList.get(position).setSelect(0);
        }

        mAdapter.notifyDataSetChanged();
    }
}
