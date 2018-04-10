package com.meigsmart.meigapp.fragment;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.text.TextUtils;
import android.view.View;
import android.widget.AdapterView;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.activity.AboutActivity;
import com.meigsmart.meigapp.activity.SettingsActivity;
import com.meigsmart.meigapp.activity.SystemMessageActivity;
import com.meigsmart.meigapp.activity.UserAppInfoActivity;
import com.meigsmart.meigapp.adapter.MainLeftListViewAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.ClientsUpdateModel;
import com.meigsmart.meigapp.model.UpdateModel;
import com.meigsmart.meigapp.util.DialogUtil;
import com.meigsmart.meigapp.util.PreferencesUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import java.net.ConnectException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import butterknife.BindArray;
import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;
import retrofit2.HttpException;


/**
 * setting_choice_normal
 * Created by chenMeng on 2017/9/7.
 */

public class SettingFragment extends BaseFragment implements View.OnClickListener{
    private Context mContext;//本类
    @BindView(R.id.userName)
    public TextView mUserName;//用户名
    @BindView(R.id.appQr)
    public ImageView mAppQr;//二维码
    private EditText mFixName;//修改名字
    private MainLeftListViewAdapter mLeftAdapter;//适配器
    @BindView(R.id.listView)
    public ListView mLeftLv;//左侧布局listview控件
    @BindArray(R.array.MainLeftData) String[] data;//左侧标题数据
    private static final int[] resImg = {R.drawable.title_group,R.drawable.title_userinfo,R.drawable.title_set,R.drawable.title_about,R.drawable.my_40};

    @Override
    protected int setContentView() {
        return R.layout.fragment_setting;
    }

    @Override
    public void onResume() {
        super.onResume();
        mUserName.setText(MyApplication.userName);
    }

    @Override
    protected void startLoad() {
        updateApp();
    }

    @Override
    protected void initData() {
        mContext = getContext();
        mUserName.setOnClickListener(this);
        mAppQr.setOnClickListener(this);
        initLeftListViewData();
    }

    /**
     * 初始化左侧数据
     */
    private void initLeftListViewData(){
        mLeftAdapter = new MainLeftListViewAdapter(Arrays.asList(data),resImg);
        mLeftLv.setAdapter(mLeftAdapter);
        mLeftLv.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> adapterView, View view, int position, long l) {
                switch (position){
//                    case 0:
//                        Intent group = new Intent(mContext,GroupListActivity.class);
//                        startActivity(group);
//                        break;
                    case 0:
                        Intent user = new Intent(mContext,UserAppInfoActivity.class);
                        user.putExtra("type",0);
                        startActivity(user);
                        break;
                    case 1:
                        Intent system = new Intent(mContext,SystemMessageActivity.class);
                        startActivity(system);
                        break;
                    case 2:
                        Intent about = new Intent(mContext,AboutActivity.class);
                        startActivity(about);
                        break;
                    case 3:
                        Intent set = new Intent(mContext,SettingsActivity.class);
                        startActivity(set);
                        break;
                    default:
                        break;
                }
            }
        });
    }

    @Override
    public void onClick(View v) {
        if (v == mUserName){
            View dialog = DialogUtil.customInputDialog(mContext,
                    getResources().getString(R.string.user_info_fix_name),
                    getResources().getString(R.string.sure),getResources().getString(R.string.cancel), new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i) {
                            if (TextUtils.isEmpty(mFixName.getText().toString().trim())){
                                ToastUtil.showBottomShort(getResources().getString(R.string.group_set_name_no));
                                return;
                            }
                            Map<String,Object> map = new HashMap<>();
                            map.put("token",MyApplication.diverId);
                            map.put("lang", RequestCode.USER_LANG);
                            map.put("bundle_id",RequestCode.USER_BUNDLE_ID);
                            map.put("name",mFixName.getText().toString());
                            updateName(MyApplication.clientsModel.getClient_uuid(), JSON.toJSONString(map));
                        }
                    },null);
            mFixName = (EditText) dialog.findViewById(R.id.dialog_et_txt);
            mFixName.setText(mUserName.getText().toString());
        }
        if (v == mAppQr){
            Intent intent = new Intent(mContext,UserAppInfoActivity.class);
            intent.putExtra("type",0);
            startActivity(intent);
        }
    }

    /**
     * 修改名称
     */
    private void updateName(String client_uuid, String body) {
        HttpManager.getApiService().updateAppInfo(client_uuid, HttpManager.getParameter(body))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .compose(new DialogTransformer(getActivity(), getResources().getString(R.string.loading_title)).<ClientsUpdateModel>transformer())
                .subscribe(new BaseObserver<ClientsUpdateModel>() {

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
                    protected void onSuccess(ClientsUpdateModel model) {
                        if (model.getResult().equals("200")) {
                            mUserName.setText(mFixName.getText().toString());
                            PreferencesUtil.setStringData(mContext,"userName",mFixName.getText().toString());
                            MyApplication.userName = PreferencesUtil.getStringData(mContext,"userName");
                            ToastUtil.showBottomShort(getResources().getString(R.string.group_set_update_success));
                        } else {
                            ToastUtil.showBottomShort(model.getReason());
                        }
                    }
                });
    }

    /**
     * 版本更新
     */
    private void updateApp(){
        HttpManager.getApiService().updateApp(RequestCode.APP_KEY,RequestCode.APP_SECRET,MyApplication.getInstance().getVersionCode())
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<UpdateModel>() {
                    @Override
                    public void onError(Throwable e) {
                    }
                    @Override
                    protected void onSuccess(UpdateModel model) {
                        if (model.getResult().equals("200")){
                            if (Integer.parseInt(model.getVersionCode())>Integer.parseInt(MyApplication.getInstance().getVersionCode())){
                                mLeftAdapter.updateStatus(2,1);
                            }else{
                                mLeftAdapter.updateStatus(2,0);
                            }
                        }
                    }
                });
    }
}
