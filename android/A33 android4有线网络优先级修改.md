# A33 android4有线网络优先级高于wifi修改
修改文件evice/softwinner/icool/overlay/frameworks/base/core/res/res/values/config.xml
中将
```java
<item>"ethernet,9,9,2,-1,true"</item>
改为
<item>"ethernet,9,9,0,-1,true"</item>
```
文件frameworks/base/packages/SystemUI/src/com/android/systemui/statusbar/policy/NetworkController.java
函数refreshSignalCluster中
```java
                   cluster.setWifiIndicators(
                           // only show wifi in the cluster if connected or if wifi-only
                           mWifiEnabled/* && (mWifiConnected || !mHasMobileDataFeature)*/,
                           mWifiIconId,
                           mContentDescriptionWifi);
修改为
               if(mEthernetConnected) //add by leijie
                   cluster.setWifiIndicators(
                           // only show wifi in the cluster if connected or if wifi-only
                           false/* && (mWifiConnected || !mHasMobileDataFeature)*/,
                           mWifiIconId,
                           mContentDescriptionWifi);
               else
                   cluster.setWifiIndicators(
                           // only show wifi in the cluster if connected or if wifi-only
                           mWifiEnabled/* && (mWifiConnected || !mHasMobileDataFeature)*/,
                           mWifiIconId,
                           mContentDescriptionWifi);
有线网络连接成功后隐藏wifi图标
```
文件frameworks/base/services/java/com/android/server/ConnectivityService.java
函数getPersistedNetworkPreference中
```java
在
final int networkPrefSetting = Settings.Global
                 .getInt(cr, Settings.Global.NETWORK_PREFERENCE, -1);
前增加
Settings.Global.putInt(cr, Settings.Global.NETWORK_PREFERENCE, ConnectivityManager.TYPE_ETHERNET);
```
