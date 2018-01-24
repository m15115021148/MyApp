package com.meigsmart.meigapp.view;

import android.app.Dialog;
import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import com.baidu.mapapi.model.LatLng;
import com.meigsmart.meigapp.R;

/**
 * Created by chenMeng on 2017/12/22.
 */

public class GeoFenceDialog extends Dialog implements View.OnClickListener{
    private TextView cancel;
    private TextView sure;
    private TextView radius;
    private TextView name;
    private int index = 0;
    private LatLng latLng;

    public GeoFenceDialog(@NonNull Context context) {
        super(context);
    }

    public GeoFenceDialog(@NonNull Context context, int themeResId) {
        super(context, themeResId);
    }

    protected GeoFenceDialog(@NonNull Context context, boolean cancelable, @Nullable OnCancelListener cancelListener) {
        super(context, cancelable, cancelListener);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setBackgroundDrawable(new ColorDrawable(android.graphics.Color.TRANSPARENT));
        setContentView(R.layout.geo_fence_dialog_layout);
        getWindow().getAttributes().gravity = Gravity.CENTER;
        setCanceledOnTouchOutside(false);
        Window window = this.getWindow() ;
        Display d = window.getWindowManager().getDefaultDisplay(); // 获取屏幕宽、高用
        WindowManager.LayoutParams p = window.getAttributes(); // 获取对话框当前的参数值
        p.width = (int) (d.getWidth() * 0.65); // 宽度设置为屏幕的0.65
        window.setAttributes(p);
        initView();
    }

    private void initView(){
        cancel = (TextView) findViewById(R.id.cancel);
        sure = (TextView) findViewById(R.id.sure);
        radius = (TextView) findViewById(R.id.radius);
        name = (TextView) findViewById(R.id.name);
        cancel.setOnClickListener(this);
        sure.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        if (v == cancel){
            if (callBack!=null) callBack.onCancel();
        }
        if (v == sure){
            if (TextUtils.isEmpty(name.getText().toString()))return;
            if (TextUtils.isEmpty(radius.getText().toString()))return;
            if (callBack!=null){
                callBack.onSure(latLng,name.getText().toString(),Integer.parseInt(radius.getText().toString()),index);
            }
        }
    }

    public void showDialog(LatLng latLng,int index){
        if (!this.isShowing()){
            this.index = index;
            this.latLng = latLng;
            this.show();
        }
    }

    public void dismissDialog(){
        setName("");
        setRadius("");
        if (this.isShowing())this.dismiss();
    }

    public void setName(String name){
        this.name.setText(name);
    }

    public void setRadius(String radius){
        this.radius.setText(radius);
    }

    private OnGeoFenceCallBack callBack;

    public void setCallBack(OnGeoFenceCallBack callBack) {
        this.callBack = callBack;
    }

    public interface OnGeoFenceCallBack{
        void onSure(LatLng latLng,String name ,int radius,int index);
        void onCancel();
    }

}
