package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.TypeModel;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;

/**
 * 首页左侧布局 适配器
 * Created by chenMeng on 2017/9/6.
 */

public class MainLeftListViewAdapter extends BaseAdapter<TypeModel> {
    private List<TypeModel> mList;

    public MainLeftListViewAdapter(List<String> list, int[] resImg) {
        mList = new ArrayList<>();
        for (int i=0;i<list.size();i++){
            if (i!=0){
                TypeModel model = new TypeModel();
                model.setFlag(0);
                model.setName(list.get(i));
                model.setRes(resImg[i]);
                mList.add(model);
            }
        }
        this.list = mList;
    }

    /**
     * 更新状态
     * @param pos
     * @param flag
     */
    public void updateStatus(int pos,int flag){
        this.list.get(pos).setFlag(flag);
        this.notifyDataSetChanged();
    }

    @Override
    protected int setView() {
        return R.layout.main_left_list_view_item;
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
        if (list.get(position).getFlag()==0){
            holder.msg.setVisibility(View.GONE);
        }else {
            holder.msg.setVisibility(View.VISIBLE);
        }
    }

    class Holder extends ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.img)
        ImageView img;
        @BindView(R.id.message)
        ImageView msg;

        Holder(View view) {
            super(view);
        }
    }
}
