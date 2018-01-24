package com.meigsmart.meigapp.adapter;

import android.view.View;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.DeviceListModel;

import butterknife.BindView;

/**
 * Created by chenMeng on 2017/12/25.
 */

public class DeviceBindingAdapter extends BaseAdapter<DeviceListModel.DeviceModel> {

    @Override
    protected int setView() {
        return R.layout.device_binding_layout_item;
    }

    @Override
    protected ViewHolder onBindHolder(View view) {
        return new Holder(view);
    }

    @Override
    protected void initData(int position, ViewHolder viewHolder) {
        Holder holder = (Holder) viewHolder;

        holder.name.setText(this.list.get(position).getSerialNumber());
    }

    class Holder extends ViewHolder{
        @BindView(R.id.name)
        TextView name;

        Holder(View view) {
            super(view);
        }
    }

}
