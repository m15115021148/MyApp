package com.meigsmart.meigapp.activity;

import android.text.TextUtils;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.util.PreferencesUtil;
import com.meigsmart.meigapp.util.ToastUtil;

import butterknife.BindView;
import butterknife.BindViews;

public class SettingsActivity extends BaseActivity implements View.OnClickListener{
    private SettingsActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.setting_baidu)
    public TextView mBaiDu;
    @BindView(R.id.setting_gaode)
    public TextView mGaoDe;
    @BindView(R.id.setting_google)
    public TextView mGoogle;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_settings;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.settings_title));
        mBaiDu.setOnClickListener(this);
        mGaoDe.setOnClickListener(this);
        mGoogle.setOnClickListener(this);
        String type = PreferencesUtil.getStringData(this,"mapType");
        if (TextUtils.isEmpty(type)){
            resetBg();
            mBaiDu.setSelected(true);
            PreferencesUtil.setStringData(this,"mapType","1");
        }else if ("2".equals(type)){
            //resetBg();
            //mGaoDe.setSelected(true);
//            PreferencesUtil.setStringData(this,"mapType","2");
        }else if ("3".equals(type)){
            resetBg();
            mGoogle.setSelected(true);
            PreferencesUtil.setStringData(this,"mapType","3");
        }else {
            resetBg();
            mBaiDu.setSelected(true);
            PreferencesUtil.setStringData(this,"mapType","1");
        }

    }

    private void resetBg(){
        mBaiDu.setSelected(false);
        mGaoDe.setSelected(false);
        mGoogle.setSelected(false);
    }

    @Override
    public void onClick(View v) {
        if(v == mBack)mContext.finish();
        if (v == mBaiDu){
            resetBg();
            mBaiDu.setSelected(true);
            PreferencesUtil.setStringData(this,"mapType","1");
        }
        if (v == mGaoDe){
           // resetBg();
            //mGaoDe.setSelected(true);
            //PreferencesUtil.setStringData(this,"mapType","2");
            ToastUtil.showBottomShort("Temporary does not support");
        }
        if (v == mGoogle){
            resetBg();
            mGoogle.setSelected(true);
            PreferencesUtil.setStringData(this,"mapType","3");
        }
    }
}
