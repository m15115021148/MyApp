package com.meigsmart.meigapp.activity;

import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;

import butterknife.BindView;

/**
 * 二维码扫描结果 其他显示
 * @author chenMeng created by 2017.11.1
 */
public class QrOtherActivity extends BaseActivity implements View.OnClickListener {
    private QrOtherActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.result)
    public TextView mResult;//结果

    @Override
    protected int getLayoutId() {
        return R.layout.activity_qr_other;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.qr_title));
        mResult.setText(getIntent().getStringExtra("result"));
    }

    @Override
    public void onClick(View v) {
        if (v == mBack){
            mContext.finish();
        }
    }
}
