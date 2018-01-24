package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.fragment.BaseFragment;
import com.meigsmart.meigapp.model.TypeModel;

import java.util.ArrayList;
import java.util.List;

/**
 * 群组管理适配器
 * Created by chenMeng on 2017/9/7.
 */

public class MenuFragmentAdapter extends FragmentPagerAdapter {
    private List<TypeModel> mList;
    private List<BaseFragment> mFragmentList;
    private Context mContext;
    private int[] mRes = {R.drawable.fragment_message_bg,R.drawable.fragment_linkman_bg,R.drawable.fragment_setting_bg};

    public MenuFragmentAdapter(FragmentManager fm, String[] data, List<BaseFragment> fragments, Context context) {
        super(fm);
        this.mList = getData(data);
        this.mFragmentList = fragments;
        this.mContext = context;
    }

    private List<TypeModel> getData(String[] data){
        List<TypeModel> list = new ArrayList<>();
        for (int i=0;i<data.length;i++){
            TypeModel model = new TypeModel();
            model.setRes(mRes[i]);
            model.setName(data[i]);
            list.add(model);
        }
        return list;
    }

    public View getView(int pos){
        View view = LayoutInflater.from(mContext).inflate(R.layout.tablayout_title_item_layout,null);
        TextView name = (TextView) view.findViewById(R.id.name);
        ImageView img = (ImageView) view.findViewById(R.id.img);
        name.setText(mList.get(pos).getName());
        img.setImageResource(mList.get(pos).getRes());
        return view;
    }

    @Override
    public Fragment getItem(int position) {
        return mFragmentList.get(position);
    }

    @Override
    public int getCount() {
        return mList.size();
    }

    //配置标题的方法
    @Override
    public CharSequence getPageTitle(int position) {
        return mList.get(position).getName();
    }

    @Override
    public int getItemPosition(Object object) {
        return POSITION_NONE;
    }
}
