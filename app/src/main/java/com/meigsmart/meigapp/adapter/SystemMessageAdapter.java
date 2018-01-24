package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.SystemMsgBean;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * Created by chenMeng on 2017/12/13.
 */

public class SystemMessageAdapter extends ExpandRecyclerViewAdapter<SystemMessageAdapter.GroupItemViewHolder,SystemMessageAdapter.SubItemViewHolder> {
    private Context mContext;

    private List<DataBean<SystemMsgBean, SystemMsgBean.MessageModel>> mList = new ArrayList<>();

    public SystemMessageAdapter(Context context) {
        this.mContext = context;
    }

    public void setData(List datas) {
        mList = datas;
        notifyNewData(mList);
    }

    public void setEmptyView(){
        mList = new ArrayList<>();
        notifyNewData(mList);
    }

    @Override
    public RecyclerView.ViewHolder groupItemViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.system_message_first_layout_item, parent, false);
        return new GroupItemViewHolder(v);
    }

    @Override
    public RecyclerView.ViewHolder subItemViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.system_message_second_layout_item, parent, false);
        return new SubItemViewHolder(v);
    }

    @Override
    public RecyclerView.ViewHolder emptyViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.include_blue_empty_layout, parent, false);
        return new EmptyItemViewHolder(v);
    }

    @Override
    public void onGroupItemBindViewHolder(RecyclerView.ViewHolder holder, int groupItemIndex) {
        ((GroupItemViewHolder) holder).time.setText(mList.get(groupItemIndex).getGroupItem().getTime());
    }

    @Override
    public void onSubItemBindViewHolder(RecyclerView.ViewHolder holder, int groupItemIndex, int subItemIndex) {
        ((SubItemViewHolder) holder).time.setText(mList.get(groupItemIndex).getSubItems().get(subItemIndex).getTime());
        ((SubItemViewHolder) holder).message.setText(mList.get(groupItemIndex).getSubItems().get(subItemIndex).getMessage());
    }

    @Override
    public void onEmptyBindViewHolder(RecyclerView.ViewHolder holder) {

    }

    @Override
    public void onGroupItemClick(Boolean isExpand, GroupItemViewHolder holder, int groupItemIndex) {

    }

    @Override
    public void onSubItemClick(SubItemViewHolder holder, int groupItemIndex, int subItemIndex) {

    }

    public static class GroupItemViewHolder extends RecyclerView.ViewHolder {
        @BindView(R.id.time)
        TextView time;

        public GroupItemViewHolder(View itemView) {
            super(itemView);
            ButterKnife.bind(this, itemView);
        }
    }

    public static class SubItemViewHolder extends RecyclerView.ViewHolder {
        @BindView(R.id.time)
        TextView time;
        @BindView(R.id.message)
        TextView message;

        public SubItemViewHolder(View itemView) {
            super(itemView);
            ButterKnife.bind(this, itemView);
        }
    }

    public static class EmptyItemViewHolder extends RecyclerView.ViewHolder {

        public EmptyItemViewHolder(View itemView) {
            super(itemView);
            ButterKnife.bind(this, itemView);
        }
    }
}
