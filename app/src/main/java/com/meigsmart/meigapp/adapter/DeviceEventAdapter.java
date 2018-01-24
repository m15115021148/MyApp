package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.view.View;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.DeviceEventsListModel;
import com.meigsmart.meigapp.util.DateUtil;

import java.util.List;

import butterknife.BindView;

/**
 * 设备事件 适配器
 * Created by chenMeng on 2017/10/17.
 */

public class DeviceEventAdapter extends BaseAdapter<DeviceEventsListModel.DeviceEventModel> {
    private Context mContext;

    public DeviceEventAdapter(List<DeviceEventsListModel.DeviceEventModel> list,Context context) {
        this.list = list;
        this.mContext = context;
    }

    @Override
    protected int setView() {
        return R.layout.device_event_list_item;
    }

    @Override
    protected ViewHolder onBindHolder(View view) {
        return new Holder(view);
    }

    @Override
    protected void initData(int position, ViewHolder viewHolder) {
        Holder holder = (Holder) viewHolder;
        DeviceEventsListModel.DeviceEventModel model = this.list.get(position);

        String eventType = model.getEventType();
        if (eventType.equals("tracking")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_1));
        }else if (eventType.equals("heartbeat")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_2));
        }else if (eventType.equals("shock")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_3));
        }else if (eventType.equals("geo_fence")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_4));
        }else if (eventType.equals("environment")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_5));
        }else if (eventType.equals("battery")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_6));
        }else if (eventType.equals("power_off")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_7));
        }else if (eventType.equals("help")){
            holder.name.setText(mContext.getResources().getString(R.string.device_event_type_8));
        }

        if (model.getEventTime()==null){
            holder.time.setText("");
        }else{
            holder.time.setText(DateUtil.DateToStr(DateUtil.getISODateByStr(model.getEventTime())));
        }

    }

    public class Holder extends ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.time)
        TextView time;

        Holder(View view) {
            super(view);
        }

    }
}
