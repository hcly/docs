package com.gzease.hlservice;

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

public class MainService extends Service {
    private final String TAG = "gzease-MainService";
    private final boolean debug = true;
    private MediaRecv mediaRecv;
    private Handler handler = new Handler();
    @Override
    public void onCreate() {
        super.onCreate();
        if(debug)Log.d(TAG,"onCreate");
        new Thread(CheckApp).start();
        mediaRecv = new MediaRecv();
        IntentFilter intentFilter = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addDataScheme("file");
        registerReceiver(mediaRecv, intentFilter);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if(debug)Log.d(TAG,"onDestroy");
        unregisterReceiver(mediaRecv);
    }

    private Runnable CheckApp = new Runnable() {
        @Override
        public void run() {
            checkDir(DataBase.tfdisk + DataBase.appdir);
            checkDir(DataBase.usbdisk + DataBase.appdir);
        }
    };


    private class MediaRecv extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
    		if(debug)Log.d(TAG,"MediaRecv :" + intent.getAction());
            if(intent.getAction() == Intent.ACTION_MEDIA_MOUNTED) {
                 //new Thread(CheckApp).start();
                 handler.postDelayed(CheckApp,1000);
            }
        }
    }
    
    private void checkDir(String path) {
        File dir = new File(path);
        if(debug)Log.d(TAG,path + " dir.exists(): " + dir.exists());
        if(!dir.exists())
            return;
        File[] files = dir.listFiles();
        if(debug)Log.d(TAG,path + " files: " + files);
        if(files != null) {
            for (File spec : files) {
                if (debug) Log.d(TAG, spec.getName() + " apk: " + spec.getName().endsWith(".apk"));
                if (spec.getName().endsWith(".apk")) {
					appInstall(path + "/" + spec.getName());
                }
            }
        }
    }
	/*
    private List<String> getHomes() {
		List<String> names = new ArrayList<String>();
		PackageManager packageManager = getPackageManager();
		Intent intent = new Intent(Intent.ACTION_MAIN);
		intent.addCategory(Intent.CATEGORY_HOME);
		List<ResolveInfo> resolveInfo = packageManager.queryIntentActivities(intent,
				PackageManager.MATCH_DEFAULT_ONLY);
		for (ResolveInfo ri : resolveInfo) {
			names.add(ri.activityInfo.packageName);
		}
		return names;
	}*/
	
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
        	/*
        	List<String> homelist = getHomes();
        	if(debug)Log.d(TAG," getHomes211: " + homelist + " homelist.size():" + homelist.size());
        	if(homelist.size() > 1) {
        		for(String pak : homelist) {
		        	if(debug)Log.d(TAG, pak + " pak.equals(basePackageName):" + pak.equals(basePackageName));
        			if(pak.equals(basePackageName)) {
			 			Intent myint = new Intent();
						PackageManager pm = getPackageManager();
			 			myint = pm.getLaunchIntentForPackage(basePackageName);
			 			myint.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
						//myint.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
						startActivityAsUser(myint, UserHandle.CURRENT);
        				break;
        			}
        		}
        	}
        	*/
        }
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
