# A33 android4.4增加有线网络设置接口及相关说明


## framework修改

### 获取有线网络ip地址相关修改
修改frameworks/base/ethernet/java/android/net/ethernet/EthernetDataTracker.java  
增加函数setDevInfo
```java
    public void setDevInfo(EthernetDevInfo devinfo) {
        if ((mLinkProperties == null) || (devinfo == null))
			return;
		//Slog.d(TAG,"devinfo.getIfName():" + devinfo.getIfName() + "mLinkProperties.getInterfaceName()" + mLinkProperties.getInterfaceName());
		if(!devinfo.getIfName().equals(mLinkProperties.getInterfaceName()))
			return;
        DhcpInfo info = new DhcpInfo();
        for (LinkAddress la : mLinkProperties.getLinkAddresses()) {
            InetAddress addr = la.getAddress();
            if (addr instanceof Inet4Address) {
                //Slog.d(TAG,"ip:" + addr.getHostAddress());
				devinfo.setIpAddress(addr.getHostAddress());
                break;
            }
        }
        for (RouteInfo r : mLinkProperties.getRoutes()) {
            if (r.isDefaultRoute()) {
                InetAddress gateway = r.getGateway();
                if (gateway instanceof Inet4Address) {
					//Slog.d(TAG,"gateway:" + gateway.getHostAddress());
					devinfo.setGateWay(gateway.getHostAddress());
                }
            } else if (r.hasGateway() == false) {
                LinkAddress dest = r.getDestination();
                if (dest.getAddress() instanceof Inet4Address) {
					int netmask = NetworkUtils.prefixLengthToNetmaskInt(dest.getNetworkPrefixLength());
					//Slog.d(TAG,"netmask:" + netmask + " NetworkUtils.intToInetAddress(netmask):" + NetworkUtils.intToInetAddress(netmask));
					//Slog.d(TAG," NetworkUtils.intToInetAddress(netmask).getHostAddress():" + NetworkUtils.intToInetAddress(netmask).getHostAddress());
					devinfo.setNetMask(NetworkUtils.intToInetAddress(netmask).getHostAddress());
                }
            }
        }
        int dnsFound = 0;
        for (InetAddress dns : mLinkProperties.getDnses()) {
            if (dns instanceof Inet4Address) {
                if (dnsFound == 0) {
                    //info.dns1 = NetworkUtils.inetAddressToInt((Inet4Address)dns);
					//Slog.d(TAG,"dns1:" + dns.getHostAddress());
					devinfo.setDnsAddr(dns.getHostAddress());
                } else {
                    //info.dns2 = NetworkUtils.inetAddressToInt((Inet4Address)dns);
					//Slog.d(TAG,"dns2:" + dns.getHostAddress());
					devinfo.setDnsAddr(dns.getHostAddress());
                }
                if (++dnsFound > 1) break;
            }
        }
        return;
    }
```
修改frameworks/base/services/java/com/android/server/EthernetService.java  
中getDeviceNameList函数
```java
	public List<EthernetDevInfo> getDeviceNameList() {
		List<EthernetDevInfo> reDevs = new ArrayList<EthernetDevInfo>();

		synchronized(mDeviceMap){
			if(mDeviceMap.size() == 0)
				return null;
			for(EthernetDevInfo devinfo : mDeviceMap.values()){
				mTracker.setDevInfo(devinfo);
				reDevs.add(devinfo);
			}
		}
		return reDevs;
	}
```
增加
```
mTracker.setDevInfo(devinfo);
```
这样应用层就能获取到有线网络ip,应用层获取后面在说

### 设置有线网络相关修改

修改frameworks/base/ethernet/java/android/net/ethernet/EthernetManager.java  
增加
```
public static final String EXTRA_DEV_INFO	= "devinfo";
public static final int EVENT_TURN_ON                      = 8;
public static final int EVENT_TURN_OFF                      = 9;
public static final int EVENT_UPDATE_INFO                      = 10;
```
修改frameworks/base/services/java/com/android/server/EthernetService.java  
handleReceive函数中增加
```
case EthernetManager.EVENT_TURN_ON:
    //Log.d(TAG, "handleReceive EthernetManager.EVENT_TURN_ON");
    setState(EthernetManager.ETHERNET_STATE_ENABLED);
    break;
case EthernetManager.EVENT_TURN_OFF:
    //Log.d(TAG, "handleReceive EthernetManager.EVENT_TURN_OFF");
    setState(EthernetManager.ETHERNET_STATE_DISABLED);
    break;
case EthernetManager.EVENT_UPDATE_INFO:
    //Log.d(TAG, "handleReceive EthernetManager.EVENT_UPDATE_INFO");
    final EthernetDevInfo info = (EthernetDevInfo)intent.getParcelableExtra(EthernetManager.EXTRA_DEV_INFO);
    updateDevInfo(info);
    break;
```

## AS导入编译的framework使用
拷贝编译的
out/target/common/obj/JAVA_LIBRARIES/framework-base_intermediates/classes-full-debug.jar
到AS工程libs目录中  
修改工程build.gradle文件修改如下
```java
allprojects {
    repositories {
        google()
        jcenter()
    }
	<font color=red>
    gradle.projectsEvaluated {
        tasks.withType(JavaCompile) {
            options.compilerArgs.add
            ('-Xbootclasspath/p:app\\libs\\classes-full-debug.jar')
        }
    }
	</font>
}
```
增加gradle.projectsEvaluated  
修改app模块下build.gradle文件
```java
    defaultConfig {
        multiDexEnabled = true
        applicationId "com.gzease.sfjdemo"
        minSdkVersion 19
        targetSdkVersion 28
        versionCode 1
        versionName "1.0"
        testInstrumentationRunner "android.support.test.runner.AndroidJUnitRunner"
    }
```
defaultConfig中增加multiDexEnabled  
```java
dependencies {
    //implementation fileTree(include: ['*.jar'], dir: 'libs')
    implementation fileTree(dir: 'libs/armeabi-v7a')
    implementation 'com.android.support.constraint:constraint-layout:1.1.3'
    testImplementation 'junit:junit:4.12'
    androidTestImplementation 'com.android.support.test:runner:1.0.2'
    androidTestImplementation 'com.android.support.test.espresso:espresso-core:3.0.2'
    //compile project(path: ':hwc')
    compile 'com.android.support:multidex:1.0.0'
    compile(name: 'sfj-release-1.0', ext: 'aar')
    compileOnly files('libs/classes-full-debug.jar')
}
```
修改了
```java
//implementation fileTree(include: ['*.jar'], dir: 'libs')
implementation fileTree(dir: 'libs/armeabi-v7a')
compile 'com.android.support:multidex:1.0.0'
compileOnly files('libs/classes-full-debug.jar')
```
在最后增加
```java
preBuild {
    doLast {
        def imlFile = file(project.name + ".iml")
        println 'Change ' + project.name + '.iml order'
        try {
            def parsedXml = (new XmlParser()).parse(imlFile)
            def jdkNode = parsedXml.component[1].orderEntry.find { it.'@type' == 'jdk' }
            parsedXml.component[1].remove(jdkNode)
            def sdkString = "Android API " + android.compileSdkVersion.substring("android-".length()) + " Platform"
            new Node(parsedXml.component[1], 'orderEntry', ['type': 'jdk', 'jdkName': sdkString, 'jdkType': 'Android SDK'])
            groovy.xml.XmlUtil.serialize(parsedXml, new FileOutputStream(imlFile))
        } catch (FileNotFoundException e) {
            // nop, iml not found
        }
    }
}
```
修改完成后就能调用标准api没有的接口函数了  
其中有有线网络中的一些接口会提示，api版本要求21但我的是android4.4 api是19  
还会有一些接口抛安全异常，需要系统级应用才能调用，所以需要对文件系统进行修改  

## 测试demo
下面是截取的代码主要功能代码已经全部包括

```
package com.gzease.sfjdemo;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.graphics.Color;
import android.media.MediaPlayer;
import android.net.ConnectivityManager;
import android.net.DhcpInfo;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.NetworkInfo;
import android.net.ethernet.EthernetDevInfo;
import android.net.ethernet.EthernetManager;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.TextView;

import java.io.File;
import java.io.IOException;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;

public class Main extends Activity implements RadioGroup.OnCheckedChangeListener {
    private final String TAG = "Gzease-SfjDemo";
    private Handler handler = new Handler();
    private ConnectivityManager mService;
    private EthernetManager mEthManager;
    private BroadcastReceiver mEthStateReceiver;
    private IntentFilter mFilter;
    private List<EthernetDevInfo> mListDevices = new ArrayList<EthernetDevInfo>();
    private CheckBox mEthEnable;
    private TextView ethsta;
    private RadioButton ethmoded;
    private RadioButton ethmodes;
    private EditText ethmac;
    private EditText ethip;
    private EditText ethmask;
    private EditText ethgw;
    private EditText ethdns;
    private RadioGroup moderg;
    private Button ethset;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        mEthEnable = (CheckBox)findViewById(R.id.echeck);
        ethsta = (TextView)findViewById(R.id.ethsta);
        ethmac = (EditText)findViewById(R.id.ethmac);
        ethmac.setEnabled(false);
        ethmoded = (RadioButton) findViewById(R.id.ethmoded);
        ethmodes = (RadioButton) findViewById(R.id.ethmodes);
        moderg = (RadioGroup)findViewById(R.id.moderg);
        ethip = (EditText)findViewById(R.id.ethip);
        ethmask = (EditText)findViewById(R.id.ethmask);
        ethgw = (EditText)findViewById(R.id.ethgw);
        ethdns = (EditText)findViewById(R.id.ethdns);
        ethset = (Button)findViewById(R.id.ethset);

        mFilter = new IntentFilter();
        mFilter.addAction(EthernetManager.ETHERNET_STATE_CHANGED_ACTION);
        mFilter.addAction(EthernetManager.NETWORK_STATE_CHANGED_ACTION);

        mEthStateReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                handleEvent(context, intent);
            }
        };

        mService = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        mEthManager = EthernetManager.getInstance();

        if(mEthManager.getState() == EthernetManager.ETHERNET_STATE_ENABLED) {
            mEthEnable.setChecked(true);
            ethset.setEnabled(true);
        } else {
            mEthEnable.setChecked(false);
            ethset.setEnabled(false);
        }
        getMac();
        EthernetDevInfo saveInfo = mEthManager.getSavedConfig();
        updateInfo(saveInfo);

        mEthEnable.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
                Log.d(TAG,"ischeck: " + b);
                setEthEnabled(b);
                mEthEnable.setEnabled(false);
                ethset.setEnabled(b);
            }
        });
        moderg.setOnCheckedChangeListener(this);
        ethset.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                ethset.setEnabled(false);
                mEthEnable.setEnabled(false);
                sendStateBroadcast(EthernetManager.EVENT_UPDATE_INFO);
            }
        });
    }

    private void setEthEnabled(final boolean enable){
        if(enable) {
            sendStateBroadcast(EthernetManager.EVENT_UPDATE_INFO);
            sendStateBroadcast(EthernetManager.EVENT_TURN_ON);
        } else
            sendStateBroadcast(EthernetManager.EVENT_TURN_OFF);
    }

    private void sendStateBroadcast(int event) {
        Intent intent = new Intent(EthernetManager.NETWORK_STATE_CHANGED_ACTION);
        intent.putExtra(EthernetManager.EXTRA_ETHERNET_STATE, event);
        EthernetDevInfo info = new EthernetDevInfo();
        if(moderg.getCheckedRadioButtonId() == R.id.ethmodes)
            info.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_MANUAL);
        else
            info.setConnectMode(EthernetDevInfo.ETHERNET_CONN_MODE_DHCP);
        info.setDnsAddr(ethdns.getText().toString());
        info.setGateWay(ethgw.getText().toString());
        info.setIpAddress(ethip.getText().toString());
        EthernetDevInfo saveInfo = mEthManager.getSavedConfig();
        Log.d(TAG," saveInfo:" + saveInfo);
        if(saveInfo != null) {
            Log.d(TAG," sendStateBroadcast saveInfo.getIfName(): " + saveInfo.getIfName());
            info.setIfName(saveInfo.getIfName());

        } else
            info.setIfName("eth0");
        info.setHwaddr(getMac());
        info.setNetMask(ethmask.getText().toString());
        intent.putExtra(EthernetManager.EXTRA_DEV_INFO, info);
        sendBroadcast(intent);
    }

    private String getMac() {
        mListDevices = mEthManager.getDeviceNameList();
        for(EthernetDevInfo deviceinfo : mListDevices) {
            if(deviceinfo.getIfName().equals("eth0")) {
                Log.d(TAG,"deviceinfo.getHwaddr():" + deviceinfo.getHwaddr());
                return deviceinfo.getHwaddr();
            }
        }
        return "";
    }

    private void updateInfo(final EthernetDevInfo DevIfo) {
        handler.post(new Runnable() {
            @Override
            public void run() {
                String ifname = "";
                if(mService != null) {
                    NetworkInfo networkinfo = mService.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);
                    if(networkinfo.isConnected()) {
                        if(DevIfo != null)
                            ethsta.setText("已连接 设备:" + DevIfo.getIfName());
                        else
                            ethsta.setText("已连接");
                    } else {
                        ethsta.setText("未连接");
                    }
                }
                Log.d(TAG,"DevIfo:" + DevIfo);
                if(DevIfo != null)
                    ifname = DevIfo.getIfName();
                else
                    return;

                mListDevices = mEthManager.getDeviceNameList();
                if(mListDevices != null) {
                    for(EthernetDevInfo deviceinfo : mListDevices) {
                        Log.d(TAG,"DevIfo.getIfName():" + DevIfo.getIfName() + " deviceinfo.getIfName():" +deviceinfo.getIfName());
                        Log.d(TAG,"deviceinfo.getHwaddr():" + deviceinfo.getHwaddr());
                        if(deviceinfo.getIfName().equals(DevIfo.getIfName())) {
                            DevIfo.setHwaddr(deviceinfo.getHwaddr());
                            ethmac.setText(deviceinfo.getHwaddr().toUpperCase());
                            ethip.setText(deviceinfo.getIpAddress());
                            ethmask.setText(deviceinfo.getNetMask());
                            ethgw.setText(deviceinfo.getGateWay());
                            ethdns.setText(deviceinfo.getDnsAddr());
                            if(deviceinfo.getConnectMode() == EthernetDevInfo.ETHERNET_CONN_MODE_DHCP){
                                ethmoded.setChecked(true);
                                //ethmac.setEnabled(false);
                                ethip.setEnabled(false);
                                ethmask.setEnabled(false);
                                ethgw.setEnabled(false);
                                ethdns.setEnabled(false);
                            }else{
                                ethmodes.setChecked(true);
                                //ethmac.setEnabled(true);
                                ethip.setEnabled(true);
                                ethmask.setEnabled(true);
                                ethgw.setEnabled(true);
                                ethdns.setEnabled(true);
                            }
                        }
                    }
                }
            }
        });

    }

    private void handleEvent(Context context, Intent intent) {
        String action = intent.getAction();
        Log.d(TAG," action:" + action);
        if (EthernetManager.ETHERNET_STATE_CHANGED_ACTION.equals(action)) {
            final EthernetDevInfo devinfo = (EthernetDevInfo)
                    intent.getParcelableExtra(EthernetManager.EXTRA_ETHERNET_INFO);
            final int event = intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE,
                    EthernetManager.EVENT_NEWDEV);

            if(event == EthernetManager.EVENT_NEWDEV || event == EthernetManager.EVENT_DEVREM) {

            }
        } else if (EthernetManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
            final int event = intent.getIntExtra(EthernetManager.EXTRA_ETHERNET_STATE,
                    EthernetManager.EVENT_CONFIGURATION_SUCCEEDED);
            //Log.d(TAG," event:" + event);
            switch(event) {
                case EthernetManager.EVENT_CONFIGURATION_SUCCEEDED:
                    mEthEnable.setEnabled(true);
                    ethset.setEnabled(true);
                    break;
                case EthernetManager.EVENT_CONFIGURATION_FAILED:
                    break;
                case EthernetManager.EVENT_DISCONNECTED:
                    mEthEnable.setEnabled(true);
                    break;
                default:
                    break;
            }
            EthernetDevInfo saveInfo = mEthManager.getSavedConfig();
            updateInfo(saveInfo);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        Log.d(TAG,"onResume");
        registerReceiver(mEthStateReceiver, mFilter);
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG,"onPause");
        unregisterReceiver(mEthStateReceiver);
    }

    @Override
    public void onCheckedChanged(RadioGroup radioGroup, int id) {
        switch (id) {
            case R.id.ethmoded:
                //ethmac.setEnabled(false);
                ethip.setEnabled(false);
                ethmask.setEnabled(false);
                ethgw.setEnabled(false);
                ethdns.setEnabled(false);
                break;
            case R.id.ethmodes:
                //ethmac.setEnabled(true);
                ethip.setEnabled(true);
                ethmask.setEnabled(true);
                ethgw.setEnabled(true);
                ethdns.setEnabled(true);
                break;
        }
    }
}
```

## 参考链接
<https://blog.csdn.net/abs625/article/details/79611411>
