package com.meigsmart.meigapp.adapter;

import android.content.Context;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.LinkManModel;
import com.meigsmart.meigapp.util.RegularUtil;

import java.util.ArrayList;
import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * Created by chenMeng on 2017/12/29.
 */

public class FragmentLinkManAdapter extends ExpandRecyclerViewAdapter<FragmentLinkManAdapter.GroupItemViewHolder,FragmentLinkManAdapter.SubItemViewHolder>{
    private Context mContext;
    private List<DataBean<LinkManModel, LinkManModel.Person>> mList = new ArrayList<>();
    private OnLinkManCallBack callBack;

    public FragmentLinkManAdapter(Context context,OnLinkManCallBack callBack) {
        this.mContext = context;
        this.callBack = callBack;
        this.setExpand(true);
        this.setShowEmpty(false);
    }

    public interface OnLinkManCallBack{
        void onGroupClick(int groupItemIndex);
        void onSubItemClick(int groupItemIndex, int subItemIndex);
        void onSubItemLongClick(int groupItemIndex, int subItemIndex);
    }

    public void setData(List data) {
        mList = data;
        notifyNewData(mList);
    }

    public void refreshItemData(int groupItemIndex, int subItemIndex){
        mList.get(groupItemIndex).getSubItems().remove(subItemIndex);
        if (mList.get(groupItemIndex).getSubItems().size()<=0){
            mList.remove(groupItemIndex);
        }
        this.notifyDataSetChanged();
    }

    @Override
    public RecyclerView.ViewHolder groupItemViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.fragment_link_man_group_layout_item, parent, false);
        return new GroupItemViewHolder(v);
    }

    @Override
    public RecyclerView.ViewHolder subItemViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.fragment_link_man_subitem_layout_item, parent, false);
        return new SubItemViewHolder(v);
    }

    @Override
    public RecyclerView.ViewHolder emptyViewHolder(ViewGroup parent) {
        View v = LayoutInflater.from(mContext).inflate(R.layout.include_blue_empty_layout, parent, false);
        return new EmptyItemViewHolder(v);
    }

    @Override
    public void onGroupItemBindViewHolder(RecyclerView.ViewHolder holder,final int groupItemIndex) {
        ((GroupItemViewHolder) holder).name.setText(mList.get(groupItemIndex).getGroupItem().getName());
        ((GroupItemViewHolder) holder).layout.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (callBack!=null)callBack.onGroupClick(groupItemIndex);
            }
        });
    }

    @Override
    public void onSubItemBindViewHolder(RecyclerView.ViewHolder holder,final int groupItemIndex,final int subItemIndex) {
        String name = mList.get(groupItemIndex).getSubItems().get(subItemIndex).getName();

        ((SubItemViewHolder) holder).head.setText(getName(name));
        ((SubItemViewHolder) holder).name.setText(name);

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

    }

    @Override
    public void onSubItemClick(SubItemViewHolder holder, int groupItemIndex, int subItemIndex) {

    }

    /**
     * 名称缩写显示
     * @param str
     * @return
     */
    private String getName(String str){
        if (TextUtils.isEmpty(str) || str.equals("null")){
            return "Unknown";
        }
        if (str.length()>2 && str.length() <15){

            if (RegularUtil.isInputCharOrNum(str)){
                return str.substring(str.length()-3,str.length());
            }else{
                return str.substring(0,2);
            }
        }else if (str.length()>=15){
            if (RegularUtil.isInputCharOrNum(str)){
                return str.substring(str.length()-3,str.length());
            }else{
                return str.substring(0,2);
            }
        }
        return str;
    }

    public static class GroupItemViewHolder extends RecyclerView.ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.layout)
        LinearLayout layout;

        public GroupItemViewHolder(View itemView) {
            super(itemView);
            ButterKnife.bind(this, itemView);
        }
    }

    public static class SubItemViewHolder extends RecyclerView.ViewHolder {
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.head)
        TextView head;
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
