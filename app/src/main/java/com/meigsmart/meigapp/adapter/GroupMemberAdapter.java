package com.meigsmart.meigapp.adapter;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.support.annotation.RequiresApi;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.model.MembersModel;
import com.meigsmart.meigapp.util.RegularUtil;

import java.util.List;

import butterknife.BindView;
import butterknife.ButterKnife;

/**
 * 成员列表 适配器
 * Created by chenMeng on 2017/9/16.
 */
public class GroupMemberAdapter extends RecyclerView.Adapter<GroupMemberAdapter.Holder> {
    private List<MembersModel> mList;
    private Context mContext;
    private OnCallBackClickListener mCallBack;
    private int type = 1;

    public GroupMemberAdapter(Context context,List<MembersModel> list,OnCallBackClickListener callBack){
        this.mContext = context;
        this.mList = list;
        this.mCallBack = callBack;
    }

    public interface OnCallBackClickListener{
        void onClickItemListener(int position);
        void onLongClickItemListener(int position);
    }


    @Override
    public Holder onCreateViewHolder(ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.group_member_layout_item,null);
        Holder holder = new Holder(view);
        return holder;
    }

    @Override
    public void onBindViewHolder(Holder holder, int position) {
        holder.setData(position);
    }

    @Override
    public int getItemCount() {
        return mList.size();
    }


    class Holder extends RecyclerView.ViewHolder{
        @BindView(R.id.layout)
        RelativeLayout mLayout;
        @BindView(R.id.name)
        TextView name;
        @BindView(R.id.img)
        ImageView img;

        public Holder(View itemView) {
            super(itemView);
            ButterKnife.bind(this,itemView);
        }

        @SuppressLint("NewApi")
        public void setData(final int position){
            MembersModel model = mList.get(position);
            if (position==mList.size()-1){
                mLayout.setBackgroundColor(mContext.getResources().getColor(R.color.white));
                img.setVisibility(View.VISIBLE);
            }else{
                img.setVisibility(View.GONE);
                name.setText(getName(model.getName()));
                if (model.getOnline().equals("0")){
                    mLayout.setBackground(mContext.getResources().getDrawable(R.drawable.person_offline));
                }else{
                    mLayout.setBackground(mContext.getResources().getDrawable(R.drawable.person_online));
                }


            }

            mLayout.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onClickItemListener(position);
                    }
                }
            });
            mLayout.setOnLongClickListener(new View.OnLongClickListener() {
                @Override
                public boolean onLongClick(View view) {
                    if (mCallBack!=null){
                        mCallBack.onLongClickItemListener(position);
                    }
                    return false;
                }
            });
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
            if (str.length()>6){
                if (RegularUtil.isInputCharOrNum(str)){
                    return str.substring(0,3) + "***" + str.substring(str.length()-3,str.length());
                }else{
                    return str.substring(0,2) + "**" + str.substring(str.length()-2,str.length());
                }
            }
            return str;
        }
    }
}
