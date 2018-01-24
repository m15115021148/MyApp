package com.meigsmart.meigapp.activity;

import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.http.rxjava.observable.DialogTransformer;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.ClientsModel;


import java.util.HashMap;
import java.util.Map;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;

/**
 * app激活
 *
 * @author chenemng created by 2017/9/5
 */
public class RegisterAppActivity extends BaseActivity implements View.OnClickListener {
    private RegisterAppActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.register)
    public TextView mRegister;//激活

    @Override
    protected int getLayoutId() {
        return R.layout.activity_register_app;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText("激活");
        mRegister.setOnClickListener(this);
    }

    @Override
    public void onClick(View view) {
        if (view == mBack){
            mContext.finish();
        }
        if (view == mRegister){
            Map<String,Object> map = new HashMap<>();
            map.put("token","0000a64b05d5b944f4d50f72b07890aadb17ae30ceeed02d");
            map.put("lang","ja");
            map.put("bundle_id","info.e3phone.iPhone");
            map.put("name","Yasunori");

            HttpManager.getApiService().register(HttpManager.getParameter(JSON.toJSONString(map)))
                    .subscribeOn(Schedulers.io())
                    .observeOn(AndroidSchedulers.mainThread())
                    .compose(new DialogTransformer(mContext).<ClientsModel>transformer())
                    .subscribe(new BaseObserver<ClientsModel>() {
                        @Override
                        protected void onSuccess(ClientsModel logisticsInfo) {

                        }
                    });
        }
    }

}
