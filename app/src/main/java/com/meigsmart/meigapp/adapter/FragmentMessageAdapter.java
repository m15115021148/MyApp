package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.model.MembersModel;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * Created by chenMeng on 2017/12/27.
 */

public class FragmentMessageAdapter extends ExpandRecyclerViewAdapter<FragmentMessageAdapter.GroupItemViewHolder,FragmentMessageAdapter.SubItemViewHolder> {
    private Context mContext;
    private List<DataBean<GroupListModel, MembersModel>> mList = new ArrayList<>();
    private onMessageCallBack callBack;

    public FragmentMessageAdapter(Context context,onMessageCallBack callBack) {
        this.mContext = context;
        this.callBack = callBack;
    }

    public interface onMessageCallBack{
        void onGroupClick(int groupItemIndex);
        void onSubItemClick(int groupItemIndex, int subItemIndex);
        void onSubItemLongClick(int groupItemIndex, int subItemIndex);
    }

    public void setData(List datas) {
        mList = datas;
        this.setExpand(false);
        notifyNewData(mList);
    }

    public void setEmptyView(){
        mList = new ArrayList<>();
        notifyNewData(mList);
    }

    @Override
    public RecyclerView.ViewHolder groupItemViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.fragment_message_group_layout_item, parent, false);
        return new GroupItemViewHolder(v);
    }

    @Override
    public RecyclerView.ViewHolder subItemViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.fragment_message_subitem_layout_item, parent, false);
        return new SubItemViewHolder(v);
    }

    @Override
    public RecyclerView.ViewHolder emptyViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.include_blue_empty_layout, parent, false);
        return new EmptyItemViewHolder(v);
    }

    @Override
    public void onGroupItemBindViewHolder(RecyclerView.ViewHolder holder, final int groupItemIndex) {
        ((GroupItemViewHolder) holder).name.setText(mList.get(groupItemIndex).getGroupItem().getName());
        ((GroupItemViewHolder) holder).layout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (callBack!=null)callBack.onGroupClick(groupItemIndex);
            }
        });
        if (mList.get(groupItemIndex).getGroupItem().getSelect() == 0){
            ((GroupItemViewHolder) holder).status.setSelected(false);
        }else{
            ((GroupItemViewHolder) holder).status.setSelected(true);
        }
    }

    @Override
    public void onSubItemBindViewHolder(RecyclerView.ViewHolder holder,final int groupItemIndex, final int subItemIndex) {
        ((SubItemViewHolder) holder).name.setText(mList.get(groupItemIndex).getSubItems().get(subItemIndex).getName());
        ((SubItemViewHolder) holder).layout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (callBack!=null)callBack.onSubItemClick(groupItemIndex,subItemIndex);
            }
        });
        ((SubItemViewHolder) holder).layout.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View v) {
                if (callBack!=null)callBack.onSubItemLongClick(groupItemIndex,subItemIndex);
                return true;
            }
        });
    }

    @Override
    public void onEmptyBindViewHolder(RecyclerView.ViewHolder holder) {

    }

    @Override
    public void onGroupItemClick(Boolean isExpand, GroupItemViewHolder holder, int groupItemIndex) {
        holder.status.setSelected(!isExpand);
        if (isExpand){
            mList.get(groupItemIndex).getGroupItem().setSelect(0);
        }else{
            mList.get(groupItemIndex).getGroupItem().setSelect(1);
        }
    }

    @Override
    public void onSubItemClick(SubItemViewHolder holder, int groupItemIndex, int subItemIndex) {

    }

    public static class GroupItemViewHolder extends RecyclerView.ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.layout)
        LinearLayout layout;
        @BindView(R.id.status)
        LinearLayout status;

        public GroupItemViewHolder(View itemView) {
            super(itemView);
            ButterKnife.bind(this, itemView);
        }
    }

    public static class SubItemViewHolder extends RecyclerView.ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.layout)
        RelativeLayout layout;

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
