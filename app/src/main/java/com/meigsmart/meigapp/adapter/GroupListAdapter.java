package com.meigsmart.meigapp.adapter;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.view.SwipeMenuView;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * 群组列表适配器
 * Created by chenMeng on 2017/9/15.
 */
public class GroupListAdapter extends RecyclerView.Adapter<GroupListAdapter.Holder>{
    private List<GroupListModel> mList;
    private OnCallBackDel mCallBack;

    public interface OnCallBackDel{
        void onDel(SwipeMenuView view,int position);
        void onItemView(int position);
    }

    public GroupListAdapter(List<GroupListModel> list,OnCallBackDel callBack){
        this.mList = list;
        this.mCallBack = callBack;
    }

    /**
     * 数据
     */
    public void setData(List<GroupListModel> list){
        this.mList = list;
        this.notifyDataSetChanged();
    }

    @Override
    public Holder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.group_list_listview_item,null);
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
        @BindView(R.id.img) ImageView img;
        @BindView(R.id.name) TextView name;
        @BindView(R.id.delete) LinearLayout del;
        @BindView(R.id.swipeView) SwipeMenuView mSwipe;
        @BindView(R.id.layout) RelativeLayout mItem;

        Holder(View view) {
            super(view);
            ButterKnife.bind(this,view);
        }

        public void initData(final int position){
            GroupListModel model = mList.get(position);
            name.setText(model.getName());
            mSwipe.setSwipeEnable(false);
            del.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onDel(mSwipe,position);
                    }
                }
            });
            mItem.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onItemView(position);
                    }
                }
            });
        }
    }
}
