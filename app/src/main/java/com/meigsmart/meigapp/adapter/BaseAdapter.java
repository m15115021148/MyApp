package com.meigsmart.meigapp.adapter;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

import butterknife.ButterKnife;

/**
 * 适配器adapter 的基类
 * Created by chenM on 2017/9/4.
 */
public abstract class BaseAdapter<T> extends android.widget.BaseAdapter {
    protected List<T> list = new ArrayList<>();//数据
    private ViewHolder holder;//holder

    @Override
    public int getCount() {
        return list.size();
    }

    @Override
    public Object getItem(int position) {
        return list.get(position);
    }

    @Override
    public long getItemId(int position) {
        return position;
    }

    @Override
    public View getView(int position, View convertView, ViewGroup parent) {
        if (convertView == null){
            convertView = LayoutInflater.from(parent.getContext()).inflate(setView(),null);
            holder = onBindHolder(convertView);
            convertView.setTag(holder);
        }else{
            holder = (ViewHolder) convertView.getTag();
        }
        initData(position,holder);
        return convertView;
    }

    public List<T> getList() {
        return list;
    }

    public void setList(List<T> list) {
        this.list = list;
    }

    protected abstract int setView();//设置视图

    protected abstract ViewHolder onBindHolder(View view);//绑定holder

    protected abstract void initData(int position,ViewHolder viewHolder);//初始化数据

    //帮助类 holder
    abstract class ViewHolder{
        ViewHolder(View view){
            ButterKnife.bind(this,view);
        }
    }

}
