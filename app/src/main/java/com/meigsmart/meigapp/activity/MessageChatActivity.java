package com.meigsmart.meigapp.activity;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.NotificationManager;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Message;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.text.TextUtils;
import android.view.Gravity;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.alibaba.fastjson.JSON;
import com.meigsmart.charlibs.adapter.ChatRecyclerAdapter;
import com.meigsmart.charlibs.bean.ChatConst;
import com.meigsmart.charlibs.bean.ChatMessageBean;
import com.meigsmart.charlibs.bean.ChatMessageType;
import com.meigsmart.charlibs.db.LastIdBean;
import com.meigsmart.charlibs.pop.SimpleTooltip;
import com.meigsmart.charlibs.util.KeyBoardUtils;
import com.meigsmart.charlibs.view.AudioRecordButton;
import com.meigsmart.charlibs.view.StateButton;
import com.meigsmart.meigapp.R;
import com.meigsmart.meigapp.application.MyApplication;
import com.meigsmart.meigapp.config.RequestCode;
import com.meigsmart.meigapp.http.download.DownloadHelper;
import com.meigsmart.meigapp.http.download.listener.DownloadListener;
import com.meigsmart.meigapp.http.rxjava.observer.BaseObserver;
import com.meigsmart.meigapp.http.service.HttpManager;
import com.meigsmart.meigapp.http.upload.UpLoadHelper;
import com.meigsmart.meigapp.http.upload.UpLoadListener;
import com.meigsmart.meigapp.log.LogUtil;
import com.meigsmart.meigapp.model.GroupListModel;
import com.meigsmart.meigapp.model.SendComplete;
import com.meigsmart.meigapp.model.SendCompletedModel;
import com.meigsmart.meigapp.model.UploadMsgModel;
import com.meigsmart.meigapp.model.VoiceMessageListModel;
import com.meigsmart.meigapp.model.VoiceMessageModel;
import com.meigsmart.meigapp.model.VoiceMsgDataModel;
import com.meigsmart.meigapp.sip.ObserverSip;
import com.meigsmart.meigapp.util.DateUtil;
import com.meigsmart.meigapp.util.FileUtil;
import com.meigsmart.meigapp.util.ToastUtil;
import com.tbruyelle.rxpermissions2.RxPermissions;

import org.pjsip.pjsua2.pjsip_inv_state;
import org.pjsip.pjsua2.pjsip_status_code;

import java.io.File;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TimerTask;

import butterknife.BindView;
import io.reactivex.android.schedulers.AndroidSchedulers;
import io.reactivex.functions.Consumer;
import io.reactivex.schedulers.Schedulers;

/**
 * 消息聊天 页面
 * @author chenmeng created by 2017/9/11
 */
public class MessageChatActivity extends BaseActivity implements View.OnClickListener,ChatRecyclerAdapter.OnCallBackClickListener,ObserverSip{
    private MessageChatActivity mContext;// 本类
    @BindView(R.id.back)
    public LinearLayout mBack;//返回上一层
    @BindView(R.id.title)
    public TextView mTitle;//标题
    @BindView(R.id.recyclerView)
    public RecyclerView mRecyclerView;//recyclerView
    @BindView(R.id.voiceImg)
    public ImageView mVoiceImg;//语音切换按钮
    @BindView(R.id.txt)
    public EditText mTxt;//文本
    @BindView(R.id.voice)
    public AudioRecordButton mVoice;//语音按钮
    @BindView(R.id.send)
    public StateButton mSend;//发送
    @BindView(R.id.more)
    public LinearLayout mMore;//右侧按钮
    @BindView(R.id.rightName)
    public TextView rightName;//内容 右侧
    private List<ChatMessageBean> messageList = new ArrayList<>();//数据
    private ChatRecyclerAdapter chatRecyclerAdapter;//适配器
    private SendMessageHandler sendMessageHandler;
    private static final int SEND_OK = 0x1110;
    private static final int RECEIVE_OK = 0x1111;
    private String userName = "";//人
    private GroupListModel mGroupModel;//f群聊数据
    private String voiceUrl = "";//发送语音消息 url
    private Map<String,Object> mVoiceMap = new HashMap<>();//发送语音消息数据
    private TimerTask timerTask;//定时任务
    private Timer timer;//定时器
    private List<ChatMessageBean> charHistoryList = new ArrayList<>();//历史数据
    private boolean isExitVoice = false;//是否有语音消息
    private int sumVoiceNum = 0;//语音消息的数量
    private int currNum = 0;//当前下载的数量
    private File downFile;//下载保存路径
    private String lastId = "0";//最后一条消息的id 默认0
    private int sumMsg = 0;//当前保存消息的总数量
    private boolean isDownFinish = false;//是否全部下载完成
    private boolean isFirstCome = true;//是否第一次进入这个页面
    private boolean isFilterMyMsg = false;//是否过滤自己的消息 true过滤  false不过滤(第一次进入不过滤)
    private String mGroupName = "";//群组名
    private String mGroupUuid = "";//uuid
    private int type = 0;//进入的类别
    private boolean isStartTimer = false;//定时器是否开始获取语音列表
    private int redoPos = -1;//点击 重新发送的信息 位置
    private NotificationManager mNotify;//通知

    @Override
    protected int getLayoutId() {
        return R.layout.activity_message_chat;
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        timerTask.cancel();
        timer.cancel();
        if (chatRecyclerAdapter!=null)chatRecyclerAdapter.pausePlayer();
        MyApplication.isPush = true;
        isStartTimer = true;
    }

    @Override
    protected void onResume() {
        super.onResume();
        isDownFinish = true;
        mTitle.setText(mGroupName);
        getInfoGroup(mGroupUuid);
        MyApplication.isPush = false;
        mNotify = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        mNotify.cancelAll();
    }

    @Override
    protected void onPause() {
        super.onPause();
        isDownFinish = false;
        if (chatRecyclerAdapter!=null)chatRecyclerAdapter.pausePlayer();
        MyApplication.isPush = true;
        isStartTimer = true;
    }

    @Override
    protected void initData() {
        mContext = this;
        mBack.setOnClickListener(this);
        mVoiceImg.setOnClickListener(this);
        mSend.setOnClickListener(this);
        mMore.setOnClickListener(this);
        rightName.setText(getResources().getString(R.string.message_chart_set));

        mMore.setVisibility(View.VISIBLE);
        type = getIntent().getIntExtra("type",0);
        MyApplication.isPush = false;
        if (type==1){
            mGroupModel = (GroupListModel) getIntent().getSerializableExtra("GroupListModel");
            mGroupUuid = mGroupModel.getGroup_uuid();
            mGroupName = mGroupModel.getName();
        }else{
            mGroupUuid = getIntent().getStringExtra("groupId");
            mGroupName = getIntent().getStringExtra("groupName");
        }
        MyApplication.getInstance().setCallBackSip(this);
        userName = MyApplication.userName;
        getVoiceMsgService(mGroupUuid);

        mRecyclerView.setLayoutManager(new LinearLayoutManager(this));
        sendMessageHandler = new SendMessageHandler(this);
        charHistoryList.clear();
        downFile = FileUtil.createRootDirectory("MeiGsmartVoice");

        mVoice.setAudioFinishRecorderListener(new AudioRecordButton.AudioFinishRecorderListener() {
            @Override
            public void onStart() {
            }

            @Override
            public void onFinished(float seconds, String filePath) {
                sendAudio(seconds,filePath);
            }
        });
        initListData();

        /**
         * 设置定时器，每10秒钟去更新一次聊天记录
         */
        timerTask = new TimerTask() {
            @Override
            public void run() {
                LogUtil.w("jack","down:"+isDownFinish+"\nfirst:"+isFirstCome);
                if (isDownFinish || isFirstCome){
                    isStartTimer = true;
                    isFirstCome = false;
                    mHandler.sendEmptyMessage(RequestCode.UPDATE_MSG);
                }
            }
        };
        timer = new Timer();
        timer.schedule(timerTask, 0, 1000*60*5);
    }

    /**
     * 初始化数据
     */
    private void initListData(){
        loadLocalMessage();
        chatRecyclerAdapter = new ChatRecyclerAdapter(this, messageList,this);
        chatRecyclerAdapter.resetRecycledViewPoolSize(mRecyclerView);

        mRecyclerView.setAdapter(chatRecyclerAdapter);
        mRecyclerView.scrollToPosition(messageList.size() - 1);
    }

    /**
     * 查询数据
     */
    private void loadLocalMessage() {
        if (messageList != null) {
            messageList.clear();
        }
        List<ChatMessageBean> chatMessageBeen = MyApplication.getInstance().mDb.getAllData(mGroupUuid);
        LogUtil.v("result","char_size:"+chatMessageBeen.size());
        messageList.addAll(chatMessageBeen);
        sumMsg = chatMessageBeen.size();
        if (chatMessageBeen.size()>0){
            isFilterMyMsg = true;
            lastId = String.valueOf(chatMessageBeen.get(sumMsg-1).getId());
            LastIdBean bean = MyApplication.getInstance().mLastDb.getData(mGroupUuid);

            if (bean!=null && !bean.equals("{}") && bean.getGroupId()!=null){
                lastId = MyApplication.getInstance().mLastDb.getData(mGroupUuid).getLastId();
//                MyApplication.getInstance().mLastDb.updateLastId(bean.getId(),Long.parseLong(lastId));
            }else{
                bean = new LastIdBean();
                bean.setGroupId(mGroupUuid);
                bean.setLastId(lastId);
                MyApplication.getInstance().mLastDb.addData(bean);
            }
        }else{
            isFilterMyMsg = false;
        }
    }

    /**
     * 4.4.获取语音消息服务URL
     */
    private void getVoiceMsgService(String to_group_id){
        HttpManager.getApiService().getVoiceMsgServiceUrl(to_group_id)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<VoiceMsgDataModel>() {
                    @Override
                    protected void onSuccess(VoiceMsgDataModel model) {
                        if (model.getResult().equals("200")){
                            voiceUrl = model.getVoice_message_server().getUrl();
                        }else{
                        }
                    }
                    @Override
                    public void onError(Throwable e) {
                    }
                });
    }

    /**
     * 获取语音消息列表
     * @param group_id
     */
    private void getVoiceMsgData(String group_id,String id){
        HttpManager.getApiService().getVoiceMessageList(group_id,id)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<VoiceMessageListModel>() {

                    @Override
                    public void onError(Throwable e) {
                        isDownFinish = true;
                    }

                    @Override
                    protected void onSuccess(VoiceMessageListModel model) {
                        if (model.getResult().equals("200")){
                            charHistoryList.clear();
                            isExitVoice = false;
                            sumVoiceNum = 0;
                            if (model.getVoice_messages()!=null && model.getVoice_messages().size()>0){
                                for (VoiceMessageModel v : model.getVoice_messages()){
                                    if (v.getMessageType().equals("0")){
                                        if (!TextUtils.isEmpty(v.getVoice_message_url())){
                                            sumVoiceNum += 1;
                                            isExitVoice = true;
                                        }
                                    }
                                }
                                if (isExitVoice){
                                    for (VoiceMessageModel vm : model.getVoice_messages()){
                                        if (vm.getMessageType().equals("0")) {//语音
                                            String url = vm.getVoice_message_url();
                                            if (!TextUtils.isEmpty(url)){
                                                int i = url.lastIndexOf("/");
                                                String name = url.substring(i,url.length());
                                                downVoice(url, downFile.getPath(),name+".amr",vm);
                                            }
                                        }else{
                                            ChatMessageBean bean = new ChatMessageBean();
                                            bean.setUserId(vm.getClient_uuid());
                                            bean.setId(Long.parseLong(vm.getId()));
                                            bean.setUserContent(vm.getVoice_message_url());
                                            bean.setUserName(vm.getName()==null?"":vm.getName());
                                            bean.setTime(DateUtil.getISODateByStr(vm.getCreated()).getTime());
                                            bean.setMessagetype(ChatMessageType.TextMessageType.getType());
                                            if (vm.getClient_uuid().equals(MyApplication.clientsModel.getClient_uuid())){
                                                bean.setType(0);
                                            }else{
                                                bean.setType(1);
                                            }
                                            bean.setSendState(ChatConst.COMPLETED);
                                            bean.setGroupId(mGroupUuid);
                                            bean.setDeviceType(Integer.parseInt(vm.getType()));
                                            charHistoryList.add(bean);
                                        }
                                    }
                                }else{//都是文本
                                    for (VoiceMessageModel vm : model.getVoice_messages()){
                                        if (vm.getMessageType().equals("1")){//文本
                                            ChatMessageBean bean = new ChatMessageBean();
                                            bean.setUserId(vm.getClient_uuid());
                                            bean.setId(Long.parseLong(vm.getId()));
                                            bean.setUserContent(vm.getVoice_message_url());
                                            bean.setUserName(vm.getName()==null?"":vm.getName());
                                            bean.setTime(DateUtil.getISODateByStr(vm.getCreated()).getTime());
                                            bean.setMessagetype(ChatMessageType.TextMessageType.getType());
                                            if (vm.getClient_uuid().equals(MyApplication.clientsModel.getClient_uuid())){
                                                bean.setType(0);
                                            }else{
                                                bean.setType(1);
                                            }
                                            bean.setSendState(ChatConst.COMPLETED);
                                            bean.setGroupId(mGroupUuid);
                                            bean.setDeviceType(Integer.parseInt(vm.getType()));
                                            charHistoryList.add(bean);
                                        }
                                    }
                                    isDownFinish = true;
                                    mHandler.sendEmptyMessage(RequestCode.SHOW_HISTORY_TXT);
                                }
                            }else{//没有数据
                                isDownFinish = true;
                            }
                        }
                    }
                });
    }

    /**
     * 发送语音消息
     */
    private void sendVoiceData(String url, Map<String,Object> map,final ChatMessageBean chatModel){
        UpLoadHelper helper = new UpLoadHelper(url, map);
        helper.upLoadFile(new UpLoadListener() {
            @Override
            public void upLoad(long bytesRead, long contentLength) {
            }

            @Override
            public void onSuccess(String result) {
                UploadMsgModel model = JSON.parseObject(result,UploadMsgModel.class);

                Map<String ,Object> map = new HashMap<>();
                SendComplete m = new SendComplete();
                m.setClient_uuid(model.getSend_voice_message().getClient_uuid());
                m.setLatitude(model.getSend_voice_message().getLatitude());
                m.setLongitude(model.getSend_voice_message().getLongitude());
                m.setUuid(model.getSend_voice_message().getUuid()==null?"":model.getSend_voice_message().getUuid());
                m.setUrl(model.getSend_voice_message().getUrl());
                m.setUpload_key(model.getSend_voice_message().getUpload_key()==null?"":model.getSend_voice_message().getUpload_key());
                m.setMessageType("0");

                map.put("voice_message_compleated",m);

                LogUtil.v("result","map:"+JSON.toJSONString(map));

                sendCompleted(mGroupUuid,JSON.toJSONString(map),chatModel);
            }

            @Override
            public void onFailure(Throwable t) {
                LogUtil.v("result","upload......onFailure");
                ChatMessageBean bean = chatModel;
                bean.setSendState(ChatConst.SENDERROR);
                chatRecyclerAdapter.updateMessageBean(bean);
            }
        });
    }

    /**
     * 发送完成
     */
    private void sendCompleted(String group_id,String data,final ChatMessageBean chatModel){
        HttpManager.getApiService().sendCompleted(group_id,HttpManager.getParameter(data))
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<SendCompletedModel>() {
                    @Override
                    protected void onSuccess(SendCompletedModel model) {
                        ChatMessageBean bean = chatModel;
                        if (model.getResult().equals("200")){
                            bean.setSendState(ChatConst.COMPLETED);
                            bean.setId(Long.parseLong(model.getVoice_message_compleated().getId()));
                            lastId = String.valueOf(bean.getId());
                            MyApplication.getInstance().mDb.addNewData(bean);

                            LastIdBean b = MyApplication.getInstance().mLastDb.getData(mGroupUuid);
                            MyApplication.getInstance().mLastDb.updateLastId(b.getId(),Long.parseLong(lastId));
                        }else{
                            bean.setSendState(ChatConst.SENDERROR);
                        }
                        if (redoPos<0){
                            chatRecyclerAdapter.updateMessageBean(bean);
                        }else{
                            chatRecyclerAdapter.updatePosMessageBean(redoPos,bean);
                            redoPos = -1;
                        }
                    }

                    @Override
                    public void onError(Throwable e) {
                        ChatMessageBean bean = chatModel;
                        bean.setSendState(ChatConst.SENDERROR);
                        chatRecyclerAdapter.updateMessageBean(bean);
                        redoPos = -1;
                    }
                });
    }

    /**
     * 获取群组信息
     */
    private void getInfoGroup(String group_id){
        HttpManager.getApiService().getInfoGroup(group_id)
                .subscribeOn(Schedulers.io())
                .observeOn(AndroidSchedulers.mainThread())
                .subscribe(new BaseObserver<GroupListModel>() {
                    @Override
                    protected void onSuccess(GroupListModel model) {
                        mGroupModel = model;
                    }

                    @Override
                    public void onError(Throwable e) {
                    }
                });
    }

    /**
     * 下载语音
     * @param url
     */
    private void downVoice(String url, String dirPath, String fileName,final VoiceMessageModel vm){

        final DownloadHelper helper = new DownloadHelper(url,dirPath,fileName);
        helper.downloadFile(new DownloadListener() {
            @Override
            public void update(long bytesRead, long contentLength) {
            }

            @Override
            public void onSuccess(File file) {
                LogUtil.v("result","path:"+file.getPath());
                currNum+=1;
                ChatMessageBean bean = new ChatMessageBean();
                bean.setUserId(vm.getClient_uuid());
                bean.setId(Long.parseLong(vm.getId()));
                bean.setUserVoicePath(file.getPath());
                bean.setUserName(vm.getName()==null?"":vm.getName());
                bean.setTime(DateUtil.getISODateByStr(vm.getCreated()).getTime());
                bean.setMessagetype(ChatMessageType.AudioMessageType.getType());
                if (vm.getClient_uuid().equals(MyApplication.clientsModel.getClient_uuid())){
                    bean.setType(0);
                }else{
                    bean.setType(1);
                }
                bean.setUserVoiceTime(Float.parseFloat(vm.getVoiceTime())/1000);
                bean.setSendState(ChatConst.COMPLETED);
                bean.setGroupId(mGroupUuid);
                bean.setIsListened(false);
                bean.setDeviceType(Integer.parseInt(vm.getType()));
                charHistoryList.add(bean);
                LogUtil.v("jack","curN:"+currNum+"\nsumN:"+sumVoiceNum);
                if (currNum == sumVoiceNum){
                    isDownFinish = true;
                    currNum = 0;
                    mHandler.sendEmptyMessage(RequestCode.SHOW_HISTORY_TXT);
                }
            }

            @Override
            public void onFailure(Throwable t) {
                helper.cancelDownload();
                isDownFinish = true;
            }
        });
    }

    @Override
    public void onClick(View view) {
        if (view == mBack) {
            mContext.finish();
        }
        if (view == mVoiceImg){
            getPermission(mContext, Manifest.permission.RECORD_AUDIO);
            if (mVoice.getVisibility() == View.GONE){//发送语音
                mSend.setClickable(false);
                mTxt.setVisibility(View.GONE);
                mVoice.setVisibility(View.VISIBLE);
                mVoiceImg.setImageResource(R.mipmap.ic_btn_keybroad);
                KeyBoardUtils.hideKeyBoard(mContext, mTxt);
            }else{//发送文本
                mSend.setClickable(true);
                mVoiceImg.setImageResource(R.mipmap.ic_btn_voice);
                mVoice.setVisibility(View.GONE);
                mTxt.setVisibility(View.VISIBLE);
                KeyBoardUtils.showKeyBoard(mContext, mTxt);
            }
        }
        if (view == mSend){
            if (TextUtils.isEmpty(mTxt.getText().toString().trim())){
                ToastUtil.showCenterShort(getResources().getString(R.string.message_chat_msg_no));
                return;
            }
            isFilterMyMsg = true;
            new Thread(new Runnable() {
                @Override
                public void run() {
                    ChatMessageBean bean = getTbub(userName, 0, ChatMessageType.TextMessageType.getType(),
                                mTxt.getText().toString(), null, null, 0f, ChatConst.SENDING);
                    chatRecyclerAdapter.addMessage(bean);
                    sendMessageHandler.sendEmptyMessage(SEND_OK);

                    Message msg = mHandler.obtainMessage();
                    msg.obj = bean;
                    msg.what = RequestCode.SEND_TXT;
                    mHandler.sendMessage(msg);
                }
            }).start();
        }
        if (view == mMore){
            if (mGroupModel!=null){
                Intent intent = new Intent(mContext,GroupSetActivity.class);
                intent.putExtra("GroupListModel",mGroupModel);
                startActivityForResult(intent,101);
            }
        }
    }

    /**
     * 发送语音
     */
    protected void sendAudio(final float seconds, final String filePath) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                ChatMessageBean bean = getTbub(userName, 0, ChatMessageType.AudioMessageType.getType(), null, filePath,
                        null, seconds, ChatConst.SENDING);
                chatRecyclerAdapter.addMessage(bean);
                sendMessageHandler.sendEmptyMessage(SEND_OK);

                mVoiceMap.clear();
                mVoiceMap.put("time",bean.getTime());
                mVoiceMap.put("name",bean.getUserName());
                mVoiceMap.put("file",new File(bean.getUserVoicePath()));
                mVoiceMap.put("voiceTime",bean.getUserVoiceTime()*1000);

                Message msg = mHandler.obtainMessage();
                msg.obj = bean;
                msg.what = RequestCode.SEND_VOICE;
                mHandler.sendMessage(msg);

            }
        }).start();
    }


    @SuppressLint("HandlerLeak")
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            switch (msg.what) {
                case RequestCode.SEND_TXT:

                    ChatMessageBean bean = (ChatMessageBean) msg.obj;

                    Map<String ,Object> map = new HashMap<>();
                    SendComplete m = new SendComplete();
                    m.setClient_uuid(MyApplication.clientsModel.getClient_uuid());
                    m.setLatitude(String.valueOf(MyApplication.lat));
                    m.setLongitude(String.valueOf(MyApplication.lng));
                    m.setUrl(bean.getUserContent());
                    m.setMessageType("1");

                    map.put("voice_message_compleated",m);

                    LogUtil.v("result","map:"+JSON.toJSONString(map));

                    sendCompleted(mGroupUuid,JSON.toJSONString(map),bean);

                    break;
                case RequestCode.SEND_VOICE:
                    ChatMessageBean chatModel = (ChatMessageBean) msg.obj;
                    if (!TextUtils.isEmpty(voiceUrl)){
                        sendVoiceData(voiceUrl,mVoiceMap,chatModel);
                    }
                    break;
                case RequestCode.UPDATE_MSG://获取消息列表
                    LastIdBean lb = MyApplication.getInstance().mLastDb.getData(mGroupUuid);

                    getVoiceMsgData(mGroupUuid, lb.getLastId()==null?"0":lb.getLastId());
                    break;
                case RequestCode.SHOW_HISTORY_TXT:
                    //按时间排序
                    Collections.sort(charHistoryList, new Comparator<ChatMessageBean>() {

                        public int compare(ChatMessageBean user1, ChatMessageBean user2) {
                            return (user1.getTime() == user2.getTime() ? 0 : (user1.getTime() > user2.getTime() ? 1 : -1));
                        }

                    });

                    if (charHistoryList.size()>0){
                        lastId = String.valueOf(charHistoryList.get(charHistoryList.size()-1).getId());

                        LastIdBean b = MyApplication.getInstance().mLastDb.getData(mGroupUuid);
                        MyApplication.getInstance().mLastDb.updateLastId(b.getId(),Long.parseLong(lastId));
                        LogUtil.v("result","bean:"+JSON.toJSONString(b));
                    }

                    for (ChatMessageBean model : charHistoryList){
                        if (isFilterMyMsg){
                            if (!model.getUserId().equals(MyApplication.clientsModel.getClient_uuid())){//自己发送的消息 不拉取显示
                                MyApplication.getInstance().mDb.addNewData(model);
                                chatRecyclerAdapter.addMessageBean(model);
                                chatRecyclerAdapter.notifyItemInserted(charHistoryList.size() - 1);
                                mRecyclerView.smoothScrollToPosition(chatRecyclerAdapter.getItemCount() - 1);
                            }
                        }else{
                            MyApplication.getInstance().mDb.addNewData(model);
                            chatRecyclerAdapter.addMessageBean(model);
                            chatRecyclerAdapter.notifyItemInserted(charHistoryList.size() - 1);
                            mRecyclerView.smoothScrollToPosition(chatRecyclerAdapter.getItemCount() - 1);
                        }

                    }

                    break;
                default:
                    break;
            }
        }
    };


    /**
     * 数据
     */
    public ChatMessageBean getTbub(String username, int type, int messageType,
                                   String Content, String userVoicePath, String userVoiceUrl,
                                   Float userVoiceTime, int sendState) {
        ChatMessageBean tbub = new ChatMessageBean();
        tbub.setUserId(MyApplication.clientsModel.getClient_uuid());
        tbub.setUserName(username);
        tbub.setTime(System.currentTimeMillis());
        tbub.setType(type);
        tbub.setMessagetype(messageType);
        tbub.setUserContent(Content);
        tbub.setUserVoicePath(userVoicePath);
        tbub.setUserVoiceUrl(userVoiceUrl);
        tbub.setUserVoiceTime(userVoiceTime);
        tbub.setSendState(sendState);
        tbub.setGroupId(mGroupUuid);
        tbub.setIsListened(true);
        tbub.setDeviceType(1);
        return tbub;
    }

    private static class SendMessageHandler extends Handler {
        WeakReference<MessageChatActivity> mActivity;

        SendMessageHandler(MessageChatActivity activity) {
            mActivity = new WeakReference<>(activity);
        }

        @Override
        public void handleMessage(Message msg) {
            MessageChatActivity theActivity = mActivity.get();
            if (theActivity != null) {
                switch (msg.what) {
                    case SEND_OK:
                        theActivity.mTxt.setText("");
                        theActivity.chatRecyclerAdapter.notifyItemInserted(theActivity.messageList.size() - 1);
                        theActivity.mRecyclerView.smoothScrollToPosition(theActivity.chatRecyclerAdapter.getItemCount() - 1);
                        break;
                    case RECEIVE_OK:
                        theActivity.chatRecyclerAdapter.notifyItemInserted(theActivity.messageList.size() - 1);
                        theActivity.mRecyclerView.smoothScrollToPosition(theActivity.chatRecyclerAdapter.getItemCount() - 1);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    /**
     * 获取权限
     * @param context
     * @param str
     */
    public void getPermission(Activity context, String...str){
        RxPermissions permissions = new RxPermissions(context);
        permissions.request(str)
                .subscribe(new Consumer<Boolean>() {
                    @Override
                    public void accept(Boolean aBoolean) throws Exception {
                    }
                });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        LogUtil.v("jack","requestCode:"+requestCode+"\nresultCode:"+resultCode+"\ndata:"+data);
        if (requestCode == 101){
            if (data!= null){
                if (data.getBooleanExtra("isExit",false)){
                    setResult(111);
                    mContext.finish();
                }else{
                    mTitle.setText(data.getStringExtra("name"));
                    if (data.getBooleanExtra("clear",false)){
                        chatRecyclerAdapter.clear();
                    }
                    mGroupModel.setName(data.getStringExtra("name"));
                }
            }
            mHandler.sendEmptyMessage(RequestCode.UPDATE_MSG);
        }
    }

    @Override
    public void onSendErrorListener(int position, ChatMessageBean bean) {
        redoPos = position;
        LogUtil.w("tag","errorBean:"+JSON.toJSONString(bean));
        if (bean.getMessagetype() == ChatMessageType.TextMessageType.getType()){//文本
            bean.setSendState(ChatConst.SENDING);
            bean.setPosition(position);
            chatRecyclerAdapter.notifyDataSetChanged();

            Message msg = mHandler.obtainMessage();
            msg.obj = bean;
            msg.what = RequestCode.SEND_TXT;
            mHandler.sendMessage(msg);

        }else if (bean.getMessagetype() == ChatMessageType.AudioMessageType.getType()){//语音
            bean.setSendState(ChatConst.SENDING);
            bean.setPosition(position);
            chatRecyclerAdapter.notifyDataSetChanged();

            Message msg = mHandler.obtainMessage();
            msg.obj = bean;
            msg.what = RequestCode.SEND_VOICE;
            mHandler.sendMessage(msg);
        }
    }

    @Override
    public void onLongItemClickListener(final View v, final int position, final ChatMessageBean bean, int type) {
        if (type == 0){//文本
            final SimpleTooltip tooltip = new SimpleTooltip.Builder(this)
                    .anchorView(v)
                    .text(getString(R.string.copy))
                    .gravity(Gravity.TOP)
                    .dismissOnInsideTouch(false)
                    .showArrow(false)
                    .modal(true)
                    .animated(true)
                    .contentView(R.layout.pop_layout, R.id.name)
                    .build();

            //复制
            tooltip.findViewById(R.id.name).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v2) {
                    if (tooltip.isShowing()) tooltip.dismiss();
                    ClipboardManager clipboardManager = (ClipboardManager)getSystemService(CLIPBOARD_SERVICE);
                    //创建ClipData对象
                    ClipData clipData = ClipData.newPlainText(bean.getUserContent(),bean.getUserContent());
                    //添加ClipData对象到剪切板中
                    clipboardManager.setPrimaryClip(clipData);
                }
            });
            //删除
            TextView del = tooltip.findViewById(R.id.delete);
            del.setVisibility(View.VISIBLE);
            del.setText(getString(R.string.delete));
            del.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v2) {
                    if (tooltip.isShowing()) tooltip.dismiss();
                    MyApplication.getInstance().mDb.deleteCurData(String.valueOf(bean.getId()));
                    chatRecyclerAdapter.updateMovePos(position);
                }
            });
            tooltip.show();
        }else if (type == 1){//语音
            final SimpleTooltip tooltip = new SimpleTooltip.Builder(this)
                    .anchorView(v)
                    .text(getString(R.string.delete))
                    .gravity(Gravity.TOP)
                    .dismissOnInsideTouch(false)
                    .showArrow(false)
                    .modal(true)
                    .animated(true)
                    .contentView(R.layout.pop_layout, R.id.name)
                    .build();

            //删除
            tooltip.findViewById(R.id.name).setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v2) {
                    if (tooltip.isShowing()) tooltip.dismiss();
                    MyApplication.getInstance().mDb.deleteCurData(String.valueOf(bean.getId()));
                    chatRecyclerAdapter.updateMovePos(position);
                }
            });
            tooltip.show();
        }
    }

    @Override
    public void onClickHeaderListener(ChatMessageBean bean, boolean isLeft) {
        if (bean.getDeviceType()==0){
            Intent intent = new Intent(mContext,UserDeviceInfoActivity.class);
            intent.putExtra("type",0);
            intent.putExtra("deviceSerial",bean.getUserName());
            startActivity(intent);
        }else if (bean.getDeviceType()==1){//app
            Intent intent = new Intent(mContext,UserAppInfoActivity.class);
            if (isLeft){
                intent.putExtra("type",1);
                intent.putExtra("client_uuid",bean.getUserId());
            }else {
                intent.putExtra("type",0);
            }
            startActivity(intent);
        }
    }


    @Override
    public void onRegistration(String accountID, pjsip_status_code registrationStateCode) {
    }

    @Override
    public void onCallState(String accountID, int callID, pjsip_inv_state callStateCode, long connectTimestamp, boolean isLocalHold, boolean isLocalMute) {
    }

    @Override
    public void onMessageNotify(String accountID, String fromUri, String msg) {
        LogUtil.v("result","MessageChatActivity:  notifyMessageIncoming");
        if (isDownFinish){
            isStartTimer = false;
        }
        isDownFinish = false;
        if (!isStartTimer){
            mHandler.sendEmptyMessage(RequestCode.UPDATE_MSG);
        }
    }

    @Override
    public void onOutgoingCall(String accountID, int callID, String number) {
    }

}