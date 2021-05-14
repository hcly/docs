# android4.4 add ethernet


[原文地址](https://blog.csdn.net/hclydao/article/details/50976868)

拷贝frameworks/base/ethernet到frameworks/base下
修改frameworks/base/Android.mk
在
```java
	wifi/java/android/net/wifi/p2p/IWifiP2pManager.aidl \
```
下加上如下代码
```java
	ethernet/java/android/net/ethernet/IEthernetManager.aidl \
```
修改build/core/pathmap.mk在
FRAMEWORKS_BASE_SUBDIRS中加上ethernet
拷贝EthernetService.java到frameworks/base/services/java/com/android/server/下
修改frameworks/base/core/java/android/content/Context.java
在
```java
    public static final String WIFI_P2P_SERVICE = "wifip2p";
```
下加上如下内容
```java
    /**
     * Use with {@link #getSystemService} to retrieve a {@link
     * android.net.ethernet.EthernetManager} for handling management of
     * Ethernet access.
     *
     * @see #getSystemService
     * @see android.net.ethernet.EthernetManager
     */
    public static final String ETH_SERVICE = "ethernet";//add by hclydao
```
修改frameworks/base/core/java/android/app/ContextImpl.java
在
```java
import android.net.wifi.p2p.IWifiP2pManager;
import android.net.wifi.p2p.WifiP2pManager;
```
下增加如下内容
```java
import android.net.ethernet.IEthernetManager;
import android.net.ethernet.EthernetManager;
```
在
```java
        registerService(WIFI_P2P_SERVICE, new ServiceFetcher() {
                public Object createService(ContextImpl ctx) {
                    IBinder b = ServiceManager.getService(WIFI_P2P_SERVICE);
                    IWifiP2pManager service = IWifiP2pManager.Stub.asInterface(b);
                    return new WifiP2pManager(service);
                }});
```
下增加如下内容
```java
        registerService(ETH_SERVICE, new ServiceFetcher() {
                public Object createService(ContextImpl ctx) {
                    IBinder b = ServiceManager.getService(ETH_SERVICE);
                    IEthernetManager service = IEthernetManager.Stub.asInterface(b);
                    return new EthernetManager(service, ctx.mMainThread.getHandler());
                }}); //add by hclydao
```
修改frameworks/base/services/java/com/android/server/ConnectivityService.java
增加
```java
import android.net.ethernet.EthernetManager;//add by hclydao
import android.net.ethernet.EthernetStateTracker;
```
注释掉
```java
//import android.net.EthernetDataTracker;
```
在
```java
            try {
                tracker = netFactory.createTracker(targetNetworkType, config);
                mNetTrackers[targetNetworkType] = tracker;
            } catch (IllegalArgumentException e) {
                Slog.e(TAG, "Problem creating " + getNetworkTypeName(targetNetworkType)
                        + " tracker: " + e);
                continue;
            }
```
下加上如下代码
```java
			if(mNetConfigs[targetNetworkType].radio == ConnectivityManager.TYPE_ETHERNET) { //add by hclydao
                EthernetService ethernet = new EthernetService(context, (EthernetStateTracker)mNetTrackers[targetNetworkType]);
                ServiceManager.addService(Context.ETH_SERVICE, ethernet);
                mNetTrackers[targetNetworkType].startMonitoring(context, mTrackerHandler);
			}
```
注释掉
```java
                    //return EthernetDataTracker.getInstance();
```
增加如下代码
```java
					return new EthernetStateTracker(targetNetworkType, config.name);//add by hclydao
```
拷贝android_net_ethernet.cpp到frameworks/base/core/jni目录下
修改frameworks/base/core/jni下的Android.mk
在
```java
	android_net_wifi_WifiNative.cpp \
```
下加上如下代码
```java
	android_net_ethernet.cpp \
```

修改frameworks/base/core/jni/AndroidRuntime.cpp
在
```java
extern int register_android_net_wifi_WifiNative(JNIEnv* env);下
```
加上
```java
extern int register_android_net_ethernet_EthernetManager(JNIEnv* env);//add by hclydao
```
在
```java
REG_JNI(register_android_net_wifi_WifiNative),
```
下加上如下代码
```java
REG_JNI(register_android_net_ethernet_EthernetManager),
```

在framework/base/core/java/android/provider/Settings.java中
```java
       public static final String WIFI_ON = "wifi_on";
```
下加上如下代码
```java
         public static final String ETH_ON      = "eth_on";
         public static final String ETH_MODE    = "eth_mode";
         public static final String ETH_IP      = "eth_ip";
         public static final String ETH_MASK    = "eth_mask";
         public static final String ETH_DNS     = "eth_dns";
         public static final String ETH_ROUTE   = "eth_route";
         public static final String ETH_CONF    = "eth_conf";
         public static final String ETH_IFNAME  = "eth_ifname";
```
拷贝eth_configure.xml到Settings/res/layout/
拷贝ic_setttings_ethernet.png到Settings/res/drawable-hdpi与drawable-mdpi
拷贝ethernet_settings.xml到Settings/res/xml下
修改xml下settings_headers.xml在wifi下增加如下内容
```java
    <!-- Ethernet -->
   <header
        android:id="@+id/ethernet_settings"
        android:title="@string/eth_setting"
        android:icon="@drawable/ic_settings_ethernet"
        android:fragment="com.android.settings.ethernet.EthernetSettings"/>
```
修改values/strings.xml增加如下内容
```java
    <!-- Ethernet configuration dialog -->
    <string name="eth_config_title">Configure Ethernet device</string>
    <string name="eth_setting">Ethernet</string>
    <string name="eth_dev_list">Ethernet Devices:</string>
    <string name="eth_con_type">Connection Type</string>
    <string name="eth_con_type_dhcp">DHCP</string>
    <string name="eth_con_type_manual">Static IP</string>
    <string name="eth_dns">DNS address</string>
    <string name="eth_gw">Gateway address</string>
    <string name="eth_ipaddr">IP address</string>
    <string name="eth_quick_toggle_title">Ethernet</string>
    <string name="eth_quick_toggle_summary">Turn on Ethernet</string>
    <string name="eth_conf_perf_title">Ethernet configuration</string>
    <string name="eth_conf_summary">Configure Ethernet devices</string>
    <string name="eth_mask">Netmask</string>
    <string name="eth_toggle_summary_off">Turn off Ethernet</string>
    <string name="eth_toggle_summary_on">Turn on Ethernet</string>
    <string name="eth_settings_error">Failed to set: Please enter the valid characters 0~255</string>
```
拷贝Settings/src/ethernet到Settings/src文件夹下

修改Settings/AndroidManifest.xml
在
```java
        <!-- Wireless Controls -->

        <activity android:name="Settings$WirelessSettingsActivity"
                android:taskAffinity="com.android.settings"
                android:label="@string/wireless_networks_settings_title"
                android:parentActivityName="Settings">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <action android:name="android.settings.WIRELESS_SETTINGS" />
                <action android:name="android.settings.AIRPLANE_MODE_SETTINGS" />
                <action android:name="android.settings.NFC_SETTINGS" />
                <category android:name="android.intent.category.DEFAULT" />
                <category android:name="android.intent.category.VOICE_LAUNCH" />
            </intent-filter>
            <meta-data android:name="com.android.settings.FRAGMENT_CLASS"
                android:value="com.android.settings.WirelessSettings" />
            <meta-data android:name="com.android.settings.TOP_LEVEL_HEADER_ID"
                android:resource="@id/wireless_settings" />
        </activity>
```
下增加如下代码
```java
         <!-- Ethernet controls add by hclydao-->

        <activity android:name="Settings$EthernetSettingsActivity"
                android:label="@string/eth_setting">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <action android:name="android.settings.ETHERNET_SETTINGS" />
                <category android:name="android.intent.category.DEFAULT" />
                <category android:name="android.intent.category.VOICE_LAUNCH" />
                <category android:name="com.android.settings.SHORTCUT" />
            </intent-filter>
            <meta-data android:name="com.android.settings.FRAGMENT_CLASS"
                android:value="com.android.settings.ethernet.EthernetSettings" />
            <meta-data android:name="com.android.settings.TOP_LEVEL_HEADER_ID"
                android:resource="@id/ethernet_settings" />
        </activity>
```

修改Settings/src/Utils.java
在
```java
    public static String getWifiIpAddresses(Context context) {
        ConnectivityManager cm = (ConnectivityManager)
                context.getSystemService(Context.CONNECTIVITY_SERVICE);
        LinkProperties prop = cm.getLinkProperties(ConnectivityManager.TYPE_WIFI);
        return formatIpAddresses(prop);
    }
```
下增加如下代码
```java
    public static String getEtherProperties(Context context) {
        ConnectivityManager cm = (ConnectivityManager)
                context.getSystemService(Context.CONNECTIVITY_SERVICE);
        LinkProperties prop = cm.getLinkProperties(ConnectivityManager.TYPE_ETHERNET);
        return prop.toString();
    }
```
修改Settings/src/Settings.java
加上
```java
import com.android.settings.ethernet.EthernetSettings;
```
在
```java
            R.id.wifi_settings,
```
下加上
```java
			R.id.ethernet_settings,
```
在
```java
        WifiSettings.class.getName(),
```
下加上
```java
		EthernetSettings.class.getName(),
```


拷贝systemui下所有.png文件到frameworks/base/packages/SystemUI/res/drawable下
修改frameworks/base/packages/SystemUI/res/values/strings.xml
在
```java
    <string name="accessibility_no_sim">No SIM.</string>
```
下增加如下
```java
    <!-- Content description of the Ethernet connected icon for accessibility (not shown on the screen). [CHAR LIMIT=NONE] -->
    <string name="accessibility_ethernet_connected">Ethernet connected.</string>
    <string name="accessibility_ethernet_disconnected">Ethernet disconnected.</string>
    <string name="accessibility_ethernet_connecting">Ethernet connecting.</string>
```
修改frameworks/base/packages/SystemUI/res/layout/signal_cluster_view.xml
在
```java
    <View
        android:layout_height="6dp"
        android:layout_width="6dp"
        android:visibility="gone"
        android:id="@+id/spacer"
        />
    <!--<FrameLayout
        android:id="@+id/wimax_combo"
        android:layout_height="wrap_content"
        android:layout_width="wrap_content"
        android:layout_marginEnd="-6dp"
        >
```
上增加
```java
		<FrameLayout
		    android:id="@+id/ethernet_combo"
		    android:layout_height="wrap_content"
		    android:layout_width="wrap_content"
		    android:layout_marginRight="-6dp"
		    >
		    <ImageView
		        android:id="@+id/ethernet_state"
		        android:layout_height="wrap_content"
		        android:layout_width="wrap_content"
		        android:layout_alignParentRight="true"
		        android:layout_centerVertical="true"
		        android:scaleType="center"
		        />
		</FrameLayout>
```
修改frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/SignalClusterView.java
在
```java
	private int mWifiStrengthId = 0; 
```
下增加
```java
    private boolean mEthernetVisible = false;
    private int mEthernetStateId = 0;
```
在
```java
private String mWifiDescription, mMobileDescription, mMobileTypeDescription,mEthernetDescription;
```
后增加,mEthernetDescription如上
下面的修改如下
```java
    ViewGroup mWifiGroup, mMobileGroup,mEthernetGroup;
    ImageView mWifi, mMobile, mWifiActivity, mMobileActivity, mMobileType, mAirplane,mEthernet;
```

在
```java
    @Override
    public void setIsAirplaneMode(boolean is, int airplaneIconId) {
        mIsAirplaneMode = is;
        mAirplaneIconId = airplaneIconId;

        apply();
    }
```
下增加
```java
    @Override
    public void setEthernetIndicators(boolean visible, int stateIcon, int activityIcon,
            String contentDescription) {
        mEthernetVisible = visible;
        mEthernetStateId = stateIcon;
        //mEthernetActivityId = activityIcon;
        mEthernetDescription = contentDescription;

        apply();
    }
```
在
```java
mAirplane       = (ImageView) findViewById(R.id.airplane);
```
下增加
```java
        mEthernetGroup  = (ViewGroup) findViewById(R.id.ethernet_combo);
        mEthernet       = (ImageView) findViewById(R.id.ethernet_state);
```
在mAirplane       = null;
下增加
```java
        mEthernetGroup  = null;
        mEthernet	    = null;
```
在
```java
        if (mIsAirplaneMode) {
            mAirplane.setImageResource(mAirplaneIconId);
            mAirplane.setVisibility(View.VISIBLE);
        } else {
            mAirplane.setVisibility(View.GONE);
        }
```
下增加
```java
        if (mEthernetVisible) {
            mEthernetGroup.setVisibility(View.VISIBLE);
            mEthernet.setImageResource(mEthernetStateId);
            //mEthernetActivity.setImageResource(mEthernetActivityId);
            mEthernetGroup.setContentDescription(mEthernetDescription);
        } else {
            mEthernetGroup.setVisibility(View.GONE);
        }
```
下面修改如下
```java
if (mMobileVisible && mWifiVisible && mIsAirplaneMode && mEthernetVisible) {
```
修改frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/policy/NetworkController.java
增加
```java
import android.net.ethernet.EthernetManager;
import android.net.ethernet.EthernetStateTracker;
import android.util.Slog;
import com.android.systemui.R;
```
在
```java
    String mContentDescriptionWimax;
```
下增加
```java
	String mContentDescriptionEthernet;//add by hclydao
```
在
```java
    //wimax
    private boolean mWimaxSupported = false;
    private boolean mIsWimaxEnabled = false;
    private boolean mWimaxConnected = false;
    private boolean mWimaxIdle = false;
    private int mWimaxIconId = 0;
    private int mWimaxSignal = 0;
    private int mWimaxState = 0;
    private int mWimaxExtraState = 0;
```
下增加
```java
    // Ethernet
    boolean mShowEthIcon, mEthernetWaitingDHCP;
    boolean mEthernetPhyConnect=false ;
    int mEthernetIconId = 0;
```
在
```java
    String mLastCombinedLabel = "";
```
下增加
```java
    int mLastEthernetIconId = -1;
```
在
```java
        void setIsAirplaneMode(boolean is, int airplaneIcon);
```
下增加
```java
        void setEthernetIndicators(boolean visible, int stateIcon, int activityIcon,
                String contentDescription);
```
在
```java
        filter.addAction(Intent.ACTION_AIRPLANE_MODE_CHANGED);
```
下增加
```java
		filter.addAction(EthernetManager.ETH_STATE_CHANGED_ACTION);
```
在
```java
cluster.setIsAirplaneMode(mAirplaneMode, mAirplaneIconId);
```
上增加如
```java
        cluster.setEthernetIndicators(
                mShowEthIcon,
                mEthernetIconId,
                -1,
                mContentDescriptionEthernet);  
```				
在
```java
	else if (action.equals(WimaxManagerConstants.NET_4G_STATE_CHANGED_ACTION) ||
		            action.equals(WimaxManagerConstants.SIGNAL_LEVEL_CHANGED_ACTION) ||
		            action.equals(WimaxManagerConstants.WIMAX_NETWORK_STATE_CHANGED_ACTION)) {
		        updateWimaxState(intent);
		        refreshViews();
		    } 
```
下增加
```java
	else if (action.equals(EthernetManager.ETH_STATE_CHANGED_ACTION)) {
            updateEth(intent);
            refreshViews();
        }
```
在
	updateWimaxIcons函数下增加
```java
  // ===== Ethernet ===================================================================
    private final void updateEth(Intent intent) {
        final int event = intent.getIntExtra(EthernetManager.EXTRA_ETH_STATE, EthernetStateTracker.EVENT_HW_DISCONNECTED);
        Slog.d(TAG, "updateEth event=" + event);
        switch (event) {
            case EthernetStateTracker.EVENT_HW_CONNECTED:
                if (mEthernetWaitingDHCP)
                    return;
                // else fallthrough
            case EthernetStateTracker.EVENT_INTERFACE_CONFIGURATION_SUCCEEDED: {
                    mEthernetWaitingDHCP = false;
                EthernetManager ethManager = (EthernetManager) mContext.getSystemService(mContext.ETH_SERVICE);
                if (ethManager.isEthDeviceAdded()) {
                    mShowEthIcon = true;
                    mEthernetIconId =R.drawable.ethernet_connected ; //  sEthImages[0]; 
                    mContentDescriptionEthernet = mContext.getString(R.string.accessibility_ethernet_connected);
                }
                return;
            }
            case EthernetStateTracker.EVENT_INTERFACE_CONFIGURATION_FAILED:
                mEthernetWaitingDHCP = false;
				//if(!mEthernetPhyConnect)
                 // return ;
                mShowEthIcon = true;
                mEthernetIconId = R.drawable.ethernet_disconnected; // sEthImages[1]; 
                mContentDescriptionEthernet = mContext.getString(R.string.accessibility_ethernet_disconnected);
                return;
            case EthernetStateTracker.EVENT_DHCP_START:
                mEthernetWaitingDHCP = true ;
                return;
            case EthernetStateTracker.EVENT_HW_PHYCONNECTED:
                mEthernetPhyConnect = true ;
                mShowEthIcon = true;
                mEthernetIconId =R.drawable.ethernet_connecting ; // sEthImages[2]; // 2
                mContentDescriptionEthernet = mContext.getString(R.string.accessibility_ethernet_connecting);
                return;
            case EthernetStateTracker.EVENT_HW_PHYDISCONNECTED:
                mEthernetPhyConnect = false ;
                mEthernetWaitingDHCP = false;
                mShowEthIcon = false;
                mEthernetIconId = -1;
                mContentDescriptionEthernet = null;
                return;
            case EthernetStateTracker.EVENT_HW_DISCONNECTED:
                mEthernetPhyConnect = false ;
                mEthernetWaitingDHCP = false;
                mShowEthIcon = false;
                mEthernetIconId = -1;
                mContentDescriptionEthernet = null;
                return ;
            case EthernetStateTracker.EVENT_HW_CHANGED:
                return;            
            
            default:
                if (mEthernetWaitingDHCP)
                    return;
                mShowEthIcon = false;
                mEthernetIconId = -1;
                mContentDescriptionEthernet = null;
                return;
        }
    }
```
在
```java
        if (mBluetoothTethered) {
            combinedLabel = mContext.getString(R.string.bluetooth_tethered);
            combinedSignalIconId = mBluetoothTetherIconId;
            mContentDescriptionCombinedSignal = mContext.getString(
                    R.string.accessibility_bluetooth_tether);
        }
```
下增加
```java
        if (mShowEthIcon) {
            wifiLabel = mContentDescriptionEthernet;
            combinedSignalIconId = mEthernetIconId;
            mContentDescriptionCombinedSignal = mContentDescriptionEthernet;
        }
```
在这个下面增加
```java
        final boolean ethernetConnected = (mConnectedNetworkType == ConnectivityManager.TYPE_ETHERNET);
        if (ethernetConnected) {
            // TODO: icons and strings for Ethernet connectivity
            combinedLabel = mConnectedNetworkTypeName;
        }
```
注释掉之前的　
```java
/*
        final boolean ethernetConnected = (mConnectedNetworkType == ConnectivityManager.TYPE_ETHERNET);
        if (ethernetConnected) {
            combinedLabel = context.getString(R.string.ethernet_label);
        }
*///modify by hclydao
```
在
```java
&& !ethernetConnected
```
后增加
```java
&& !mShowEthIcon
```
在
```java
         || mLastWifiIconId                 != mWifiIconId
```
下增加
```java
		 || mLastEthernetIconId             != mEthernetIconId
```
在
```java
        // the wimax icon on phones
        if (mLastWimaxIconId != mWimaxIconId) {
            mLastWimaxIconId = mWimaxIconId;
        }
```
下增加
```java
        // ethernet icon on phones
        if (mLastEthernetIconId != mEthernetIconId) {
            mLastEthernetIconId = mEthernetIconId;
            // Phone UI not supported yet.
        }
```
先执行下make update-api

20151214修改 
问题 设置静态ＩＰ无法启动问题
解决:
	修改frameworks/base/services/java/com/android/server/ConnectivityService.java部分代码为：
```java
	if(mNetConfigs[targetNetworkType].radio == ConnectivityManager.TYPE_ETHERNET) { //add by hclydao
        EthernetService ethernet = new EthernetService(context, (EthernetStateTracker)mNetTrackers[targetNetworkType]);
        ServiceManager.addService(Context.ETH_SERVICE, ethernet);
       // mNetTrackers[targetNetworkType].startMonitoring(context, mTrackerHandler);
	}
```
	以及
```java
	if(mClat != null) //add by hclydao
		if (mClat.requiresClat(netType, tracker)) {

			// If the connection was previously using clat, but is not using it now, stop the clat
			// daemon. Normally, this happens automatically when the connection disconnects, but if
			// the disconnect is not reported, or if the connection's LinkProperties changed for
			// some other reason (e.g., handoff changes the IP addresses on the link), it would
			// still be running. If it's not running, then stopping it is a no-op.
			if (Nat464Xlat.isRunningClat(curLp) && !Nat464Xlat.isRunningClat(newLp)) {
			    mClat.stopClat();
			}
			// If the link requires clat to be running, then start the daemon now.
			if (mNetTrackers[netType].getNetworkInfo().isConnected()) {
			    mClat.startClat(tracker);
			} else {
			    mClat.stopClat();
			}
		}
```

![打赏](https://github.com/hcly/pics/blob/master/zhifu.png)
