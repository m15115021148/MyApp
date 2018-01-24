package com.meigsmart.meigapp.adapter;

import android.graphics.Color;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.DeviceListModel.DeviceModel;
import com.meigsmart.meigapp.view.SwipeMenuView;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * 设备列表适配器
 * Created by chenMeng on 2017/9/15.
 */
public class DeviceListAdapter extends RecyclerView.Adapter<DeviceListAdapter.Holder>{
    private List<DeviceModel> mList;
    private OnCallBackDel mCallBack;

    public interface OnCallBackDel{
        void onDel(SwipeMenuView view, int position,int type);//1 事件  0 删除
        void onItemView(SwipeMenuView view,int position);
    }

    public DeviceListAdapter(List<DeviceModel> list, OnCallBackDel callBack){
        this.mList = list;
        this.mCallBack = callBack;
    }

    /**
     * 数据
     */
    public void setData(List<DeviceModel> list){
        this.mList = list;
        this.notifyDataSetChanged();
    }

    @Override
    public Holder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.device_list_item,null);
        Holder holder = new Holder(view);
        return holder;
    }

    @Override
    public void onBindViewHolder(Holder holder, int position) {
        holder.initData(position);
    }

    @Override
    public int getItemCount() {
        return mList.size();
    }

    class Holder extends RecyclerView.ViewHolder{
        @BindView(R.id.name) TextView name;
        @BindView(R.id.delete) TextView del;
        @BindView(R.id.swipeView) SwipeMenuView mSwipe;
        @BindView(R.id.layout) RelativeLayout mItem;
        @BindView(R.id.isSelect) ImageView select;
        @BindView(R.id.event) TextView event;

        Holder(View view) {
            super(view);
            ButterKnife.bind(this,view);
        }

        public void initData(final int position){
            DeviceModel model = mList.get(position);
            if (model==null) return;
            name.setText(model.getSerialNumber());

            if (model.isSelect()){
                mItem.setBackgroundColor(Color.parseColor("#87CEFA"));
                select.setVisibility(View.VISIBLE);
            }else{
                mItem.setBackgroundColor(Color.parseColor("#ffffff"));
                select.setVisibility(View.GONE);
            }

            del.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onDel(mSwipe,position,0);
                    }
                }
            });
            mItem.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onItemView(mSwipe,position);
                    }
                }
            });
            event.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onDel(mSwipe,position,1);
                    }
                }
            });
        }
    }
}
