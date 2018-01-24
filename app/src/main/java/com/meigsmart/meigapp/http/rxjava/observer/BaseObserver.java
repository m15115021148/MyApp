package com.meigsmart.meigapp.http.rxjava.observer;

import android.support.annotation.CallSuper;

import com.meigsmart.meigapp.http.result.HttpResponseException;
import com.meigsmart.meigapp.util.ToastUtil;

import java.net.ConnectException;

import io.reactivex.Observer;
import io.reactivex.disposables.Disposable;
import retrofit2.HttpException;

/**
 * <p>Description:
 * <p>
 * <p>Created by chenmeng on 2017/9/5.
 */

public abstract class BaseObserver<T> implements Observer<T> {


    @Override
    public void onSubscribe(Disposable d) {
    }

    @Override
    public void onNext(T t) {
        onSuccess(t);
    }

    @Override
    public void onError(Throwable e) {
        HttpResponseException responseException;
        if (e instanceof HttpException) {
            HttpException httpException = (HttpException) e;
            responseException = new HttpResponseException("网络请求出错", httpException.code());
        } else if (e instanceof HttpResponseException) {
            responseException = (HttpResponseException) e;
        } else if (e instanceof ConnectException) {
            ConnectException connectException = (ConnectException) e;
            responseException = new HttpResponseException("无网络",connectException.hashCode());
        } else {//其他或者没网会走这里
            responseException = new HttpResponseException("网络异常,请稍后重试", -1024);
        }
        onFailed(responseException);
    }

    @Override
    public void onComplete() {
    }

    protected abstract void onSuccess(T t);

    @CallSuper
    protected void onFailed(HttpResponseException responseException) {
        ToastUtil.showBottomShort(responseException.getMessage() + "(" + responseException.getStatus() + ")");
    }
}
