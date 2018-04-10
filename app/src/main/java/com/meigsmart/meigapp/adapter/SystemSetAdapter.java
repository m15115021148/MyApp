package com.meigsmart.meigapp.adapter;

import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.TypeModel;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;

/**
 * 系统设置页面 适配器
 * Created by chenMeng on 2017/9/7.
 */

public class SystemSetAdapter extends BaseAdapter<TypeModel> {
    private int[] res = {
            R.drawable.blue_group_invitation,R.drawable.blue_get_device_stat,R.drawable.blue_get_log,R.drawable.blue_get_log,R.drawable.blue_get_apn,
            R.drawable.blue_get_thresholds,R.drawable.blue_get_geo_fence,R.drawable.blue_set_sip,R.drawable.blue_set_server_url,
            R.drawable.blue_set_apn,R.drawable.blue_set_thresholds,R.drawable.blue_set_geo_fence,R.drawable.blue_set_pin,
            R.drawable.blue_pin_unlock,R.drawable.blue_puk_unlock,R.drawable.blue_upgrade
    };

    public SystemSetAdapter() {
    }

    public void setData(List<String> list) {
        this.list = getData(list);
        this.notifyDataSetChanged();
    }

    private List<TypeModel> getData(List<String> list){
        List<TypeModel> l = new ArrayList<>();

        for (int i=0;i<list.size();i++){
            TypeModel model = new TypeModel();
            model.setName(list.get(i));
            model.setRes(res[i]);
            l.add(model);
        }
        return l;
    }

    @Override
    protected int setView() {
        return R.layout.system_set_list_view_item;
    }

    @Override
    protected ViewHolder onBindHolder(View view) {
        return new Holder(view);
    }

    @Override
    protected void initData(int position, ViewHolder viewHolder) {
        Holder holder = (Holder) viewHolder;
        holder.name.setText(list.get(position).getName());
        holder.img.setImageResource(list.get(position).getRes());
    }

    class Holder extends ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.img)
        ImageView img;

        Holder(View view) {
            super(view);
        }
    }
}
