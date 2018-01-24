package com.meigsmart.meigapp.activity;

import android.content.Intent;
import android.text.TextUtils;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.meigsmart.meigapp.view.CustomDatePicker;
import butterknife.BindView;

/**
 * FilterActivity
 * Created by chenMeng 2017/12/19
 */
public class FilterActivity extends BaseActivity implements View.OnClickListener {
    private FilterActivity mContext;//本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.startTime)
    public TextView mStartTime;
    @BindView(R.id.endTime)
    public TextView mEndTime;
    @BindView(R.id.sure)
    public TextView mSure;
    private CustomDatePicker picker;

    @Override
    protected int getLayoutId() {
        return R.layout.activity_filter;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mTitle.setText(getResources().getString(R.string.filter_title));
        mStartTime.setOnClickListener(this);
        mEndTime.setOnClickListener(this);
        mSure.setOnClickListener(this);
        mSure.setSelected(true);

        mStartTime.setText(DateUtil.formatDateTime(DateUtil.getCurrentAgeTime1(24),DateUtil.DF_YYYY_MM_DD_HH_MM));
        mEndTime.setText(DateUtil.getNYRSFDate());

        picker = new CustomDatePicker(this, new CustomDatePicker.ResultHandler() {
            @Override
            public void handle(String time,int type) {
                if (type == 0){
                    mStartTime.setText(time);
                }else{
                    mEndTime.setText(time);
                }

            }
        }, 30);
    }

    @Override
    public void onClick(View v) {
        if (v == mBack)mContext.finish();
        if (v == mStartTime){
            picker.show(0);
        }
        if (v == mEndTime){
            picker.show(1);
        }
        if (v == mSure){
            if (TextUtils.isEmpty(mStartTime.getText().toString())){
                ToastUtil.showBottomShort(getResources().getString(R.string.filter_please_start_time));
                return;
            }
            if (TextUtils.isEmpty(mEndTime.getText().toString())){
                ToastUtil.showBottomShort(getResources().getString(R.string.filter_please_end_time));
                return;
            }

            if (DateUtil.getTwoTimeInterval(mEndTime.getText().toString(),mStartTime.getText().toString())<0){
                ToastUtil.showBottomShort(getResources().getString(R.string.filter_stat_end_off));
                return;
            }

            Intent intent = new Intent();
            intent.putExtra("startTime",DateUtil.getGMTDate(DateUtil.strToDateHHMM(mStartTime.getText().toString())));
            intent.putExtra("endTime",DateUtil.getGMTDate(DateUtil.strToDateHHMM(mEndTime.getText().toString())));
            setResult(1001,intent);
            mContext.finish();
        }
    }
}
