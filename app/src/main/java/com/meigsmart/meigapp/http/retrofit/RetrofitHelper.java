package com.meigsmart.meigapp.http.retrofit;


import com.meigsmart.meigapp.config.WebHostConfig;
import com.meigsmart.meigapp.http.okhttp.OkHttpHelper;

import retrofit2.Retrofit;
import retrofit2.adapter.rxjava2.RxJava2CallAdapterFactory;
import retrofit2.converter.gson.GsonConverterFactory;

/**
 * <p>Description:
 * <p>
 * <p>Created by chenmeng on 2017/9/3.
 */

public class RetrofitHelper {
    private static Retrofit retrofit;


    private RetrofitHelper() {
    }

    static {
        retrofit = new Retrofit.Builder()
                .baseUrl(WebHostConfig.getHostName())
                .client(OkHttpHelper.getClientCertificate())
                .addConverterFactory(StringConverterFactory.create()) //String 转换
                .addConverterFactory(GsonConverterFactory.create())
                .addCallAdapterFactory(RxJava2CallAdapterFactory.create())
                .validateEagerly(true)
                .build();
    }

    public static Retrofit getRetrofit() {
        return retrofit;
    }

}
