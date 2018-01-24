package com.meigsmart.meigapp.activity;

import android.text.TextUtils;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.CreateGroupModel;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import java.util.HashMap;
import java.util.Map;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;

/**
 * 创建群组
 * @author chenmeng created by 2017/9/7
 */
public class CreateGroupActivity extends BaseActivity implements OnClickListener{
    private CreateGroupActivity mContext;
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.userName)
    public EditText mName;//群组名
    @BindView(R.id.create)
    public TextView mCreate;//创建


    @Override
    protected int getLayoutId() {
        return R.layout.activity_create_group;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.create_group_title));
        mCreate.setOnClickListener(this);
        mCreate.setSelected(true);
    }

    @Override
    public void onClick(View view) {
        if (view == mBack){
            mContext.finish();
        }
        if (view == mCreate){
            if (TextUtils.isEmpty(mName.getText().toString().trim())){
                ToastUtil.showBottomShort(getResources().getString(R.string.create_group_no));
                return;
            }

            Map<String,Object> map = new HashMap<>();
            map.put("owner_client_uuid",MyApplication.clientsModel.getClient_uuid());
            map.put("station_client_uuid",MyApplication.clientsModel.getClient_uuid());
            map.put("access_key",MyApplication.clientsModel.getApi_password());
            map.put("mac_address",MyApplication.diverId);
            map.put("latitude", MyApplication.lat);
            map.put("longitude",MyApplication.lng);
            map.put("name",mName.getText().toString());
            map.put("station_serial_number","ABCD01234567890");
            map.put("time_stamp", DateUtil.getCurrentDate());

            HttpManager.getApiService().createGroup(HttpManager.getParameter(JSON.toJSONString(map)))
                    .subscribeOn(Schedulers.io())
                    .observeOn(AndroidSchedulers.mainThread())
                    .compose(new DialogTransformer(mContext).<CreateGroupModel>transformer())
                    .subscribe(new BaseObserver<CreateGroupModel>() {
                        @Override
                        protected void onSuccess(CreateGroupModel model) {
                            if (model.getResult().equals("200")){
                                ToastUtil.showBottomShort(getResources().getString(R.string.create_group_success));
                                setResult(111);
                                mContext.finish();
                            }else{
                                ToastUtil.showBottomShort(getResources().getString(R.string.create_group_fail));
                            }
                        }
                    });

        }
    }
}
