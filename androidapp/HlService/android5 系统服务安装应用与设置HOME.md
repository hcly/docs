# android5 系统服务安装应用与设置HOME
## 应用安装
```java
private void appInstall(String path) {
  if(debug)Log.d(TAG,"appInstall file: " + path);
  PackageManager pm = getPackageManager();
      int installFlags = PackageManager.INSTALL_REPLACE_EXISTING | PackageManager.INSTALL_ALLOW_DOWNGRADE;
  File apkfile = new File(path);
  InstallObserver observer = new InstallObserver();
  pm.installPackage(Uri.fromFile(apkfile),observer,installFlags,null);
}

private class InstallObserver extends PackageInstallObserver {
    @Override
    public void onPackageInstalled(String basePackageName, int returnCode, String msg, Bundle extras) {
      String defHome = "com.android.launcher3";
      if (debug)Log.d(TAG, "InstallObserver onPackageInstalled basePackageName:" + basePackageName);
      checkHome(basePackageName);
    }
}
```
appInstall函数入参为apk文件绝对路径例如/stoarge/sdcard1/home.apk
## 设置HOME并启动
```java
private void checkHome(String pakname) {
  PackageManager mPm = getPackageManager();
      ArrayList<ResolveInfo> homeActivities = new ArrayList<ResolveInfo>();
      ComponentName currentDefaultHome  = mPm.getHomeActivities(homeActivities);
      if(debug)Log.d(TAG,"homeActivities.size(): " + homeActivities.size());
      if(homeActivities.size() > 1) {
        ComponentName[] mHomeComponentSet = new ComponentName[homeActivities.size()];
        ComponentName homeactivityName = null;
      for (int i = 0; i < homeActivities.size(); i++) {
          final ResolveInfo candidate = homeActivities.get(i);
          final ActivityInfo info = candidate.activityInfo;
          ComponentName activityName = new ComponentName(info.packageName, info.name);
          mHomeComponentSet[i] = activityName;
          if(pakname.equals(info.packageName))
            homeactivityName = activityName;
        }
        if(debug)Log.d(TAG,"homeactivityName: " + homeactivityName);
        if(homeactivityName != null) {
        IntentFilter mHomeFilter = new IntentFilter(Intent.ACTION_MAIN);
      mHomeFilter = new IntentFilter(Intent.ACTION_MAIN);
      mHomeFilter.addCategory(Intent.CATEGORY_HOME);
      mHomeFilter.addCategory(Intent.CATEGORY_DEFAULT);
          mPm.replacePreferredActivity(mHomeFilter, IntentFilter.MATCH_CATEGORY_EMPTY,
              mHomeComponentSet, homeactivityName);
      Intent myint = new Intent();
      PackageManager pm = getPackageManager();
      myint = pm.getLaunchIntentForPackage(pakname);
      myint.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      //myint.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
      startActivity(myint);
          }
      }
}
```
checkHome入参为应用包名，获取到home应用列表后其中如果包含pakname就设置pakname为默认HOME
并启动，设备重启就默认就会使用这个HOME
需要包含的系统包名如下
```java
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import android.net.Uri;
import android.app.PackageInstallObserver;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.Handler;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PackageParser.PackageParserException;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.ResolveInfo;
import java.util.ArrayList;
import java.util.List;
import android.content.IntentFilter;
import android.content.ComponentName;
import android.content.pm.ActivityInfo;
import android.content.BroadcastReceiver;
import android.content.Context;
```
