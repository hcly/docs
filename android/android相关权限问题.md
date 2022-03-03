# android相关权限问题修改
## a33 android6.0 su提权修改
```java
diff --git a/frameworks/base/cmds/app_process/app_main.cpp b/frameworks/base/cmds/app_process/app_main.cpp
index df1b67f..7518703 100755
--- a/frameworks/base/cmds/app_process/app_main.cpp
+++ b/frameworks/base/cmds/app_process/app_main.cpp
@@ -201,6 +201,7 @@ static const char ZYGOTE_NICE_NAME[] = "zygote";

 int main(int argc, char* const argv[])
 {
+	/* modify by leijie
     if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0) {
         // Older kernels don't understand PR_SET_NO_NEW_PRIVS and return
         // EINVAL. Don't die on such kernels.
@@ -208,7 +209,7 @@ int main(int argc, char* const argv[])
             LOG_ALWAYS_FATAL("PR_SET_NO_NEW_PRIVS failed: %s", strerror(errno));
             return 12;
         }
-    }
+    }*/

     AppRuntime runtime(argv[0], computeArgBlockSize(argc, argv));
     // Process command line arguments
diff --git a/frameworks/base/core/jni/com_android_internal_os_Zygote.cpp b/frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
old mode 100644
new mode 100755
index b431a3f..78d2a8a
--- a/frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
+++ b/frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
@@ -222,6 +222,7 @@ static void EnableKeepCapabilities(JNIEnv* env) {
 }

 static void DropCapabilitiesBoundingSet(JNIEnv* env) {
+  /* modify by leiije
   for (int i = 0; prctl(PR_CAPBSET_READ, i, 0, 0, 0) >= 0; i++) {
     int rc = prctl(PR_CAPBSET_DROP, i, 0, 0, 0);
     if (rc == -1) {
@@ -233,7 +234,7 @@ static void DropCapabilitiesBoundingSet(JNIEnv* env) {
         RuntimeAbort(env);
       }
     }
-  }
+  }*/
 }

 static void SetCapabilities(JNIEnv* env, int64_t permitted, int64_t effective) {
diff --git a/system/core/libcutils/fs_config.c b/system/core/libcutils/fs_config.c
old mode 100644
new mode 100755
index 9a1ad19..32a807d
--- a/system/core/libcutils/fs_config.c
+++ b/system/core/libcutils/fs_config.c
@@ -123,7 +123,8 @@ static const struct fs_path_config android_files[] = {

     /* the following five files are INTENTIONALLY set-uid, but they
      * are NOT included on user builds. */
-    { 04750, AID_ROOT,      AID_SHELL,     0, "system/xbin/su" },
+    //{ 04750, AID_ROOT,      AID_SHELL,     0, "system/xbin/su" },
+    { 06755, AID_ROOT,      AID_SHELL,     0, "system/xbin/su" },
     { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/librank" },
     { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/procrank" },
     { 06755, AID_ROOT,      AID_ROOT,      0, "system/xbin/procmem" },
diff --git a/system/extras/su/su.c b/system/extras/su/su.c
old mode 100644
new mode 100755
index d932c1b..79a3818
--- a/system/extras/su/su.c
+++ b/system/extras/su/su.c
@@ -81,8 +81,9 @@ void extract_uidgids(const char* uidgids, uid_t* uid, gid_t* gid, gid_t* gids, i
 }

 int main(int argc, char** argv) {
-    uid_t current_uid = getuid();
-    if (current_uid != AID_ROOT && current_uid != AID_SHELL) error(1, 0, "not allowed");
+    /*uid_t current_uid = getuid();
+    if (current_uid != AID_ROOT && current_uid != AID_SHELL) error(1, 0, "not allowed");*/
+    //modify by leijie

     // Handle -h and --help.
     ++argv;
```
## app调用dmesg保存内核日志问题
需要修改内核
```C
kernel/printk.c check_syslog_permissions return 0
```
app内核日志保存功能
```java
    //dmesg -c must modify kernel/printk.c check_syslog_permissions return 0
    @Override
    public void run() {
        super.run();
        Process logcatProc;
        BufferedReader mReader = null;
        try {
            FileOutputStream out = new FileOutputStream(new File(BasePathVar.PATH_KLOG),true);;
            while (isrun) {
                if(!BaseVar.logPause) {
                    logcatProc = Runtime.getRuntime().exec("dmesg -c");
                    mReader = new BufferedReader(new InputStreamReader(
                            logcatProc.getInputStream()), 1024);
                    String line = null;
                    while((line = mReader.readLine()) != null) {
                        //new LogTools(TAG,"line len:" + line.length());
                        if(line.length() == 0)
                            break;
                        out.write((line + "\n").getBytes());
                        out.flush();
                        //out.getFD().sync();
                    }
                }
                Thread.sleep(1000);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
```
