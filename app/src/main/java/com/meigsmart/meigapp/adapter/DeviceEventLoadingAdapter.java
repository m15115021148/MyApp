package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.DeviceEventsListModel;
import com.meigsmart.meigapp.util.DateUtil;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * 事件列表 适配器
 * Created by chenMeng on 2017/10/18.
 */
public class DeviceEventLoadingAdapter extends RecyclerView.Adapter<DeviceEventLoadingAdapter.Holder>{
    private Context mContext;
    private List<DeviceEventsListModel.DeviceEventModel> mList = new ArrayList<>();
    private OnDeviceEventCallBack mCallBack;

    public DeviceEventLoadingAdapter(Context context){
        this.mContext = context;
    }

    public interface OnDeviceEventCallBack{
        void onItemClickListener(int position);
    }

    /**
     * 数据赋值
     * @param list
     */
    public void setData(List<DeviceEventsListModel.DeviceEventModel> list){
        this.mList = list;
        this.notifyDataSetChanged();
    }

    @Override
    public Holder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(mContext).inflate(R.layout.device_event_list_item,null);
        Holder holder = new Holder(view);
        return holder;
    }

    @Override
    public void onBindViewHolder(Holder holder, int position) {
        holder.setData(position);
    }

    @Override
    public int getItemCount() {
        return mList.size();
    }

    class Holder extends RecyclerView.ViewHolder{
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.time)
        TextView time;
        @BindView(R.id.layout)
        RelativeLayout mLayout;

        Holder(View itemView) {
            super(itemView);
            ButterKnife.bind(this,itemView);
        }

        public void setData(final int position){
            DeviceEventsListModel.DeviceEventModel model = mList.get(position);;

            String eventType = model.getEventType();
            if (eventType.equals("tracking")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_1));
            }else if (eventType.equals("heartbeat")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_2));
            }else if (eventType.equals("shock")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_3));
            }else if (eventType.equals("geo_fence")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_4));
            }else if (eventType.equals("environment")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_5));
            }else if (eventType.equals("battery")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_6));
            }else if (eventType.equals("power_off")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_7));
            }else if (eventType.equals("help")){
                name.setText(mContext.getResources().getString(R.string.device_event_type_8));
            }

            if (model.getEventTime()==null){
                time.setText("");
            }else{
                time.setText(DateUtil.GTMToLocal(DateUtil.changeByLongToGMT(model.getCreateTime())));
            }

            mLayout.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onItemClickListener(position);
                    }
                }
            });
        }
    }

}
