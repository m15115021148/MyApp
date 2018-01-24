package com.meigsmart.meigapp.adapter;

import android.support.v7.widget.RecyclerView;
import android.view.View;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.List;

/**
 * Created by chenMeng on 2017/12/13.
 */

public abstract class ExpandRecyclerViewAdapter<GVH, SVH extends RecyclerView.ViewHolder> extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

    private List<Boolean> groupItemStatus = new ArrayList<>();

    private List<DataBean> dataBeans = new ArrayList<>();

    private boolean isExpand = true;

    private boolean isShowEmpty = true;

    public boolean isShowEmpty() {
        return isShowEmpty;
    }

    public void setShowEmpty(boolean showEmpty) {
        isShowEmpty = showEmpty;
    }

    public boolean isExpand() {
        return isExpand;
    }

    public void setExpand(boolean expand) {
        isExpand = expand;
    }

    /**
     * Set new data for adapter to show. It must be called when set new data.
     *
     * @param data New data
     */
    protected void notifyNewData(List data) {
        setDataBeans(data,isExpand);
    }


    /**
     * Set new data for adapter and notify changing.
     *
     * @param dt New data
     */
    private final void setDataBeans(List dt,boolean isExpand) {
        this.dataBeans = dt;
        initGroupItemStatus(groupItemStatus,isExpand);
        notifyDataSetChanged();
    }

    /**
     * Initialize the list to false.
     *
     * @param l The list need to initialize
     */
    private void initGroupItemStatus(List l,boolean isExpand) {
        for (int i = 0; i < dataBeans.size(); i++) {
            l.add(isExpand);
        }
    }


    /**
     * Create group item view holder for onCreateViewHolder.
     *
     * @param parent Provided by onCreateViewHolder.
     */
    public abstract RecyclerView.ViewHolder groupItemViewHolder(ViewGroup parent);

    /**
     * Create subitem view holder for onCreateViewHolder.
     *
     * @param parent Provided by onCreateViewHolder.
     */
    public abstract RecyclerView.ViewHolder subItemViewHolder(ViewGroup parent);

    /**
     * Empty view
     *
     * @param parent Provided by onCreateViewHolder.
     * @return
     */
    public abstract RecyclerView.ViewHolder emptyViewHolder(ViewGroup parent);


    @Override
    public final RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {

        RecyclerView.ViewHolder viewHolder = null;

        if (viewType == ItemStatus.VIEW_TYPE_GROUPITEM) {
            viewHolder = groupItemViewHolder(parent);
        } else if (viewType == ItemStatus.VIEW_TYPE_SUBITEM) {
            viewHolder = subItemViewHolder(parent);
        } else if (viewType == ItemStatus.VIEW_TYPE_EMPTY) {
            viewHolder = emptyViewHolder(parent);
        }
        return viewHolder;
    }

    /**
     * Update the content of specified group item. The method will called by onBindViewHolder.
     *
     * @param holder         The ViewHolder which should be updated to represent the contents of the
     *                       item at the given position in the data set.
     * @param groupItemIndex The index of the group item.
     */
    public abstract void onGroupItemBindViewHolder(RecyclerView.ViewHolder holder, int groupItemIndex);

    /**
     * Update the content of specified subitem. The method will called by onBindViewHolder.
     *
     * @param holder       The ViewHolder which should be updated to represent the contents of the
     *                     item at the given position in the data set.
     * @param subItemIndex The index of the subitem.
     */
    public abstract void onSubItemBindViewHolder(RecyclerView.ViewHolder holder, int groupItemIndex, int subItemIndex);

    public abstract void onEmptyBindViewHolder(RecyclerView.ViewHolder holder);

    /**
     * The method will be called when the group item clicked.
     *
     * @param isExpand       whether is expanded or no the group item clicked.
     * @param holder         The holder' s item view clicked.
     * @param groupItemIndex The index of the group item clicked.
     */
    public abstract void onGroupItemClick(Boolean isExpand, GVH holder, int groupItemIndex);

    /**
     * The method will be called when the subitem clicked.
     *
     * @param holder       The holder' s item view clicked.
     * @param subItemIndex The index of the subitem clicked.
     */
    public abstract void onSubItemClick(SVH holder, int groupItemIndex, int subItemIndex);

    @Override
    public final void onBindViewHolder(final RecyclerView.ViewHolder holder, int position) {

        final ItemStatus itemStatus = getItemStatusByPosition(position);

        if (itemStatus.getViewType() == ItemStatus.VIEW_TYPE_GROUPITEM) {
            final DataBean dt  = dataBeans.get(itemStatus.getGroupItemIndex());

            onGroupItemBindViewHolder(holder, itemStatus.getGroupItemIndex());

            holder.itemView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {

                    int groupItemIndex = itemStatus.getGroupItemIndex();

                    if (!groupItemStatus.get(groupItemIndex)) {

                        onGroupItemClick(false, (GVH) holder, groupItemIndex);

                        groupItemStatus.set(groupItemIndex, true);
                        notifyItemRangeInserted(holder.getAdapterPosition() + 1, dt.getSubItems
                                ().size());


                    } else {

                        onGroupItemClick(true, (GVH) holder, groupItemIndex);

                        groupItemStatus.set(groupItemIndex, false);
                        notifyItemRangeRemoved(holder.getAdapterPosition() + 1, dt.getSubItems
                                ().size());

                    }

                }
            });

        } else if (itemStatus.getViewType() == ItemStatus.VIEW_TYPE_SUBITEM) {

            onSubItemBindViewHolder(holder, itemStatus.getGroupItemIndex(), itemStatus
                    .getSubItemIndex());

            holder.itemView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {

                    onSubItemClick((SVH) holder, itemStatus.getGroupItemIndex(), itemStatus.getSubItemIndex());

                }
            });

        } else if (itemStatus.getViewType() == ItemStatus.VIEW_TYPE_EMPTY) {
            onEmptyBindViewHolder(holder);
        }


    }

    @Override
    public final int getItemCount() {

        int itemCount = 0;

        if (groupItemStatus.size() == 0) {
            return isShowEmpty?1:0;//1 show empty
        }

        for (int i = 0; i < dataBeans.size(); i++) {

            if (groupItemStatus.get(i)) {
                itemCount += dataBeans.get(i).getSubItems().size() + 1;
            } else {
                itemCount++;
            }

        }

        return itemCount;
    }


    @Override
    public final int getItemViewType(int position) {
        return getItemStatusByPosition(position).getViewType();
    }


    /**
     * Get item' s status include view type, group item index and subitem index.
     *
     * @param position Position
     */
    private ItemStatus getItemStatusByPosition(int position) {

        ItemStatus itemStatus = new ItemStatus();

        int count = 0;
        int i = 0;

        if (dataBeans.size() > 0) {

            for (i = 0; i < groupItemStatus.size(); i++) {

                if (count == position) {

                    itemStatus.setViewType(ItemStatus.VIEW_TYPE_GROUPITEM);
                    itemStatus.setGroupItemIndex(i);
                    break;

                } else if (count > position) {

                    itemStatus.setViewType(ItemStatus.VIEW_TYPE_SUBITEM);
                    itemStatus.setGroupItemIndex(i - 1);
                    itemStatus.setSubItemIndex(position - (count - dataBeans.get(i - 1).getSubItems
                            ().size()));
                    break;

                }

                count++;

                if (groupItemStatus.get(i)) {

                    count += dataBeans.get(i).getSubItems().size();

                }


            }

            if (i >= groupItemStatus.size()) {
                itemStatus.setGroupItemIndex(i - 1);
                itemStatus.setViewType(ItemStatus.VIEW_TYPE_SUBITEM);
                itemStatus.setSubItemIndex(position - (count - dataBeans.get(i - 1).getSubItems().size
                        ()));
            }
        } else {
            itemStatus.setViewType(ItemStatus.VIEW_TYPE_EMPTY);
        }

        return itemStatus;
    }


    private static class ItemStatus {

        public static final int VIEW_TYPE_GROUPITEM = 0;
        public static final int VIEW_TYPE_SUBITEM = 1;
        public static final int VIEW_TYPE_EMPTY = 3;

        private int viewType;
        private int groupItemIndex = 0;
        private int subItemIndex = -1;

        public ItemStatus() {
        }

        public int getViewType() {
            return viewType;
        }

        public void setViewType(int viewType) {
            this.viewType = viewType;
        }

        public int getGroupItemIndex() {
            return groupItemIndex;
        }

        public void setGroupItemIndex(int groupItemIndex) {
            this.groupItemIndex = groupItemIndex;
        }

        public int getSubItemIndex() {
            return subItemIndex;
        }

        public void setSubItemIndex(int subItemIndex) {
            this.subItemIndex = subItemIndex;
        }
    }


    public static class DataBean<K, V> {

        private K groupItem;
        private List<V> subItems;

        public DataBean(K groupItem, List<V> subItems) {
            this.groupItem = groupItem;
            this.subItems = subItems;
        }

        public K getGroupItem() {
            return groupItem;
        }

        public List<V> getSubItems() {
            return subItems;
        }
    }
}
