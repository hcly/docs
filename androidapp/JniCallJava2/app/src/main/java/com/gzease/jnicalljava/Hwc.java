package com.gzease.jnicalljava;

import android.util.Log;

import java.io.FileDescriptor;

public class Hwc {
    private static final String TAG = "easelib-Hwc";
    public native static int init();
    public native static int threadstart();
    public native static int threadstop();

    public void MainCallback() {
        Log.d(TAG,"MainCallback");
    }

    public void ThreadCallback() {
        Log.d(TAG,"ThreadCallback");
    }

    static {
        Log.d(TAG," version:" + BuildConfig.VERSION_NAME);
        System.loadLibrary("EaseHardLib");
    }
}
