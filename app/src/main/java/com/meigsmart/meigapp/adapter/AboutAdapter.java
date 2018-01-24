package com.meigsmart.meigapp.adapter;

import android.view.View;
import android.widget.TextView;

import com.meigsmart.meigapp.R;

import java.util.List;

import butterknife.BindView;

/**
 * Created by chenMeng on 2017/9/7.
 */

public class AboutAdapter extends BaseAdapter<String> {

    public AboutAdapter(List<String> list){
        this.list = list;
    }

    @Override
    protected int setView() {
        return R.layout.about_list_view_item;
    }

    @Override
    protected ViewHolder onBindHolder(View view) {
        return new Holder(view);
    }

    @Override
    protected void initData(int position, ViewHolder viewHolder) {
        Holder holder = (Holder) viewHolder;
        holder.name.setText(list.get(position));
    }

    class Holder extends ViewHolder{
        @BindView(R.id.name)
        TextView name;

        Holder(View view) {
            super(view);
        }
    }
}
