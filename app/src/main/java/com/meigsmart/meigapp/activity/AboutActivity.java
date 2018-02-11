package com.meigsmart.meigapp.activity;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.adapter.AboutAdapter;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.model.UpdateModel;
import com.meigsmart.meigapp.util.ToastUtil;

import java.util.Arrays;

import butterknife.BindArray;
import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.schedulers.Schedulers;

public class AboutActivity extends BaseActivity implements View.OnClickListener {
    private AboutActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.listView)
    public ListView mLv;//listView
    @BindArray(R.array.AboutData) String[] data;//获取数据
    @BindView(R.id.version)
    public TextView mCurrVersion;//当前版本号
    @BindView(R.id.newVersion)
    public TextView mNewVersion;//最新版本号
    @BindView(R.id.flagPoint)
    public ImageView mRedFlag;//小红点
    @BindView(R.id.updateLayout)
    public RelativeLayout mUpdateLayout;//版本更新
    private Dialog dialog;//dialog
    private String updateContent;//更新内容
    private String url = "";//下载地址
    private boolean isUpdate = false;//是否更新
    private String newVersion;//最新版本
    private MyApplication app;//application对象

    @Override
    protected int getLayoutId() {
        return R.layout.activity_about;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mUpdateLayout.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.about_title));
        app = (MyApplication) getApplication();
//        initListViewData();
        mCurrVersion.setText("V "+MyApplication.getInstance().getVersionName());
        updateApp();
    }

    /**
     * 初始化listview数据
     */
    private void initListViewData() {
        AboutAdapter adapter = new AboutAdapter(Arrays.asList(data));
        mLv.setAdapter(adapter);
    }

    @Override
    public void onClick(View view) {
        if (view == mBack) {
            mContext.finish();
        }
        if (view == mUpdateLayout){

            if (!isUpdate){
                ToastUtil.showBottomShort(getResources().getString(R.string.aboud_update_tips));
                return;
            }

            dialog = new Dialog(this,R.style.TimeDateStyle);

            View v = getLayoutInflater().inflate(R.layout.update_version_layout, null);

            TextView content = (TextView) v.findViewById(R.id.content);
            TextView cancel = (TextView) v.findViewById(R.id.cancel);
            TextView sure = (TextView) v.findViewById(R.id.sure);

            content.setText(updateContent);

            cancel.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    dialog.dismiss();
                }
            });
            sure.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    Intent intent = new Intent(mContext,NotificationUpdateActivity.class);
                    startActivity(intent);
                    app.setDownload(true);
                    dialog.dismiss();
                }
            });

            // 设置dialog的布局,并显示
            dialog.setContentView(v);
            dialog.show();
        }
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
                                mRedFlag.setVisibility(View.VISIBLE);
                                newVersion = model.getVersionName();
                                isUpdate = true;
                            }else{
                                mRedFlag.setVisibility(View.GONE);
                                newVersion = MyApplication.getInstance().getVersionName();
                                isUpdate = false;
                            }
                            MyApplication.versionName = newVersion;
                            MyApplication.downUrl = model.getUrl();
                            mNewVersion.setText("V "+newVersion);
                            updateContent = model.getRemark();
                            url = model.getUrl();
                        }
                    }
                });
    }
}
