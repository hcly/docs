# rk3288 android7增加有线网络设置及相关接口
 主要增加系统时间设置，动态显示隐藏导航栏，动态静态IP设置，静默安装
## framework修改
### 增加service
frameworks/base/services/core/java/com/android/server/GzeaseService.java
```java
package com.android.server;

import com.android.server.SystemService;
import android.util.Slog;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.os.SystemClock;
import java.util.regex.Pattern;
import java.lang.Integer;
import java.net.InetAddress;
import java.net.Inet4Address;
import android.net.EthernetManager;
import android.net.IpConfiguration;
import android.net.IpConfiguration.IpAssignment;
import android.net.IpConfiguration.ProxySettings;
import android.net.StaticIpConfiguration;
import android.net.NetworkUtils;
import android.net.LinkAddress;
import android.net.LinkProperties;
import java.io.ByteArrayOutputStream;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PackageParser.PackageParserException;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.io.File;
import android.net.Uri;
import android.app.PackageInstallObserver;
import android.os.Bundle;
import android.os.UserHandle;
import android.os.Handler;

public class GzeaseService extends SystemService {
    private final String TAG = "GzeaseService";
	private final String ACTION = "com.gzease.GzeaseService.user";
	private static String ACTION_HIDE_NAV_BAR = "com.gzease.action.hide_nav_bar";
	private static String ACTION_SHOW_NAV_BAR = "com.gzease.action.show_nav_bar";
    private final static String CMD_KEY = "cmd";
    private final static int CMD_ID_SETTIME = 0;
    private final static int CMD_ID_HIDENAVBAR = 1;
    private final static int CMD_ID_ETH = 2;
    private final static int CMD_ID_APPINSTALL = 3;
    private final static boolean ETH_MODE_DHCP = true;
    private final static boolean ETH_MODE_STATIC = false;
	private final static String VAL_SETTIME = "settime";
	private final static String VAL_HIDENAVBAR = "hidenavbar";
	private final static String VAL_ETH_MODE = "eth_mode";
	private final static String VAL_ETH_IP = "eth_ip";
	private final static String VAL_ETH_NETMASK = "eth_netmask";
	private final static String VAL_ETH_GATEWAY = "eth_gateway";
	private final static String VAL_ETH_DNS1 = "eth_dns1";
	private final static String VAL_ETH_DNS2 = "eth_dns2";
	private final static String VAL_APPINSTALL_FILE = "appinstall_file";
	private final static String VAL_APPINSTALL_START = "appinstall_start";
    private final boolean DEBUG = true;
	private Handler mHandler = new Handler();
	private Context mContext;
	private CmdRecv cmdrecv;
    public GzeaseService(Context context) {
        super(context);
		mContext = context;
		if (DEBUG) Slog.d(TAG, "GzeaseService()");
    }

    @Override
    public void onStart() {
        if (DEBUG) Slog.d(TAG, "onStart()");
		cmdrecv = new CmdRecv();
		IntentFilter filter = new IntentFilter();
		filter.addAction(ACTION);
		mContext.registerReceiver(cmdrecv, filter);
    }

	private class CmdRecv extends BroadcastReceiver {
		@Override
		public void onReceive(Context context, Intent intent) {
			// TODO Auto-generated method stub
			int cmd = intent.getIntExtra(CMD_KEY, -1);
			if (DEBUG) Slog.d(TAG, "onReceive() cmd:" + cmd);
			switch(cmd) {
				case CMD_ID_SETTIME:
					setTime(intent);
					break;
				case CMD_ID_HIDENAVBAR:
					hideNavBar(intent);
					break;
				case CMD_ID_ETH:
					setEth(intent);
					break;
				case CMD_ID_APPINSTALL:
					appInstall(intent);
					break;
			}
		}
	}

	private void setTime(Intent intent) {
		long time = intent.getLongExtra(VAL_SETTIME, System.currentTimeMillis());
        boolean ret = SystemClock.setCurrentTimeMillis(time);
		if (DEBUG) Slog.d(TAG, "setTime() setCurrentTimeMillis:" + time + " ret:" + ret);
	}

	private void hideNavBar(Intent intent) {
		Intent mychange;
		boolean hide = intent.getBooleanExtra(VAL_HIDENAVBAR,false);
		if (DEBUG) Slog.d(TAG, "hideNavBar() hide:" + hide);
		if(hide)
			mychange = new Intent(ACTION_HIDE_NAV_BAR);
		else
			mychange = new Intent(ACTION_SHOW_NAV_BAR);
		mContext.sendBroadcast(mychange);
	}

	private void setEth(Intent intent) {
		String mEthIpAddress = null;
		String mEthNetmask = null;
		String mEthGateway = null;
		String mEthdns1 = null;
		String mEthdns2 = null;
		EthernetManager mEthManager = (EthernetManager)mContext.getSystemService(Context.ETHERNET_SERVICE);
		StaticIpConfiguration mStaticIpConfiguration = new StaticIpConfiguration();
		boolean dhcp = intent.getBooleanExtra(VAL_ETH_MODE,true);
		if (DEBUG) Slog.d(TAG, "setEth() dhcp:" + dhcp);
        if (mEthManager == null) {
			Slog.e(TAG, "get ethernet manager failed");
			return;
	    }

		if(dhcp) {
			mEthManager.setConfiguration(new IpConfiguration(IpAssignment.DHCP, ProxySettings.NONE,null,null));
		} else {
			mEthIpAddress = intent.getStringExtra(VAL_ETH_IP);
			mEthNetmask = intent.getStringExtra(VAL_ETH_NETMASK);
			mEthGateway = intent.getStringExtra(VAL_ETH_GATEWAY);
			mEthdns1 = intent.getStringExtra(VAL_ETH_DNS1);
			mEthdns2 = intent.getStringExtra(VAL_ETH_DNS2);
			if (DEBUG) Slog.d(TAG, "setEth() mEthIpAddress:" + mEthIpAddress
			+ " mEthNetmask:" + mEthNetmask
			+ " mEthGateway:" + mEthGateway
			+ " mEthdns1:" + mEthdns1
			+ " mEthdns2:" + mEthdns2);
			Inet4Address inetAddr = getIPv4Address(mEthIpAddress);
			int prefixLength = maskStr2InetMask(mEthNetmask);
		    InetAddress gatewayAddr =getIPv4Address(mEthGateway);
		    InetAddress dnsAddr = getIPv4Address(mEthdns1);
		    if (inetAddr.getAddress().toString().isEmpty() || prefixLength ==0 || gatewayAddr.toString().isEmpty()
			  || dnsAddr.toString().isEmpty()) {
				Slog.e(TAG, "ip,mask or dnsAddr is wrong");
				return;
			}

	        mStaticIpConfiguration.ipAddress = new LinkAddress(inetAddr, prefixLength);
        	mStaticIpConfiguration.gateway=gatewayAddr;
			mStaticIpConfiguration.dnsServers.add(dnsAddr);
        	if (!mEthdns2.isEmpty()) {
            	mStaticIpConfiguration.dnsServers.add(getIPv4Address(mEthdns2));
			}
			mEthManager.setConfiguration(new IpConfiguration(IpAssignment.STATIC, ProxySettings.NONE,mStaticIpConfiguration,null));
		}
	}

	private void appInstall(Intent intent) {
		String packageName = null;
		String path = intent.getStringExtra(VAL_APPINSTALL_FILE);
		boolean start = intent.getBooleanExtra(VAL_APPINSTALL_START,false);
 		PackageManager pm = mContext.getPackageManager();
        int installFlags = PackageManager.INSTALL_REPLACE_EXISTING | PackageManager.INSTALL_ALLOW_DOWNGRADE;
		File apkfile = new File(path);
		InstallObserver observer = new InstallObserver();
		observer.setStart(start);
		pm.installPackage(Uri.fromFile(apkfile),observer,installFlags,null);
	}

    private class InstallObserver extends PackageInstallObserver {
		private boolean start = false;
        @Override
        public void onPackageInstalled(String basePackageName, int returnCode, String msg, Bundle extras) {
        	if (DEBUG)Slog.d(TAG, "InstallObserver onPackageInstalled basePackageName:" + basePackageName);
			final String pakname = basePackageName;
			if(start) {
				mHandler.postDelayed(new Runnable() {
				    @Override
				    public void run() {
			 			Intent myint = new Intent();
						PackageManager pm = mContext.getPackageManager();
						if (DEBUG)Slog.d(TAG, "InstallObserver onPackageInstalled checkPackage:" + checkPackage(pakname));
			 			myint = pm.getLaunchIntentForPackage(pakname);
			 			myint.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
						//myint.setFlags(Intent.FLAG_ACTIVITY_REORDER_TO_FRONT);
						mContext.startActivityAsUser(myint, UserHandle.CURRENT);
				    }
				},1000);
			}
        }

		public void setStart(boolean isstart) {
			this.start = isstart;
		}
    }

	public boolean checkPackage(String packageName) {  
		if (packageName == null || "".equals(packageName))  
			return false;  
		try {  
			mContext.getPackageManager().getApplicationInfo(packageName, PackageManager.GET_UNINSTALLED_PACKAGES);  
			return true;  
		} catch (NameNotFoundException e) {  
	        return false;
	    }
	}

    private Inet4Address getIPv4Address(String text) {
        try {
            return (Inet4Address) NetworkUtils.numericToInetAddress(text);
        } catch (IllegalArgumentException|ClassCastException e) {
            return null;
        }
    }

    private int maskStr2InetMask(String maskStr) {
    	StringBuffer sb ;
    	String str;
    	int inetmask = 0;
    	int count = 0;
    	/*
    	 * check the subMask format
    	 */
      	Pattern pattern = Pattern.compile("(^((\\d|[01]?\\d\\d|2[0-4]\\d|25[0-5])\\.){3}(\\d|[01]?\\d\\d|2[0-4]\\d|25[0-5])$)|^(\\d|[1-2]\\d|3[0-2])$");
    	if (pattern.matcher(maskStr).matches() == false) {
    		Slog.e(TAG,"subMask is error");
    		return 0;
    	}

    	String[] ipSegment = maskStr.split("\\.");
    	for(int n =0; n<ipSegment.length;n++) {
    		sb = new StringBuffer(Integer.toBinaryString(Integer.parseInt(ipSegment[n])));
    		str = sb.reverse().toString();
    		count=0;
    		for(int i=0; i<str.length();i++) {
    			i=str.indexOf("1",i);
    			if(i==-1)  
    				break;
    			count++;
    		}
    		inetmask+=count;
    	}
    	return inetmask;
    }
}
```
在frameworks/base/services/java/com/android/server/SystemServer.java
函数startOtherServices中增加
```java
//add by leijie
mSystemServiceManager.startService(GzeaseService.class);
```
### 动态隐藏显示导航栏还额外修改
frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/phone/PhoneStatusBar.java
增加
```java
public static String ACTION_HIDE_NAV_BAR = "com.gzease.action.hide_nav_bar";
public static String ACTION_SHOW_NAV_BAR = "com.gzease.action.show_nav_bar";
```
在start()函数中增加
```java
//add by hclydao
    IntentFilter filter = new IntentFilter();
    filter.addAction(ACTION_HIDE_NAV_BAR);
    filter.addAction(ACTION_SHOW_NAV_BAR);
    mContext.registerReceiver(mHideNavBarReceiver, filter);
```
增加
```java
//add by hclydao
    BroadcastReceiver mHideNavBarReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
			if(intent.getAction().equals(ACTION_HIDE_NAV_BAR)) {
				hideNavBar(true);
			} else if(intent.getAction().equals(ACTION_SHOW_NAV_BAR)) {
				hideNavBar(false);
			}
        }
    };
	private boolean myhide = false;
	//add by hclydao
	public void hideNavBar(boolean hide) {
		if (DEBUG) Log.d(TAG, "hideNavBar hide:" + hide);
		if(hide) {
			if(myhide != hide) {
				myhide = hide;
				if (mNavigationBarView != null)
				    mWindowManager.removeViewImmediate(mNavigationBarView);
			}
		} else {
			if(myhide != hide) {
				myhide = hide;
				if (mNavigationBarView != null)
					mWindowManager.addView(mNavigationBarView, getNavigationBarLayoutParams());
			}
		}
	}
```
## app接口函数
### 公共定义Ease3288
```java
package com.gzease.easesysteminterface;

public class Ease3288 {
    public final static String ACTION = "com.gzease.GzeaseService.user";
    public final static String CMD_KEY = "cmd";
    public final static int CMD_ID_SETTIME = 0;
    public final static int CMD_ID_HIDENAVBAR = 1;
    public final static int CMD_ID_ETH = 2;
    public final static int CMD_ID_APPINSTALL = 3;
    public final static boolean ETH_MODE_DHCP = true;
    public final static boolean ETH_MODE_STATIC = false;


    public final static String VAL_SETTIME = "settime";
    public final static String VAL_HIDENAVBAR = "hidenavbar";
    public final static String VAL_ETH_MODE = "eth_mode";
    public final static String VAL_ETH_IP = "eth_ip";
    public final static String VAL_ETH_NETMASK = "eth_netmask";
    public final static String VAL_ETH_GATEWAY = "eth_gateway";
    public final static String VAL_ETH_DNS1 = "eth_dns1";
    public final static String VAL_ETH_DNS2 = "eth_dns2";
    public final static String VAL_APPINSTALL_FILE = "appinstall_file";
    public final static String VAL_APPINSTALL_START = "appinstall_start";
}
```
### 系统时间设置接口
```java
private void setTime() {
    Log.d(TAG,"+++setTime");
    Calendar time = Calendar.getInstance();
    time.set(Calendar.YEAR,2020);
    int month = 1;//设置月份
    time.set(Calendar.MONTH,month - 1);
    time.set(Calendar.DAY_OF_MONTH,10);
    time.set(Calendar.HOUR_OF_DAY,12);
    time.set(Calendar.MINUTE,00);
    time.set(Calendar.SECOND,59);
    long mytime = time.getTimeInMillis();
    if (mytime / 1000 < Integer.MAX_VALUE) {
        Log.d(TAG,"+++mytime:" + mytime + " currentTimeMillis: " + System.currentTimeMillis());
        Intent intent = new Intent();
        intent.setAction(Ease3288.ACTION);
        intent.putExtra(Ease3288.CMD_KEY,Ease3288.CMD_ID_SETTIME);
        intent.putExtra(Ease3288.VAL_SETTIME,mytime);
        sendBroadcast(intent);
    }
}
```

### 动态显示隐藏导航栏接口
```java
private void HideNavBar(boolean hide) {
    Intent intent = new Intent();
    intent.setAction(Ease3288.ACTION);
    intent.putExtra(Ease3288.CMD_KEY,Ease3288.CMD_ID_HIDENAVBAR);
    intent.putExtra(Ease3288.VAL_HIDENAVBAR,hide);
    sendBroadcast(intent);
}
```
### 动态静态IP设置接口
```java
private void setEthDhcp() {
    Intent intent = new Intent();
    intent.setAction(Ease3288.ACTION);
    intent.putExtra(Ease3288.CMD_KEY,Ease3288.CMD_ID_ETH);
    intent.putExtra(Ease3288.VAL_ETH_MODE,Ease3288.ETH_MODE_DHCP);
    sendBroadcast(intent);
}

private void setEthStatic() {
    Intent intent = new Intent();
    intent.setAction(Ease3288.ACTION);
    intent.putExtra(Ease3288.CMD_KEY,Ease3288.CMD_ID_ETH);
    intent.putExtra(Ease3288.VAL_ETH_MODE,Ease3288.ETH_MODE_STATIC);
    intent.putExtra(Ease3288.VAL_ETH_IP,"192.168.1.15");
    intent.putExtra(Ease3288.VAL_ETH_NETMASK,"255.255.255.0");
    intent.putExtra(Ease3288.VAL_ETH_GATEWAY,"192.168.1.1");
    intent.putExtra(Ease3288.VAL_ETH_DNS1,"192.168.1.1");
    intent.putExtra(Ease3288.VAL_ETH_DNS2,"");//可为空
    sendBroadcast(intent);
}
```
### 静默安装接口
```java
private void appInstall() {
    String apkfile = "/sdcard/test.apk";
    File file  = new File(apkfile);
    Log.d(TAG,"apkfile: " + apkfile + " file is file:" + file.isFile());
    if(file.exists()) {
        Intent intent = new Intent();
        intent.setAction(Ease3288.ACTION);
        intent.putExtra(Ease3288.CMD_KEY, Ease3288.CMD_ID_APPINSTALL);
        intent.putExtra(Ease3288.VAL_APPINSTALL_FILE, apkfile);
        intent.putExtra(Ease3288.VAL_APPINSTALL_START, false);
        sendBroadcast(intent);
    }
}
```
