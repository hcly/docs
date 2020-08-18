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