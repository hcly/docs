# android app JNI子线程回调java记录
## java层代码
```java
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
```
## JNI层代码
```c
#include <jni.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "android/log.h"
#include <pthread.h>

static const char *TAG="easelib-Hardware";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)

JavaVM *gJvm = NULL;
jclass gclass = NULL;
static int threadstart = 0;

JNIEXPORT jint JNICALL Java_com_gzease_jnicalljava_Hwc_init(JNIEnv * env, jobject thiz)
{
    //保存全局JVM以便在子线程中使用
    (*env)->GetJavaVM(env,&gJvm);
    jclass jclazz = (*env)->FindClass(env, "com/gzease/jnicalljava/Hwc");
    gclass = (*env)->NewGlobalRef(env, jclazz);

    jmethodID jmethodIDs = (*env)->GetMethodID(env, jclazz,"MainCallback","()V");
    jobject jobject = (*env)->AllocObject(env, jclazz);
    (*env)->CallVoidMethod(env, jobject,jmethodIDs);
}


static void* thread_exec(void *arg)
{
    JNIEnv *env = NULL;
    jmethodID jcallback= NULL;
    if((*gJvm)->AttachCurrentThread(gJvm, &env, NULL) != JNI_OK) {
        LOGE("%s: AttachCurrentThread() failed", __func__);
        return;
    }

    jcallback = (*env)->GetMethodID(env,gclass,"ThreadCallback", "()V");
    if (jcallback == NULL) {
        LOGE("%s: GetMethodID() failed", __func__);
        goto error;
    }
    jobject jobject = (*env)->AllocObject(env, gclass);
    while(threadstart) {
        (*env)->CallVoidMethod(env, jobject, jcallback);
        sleep(2);
    }
error:
    (*env)->DeleteGlobalRef(env, gclass);
    // 5. 释放 java 运行环境
    if((*gJvm)->DetachCurrentThread(gJvm) != JNI_OK) {
        LOGE("%s: DetachCurrentThread() failed", __func__);
    }
    pthread_exit(0);
}

JNIEXPORT jint JNICALL Java_com_gzease_jnicalljava_Hwc_threadstart(JNIEnv * env, jobject thiz)
{
    pthread_t threadid;
    threadstart = 1;
    if(pthread_create(&threadid,NULL,thread_exec,NULL)  != 0) {
        LOGE("thread start failed\n");
        return;
    }
}

JNIEXPORT jint JNICALL Java_com_gzease_jnicalljava_Hwc_threadstop(JNIEnv * env, jobject thiz)
{
    threadstart = 0;
}
```
##  activity代码
```java
package com.gzease.jnicalljava;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.CompoundButton;
import android.widget.ToggleButton;

public class MainActivity extends AppCompatActivity {
    private ToggleButton myonoff;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        myonoff = findViewById(R.id.myonoff);
        Hwc.init();
        myonoff.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
                if(isChecked)
                    Hwc.threadstart();
                else
                    Hwc.threadstop();
            }
        });
    }
}
```
主要测试JNI主线程调用JAVA和子线程调用JAVA
调用JNI init代码后会回调java层MainCallback
调用JNI threadstart代码后会循环回调java层ThreadCallback
其中有一个地方需要注意的是，JNI中获取class时，有网上有很多介绍使用到GetObjectClass，我最开始也是使用的这个函数，但是到GetMethodID时始终获取不到，一直报错，最后改为通过FindClass才能正常使用
执行输出如下:
```java
01-04 23:15:19.072  2026  2026 D easelib-Hwc:  version:1.0
01-04 23:15:19.073  2026  2026 D easelib-Hwc: MainCallback
01-04 23:15:23.326  2026  2057 D easelib-Hwc: ThreadCallback
01-04 23:15:25.327  2026  2057 D easelib-Hwc: ThreadCallback
01-04 23:15:27.327  2026  2057 D easelib-Hwc: ThreadCallback
01-04 23:15:29.328  2026  2057 D easelib-Hwc: ThreadCallback
01-04 23:15:31.329  2026  2057 D easelib-Hwc: ThreadCallback
```
