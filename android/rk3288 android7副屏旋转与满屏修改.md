# rk3288 android7副屏旋转与满屏修改
主屏使用的是HDMI，副屏使用的lvds
主屏是HMDI转lvds 由于转接模块不支持1280x800 所以设置了hdmi输入源为1280x720 为了保证主屏正常显示 修改了framebuffer为1280x800
就是由于修改了framebuffer导致副屏显示旋转异常,一定不要设置persist.sys.framebuffer.main这个参数
```java
persist.sys.framebuffer.main=1280x800
```
下面的patch能解决副屏旋转后显示异常及同显时不满屏的问题
修改patch如下
```java
From fff08b318e0c9ffbe81b14657b8e162455a874bb Mon Sep 17 00:00:00 2001
From: leijie <leijie@icoolarm.com>
Date: Wed, 2 Jun 2021 16:22:46 +0800
Subject: [PATCH] =?UTF-8?q?=E5=89=AF=E5=B1=8F=E6=97=8B=E8=BD=AC,=E6=BB=A1?=
 =?UTF-8?q?=E5=B1=8F=E6=98=BE=E7=A4=BA=E4=BF=AE=E6=94=B9=EF=BC=8C=E5=BC=82?=
 =?UTF-8?q?=E6=98=BE=E6=B5=8B=E8=AF=95=E6=AD=A3=E5=B8=B8?=
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit

---
 device/rockchip/rk3288/system.prop                 | 13 ++++++-----
 .../server/display/LocalDisplayAdapter.java        | 20 ++++++++++++++++-
 .../com/android/server/display/LogicalDisplay.java | 26 +++++++++++++++++++++-
 .../services/surfaceflinger/DisplayDevice.cpp      |  3 ++-
 4 files changed, 53 insertions(+), 9 deletions(-)

diff --git a/device/rockchip/rk3288/system.prop b/device/rockchip/rk3288/system.prop
index 358f0fd..f34c991 100755
--- a/device/rockchip/rk3288/system.prop
+++ b/device/rockchip/rk3288/system.prop
@@ -48,9 +48,10 @@ ro.rk.displayd.enable=false

 sys.hwc.device.primary=HDMI-A
 sys.hwc.device.extend=LVDS
-persist.sys.framebuffer.main=1280x800
-persist.sys.resolution.main=1920x1080
-persist.sys.resolution.aux=480x1280
-ro.same.orientation=false
-#ro.orientation.einit=0
-ro.rotation.external=false
+#persist.sys.framebuffer.main=1280x800
+#persist.sys.resolution.main=1280x720
+#persist.sys.resolution.aux=480x1280
+#ro.same.orientation=false
+#ro.rotation.external=false
+ro.orientation.einit=270
+persist.sys.rotation.efull=true
diff --git a/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java b/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
index 68be9ce..be69c7e 100755
--- a/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
+++ b/frameworks/base/services/core/java/com/android/server/display/LocalDisplayAdapter.java
@@ -422,8 +422,26 @@ final class LocalDisplayAdapter extends DisplayAdapter {

                     // For demonstration purposes, allow rotation of the external display.
                     // In the future we might allow the user to configure this directly.
-                    if ("portrait".equals(SystemProperties.get("persist.demo.hdmirotation"))) {
+                    /*if ("portrait".equals(SystemProperties.get("persist.demo.hdmirotation"))) {
                         mInfo.rotation = Surface.ROTATION_270;
+                    }*/
+                    String rotation = SystemProperties.get("ro.orientation.einit","0");
+                    switch(Integer.valueOf(rotation)) {
+						case 0:
+							mInfo.rotation = Surface.ROTATION_0;
+							break;
+						case 90:
+							mInfo.rotation = Surface.ROTATION_90;
+							break;
+						case 180:
+							mInfo.rotation = Surface.ROTATION_180;
+							break;
+						case 270:
+							mInfo.rotation = Surface.ROTATION_270;
+							break;
+						default:
+							mInfo.rotation = Surface.ROTATION_0;
+							break;
                     }

                     // For demonstration purposes, allow rotation of the external display
diff --git a/frameworks/base/services/core/java/com/android/server/display/LogicalDisplay.java b/frameworks/base/services/core/java/com/android/server/display/LogicalDisplay.java
index 287a25a..acc4c9f 100755
--- a/frameworks/base/services/core/java/com/android/server/display/LogicalDisplay.java
+++ b/frameworks/base/services/core/java/com/android/server/display/LogicalDisplay.java
@@ -26,7 +26,7 @@ import java.util.Arrays;
 import java.util.List;

 import libcore.util.Objects;
-
+import android.os.SystemProperties;

@@ -137,6 +137,22 @@ final class LogicalDisplay {
                 mInfo.physicalXDpi = mOverrideDisplayInfo.physicalXDpi;
                 mInfo.physicalYDpi = mOverrideDisplayInfo.physicalYDpi;
             }
+            if(mDisplayId!=Display.DEFAULT_DISPLAY){
+                 String rotation = SystemProperties.get("ro.orientation.einit","0");
+                 int rot = Integer.valueOf(rotation)/90;
+                if(rot%2!=0) {
+                    mInfo.appWidth = mPrimaryDisplayDeviceInfo.height;
+                    mInfo.appHeight = mPrimaryDisplayDeviceInfo.width;
+                    mInfo.logicalWidth = mPrimaryDisplayDeviceInfo.height;
+                    mInfo.logicalHeight=mPrimaryDisplayDeviceInfo.width;
+                }else{
+                    mInfo.appWidth = mPrimaryDisplayDeviceInfo.width;
+                    mInfo.appHeight = mPrimaryDisplayDeviceInfo.height;
+                    mInfo.logicalWidth = mPrimaryDisplayDeviceInfo.width;
+                    mInfo.logicalHeight=mPrimaryDisplayDeviceInfo.height;
+                }
+            }
+
         }
         return mInfo;
     }
@@ -349,6 +365,14 @@ final class LogicalDisplay {
         mTempDisplayRect.right += mDisplayOffsetX;
         mTempDisplayRect.top += mDisplayOffsetY;
         mTempDisplayRect.bottom += mDisplayOffsetY;
+        if(SystemProperties.getBoolean("persist.sys.rotation.efull", false)) {
+		    if(device.getDisplayDeviceInfoLocked().type!=Display.TYPE_BUILT_IN){
+		         mTempDisplayRect.left=0;
+		         mTempDisplayRect.right=physWidth;
+		         mTempDisplayRect.top=0;
+		         mTempDisplayRect.bottom=physHeight;
+		    }
+        }
         device.setProjectionInTransactionLocked(orientation, mTempLayerStackRect, mTempDisplayRect);
     }

diff --git a/frameworks/native/services/surfaceflinger/DisplayDevice.cpp b/frameworks/native/services/surfaceflinger/DisplayDevice.cpp
index 0c63381..68f93ce 100755
--- a/frameworks/native/services/surfaceflinger/DisplayDevice.cpp
+++ b/frameworks/native/services/surfaceflinger/DisplayDevice.cpp
@@ -538,6 +538,7 @@ void DisplayDevice::setProjection(int orientation,
 #endif

 #if !RK_VR & RK_HW_ROTATION
+#if 0
     bool isHdmiScreen = mType == DisplayDevice::DISPLAY_EXTERNAL;
     if (isHdmiScreen) {
         int eInitOrientation = 0;
@@ -599,7 +600,7 @@ void DisplayDevice::setProjection(int orientation,
         }
         ALOGV("update frame [%d,%d]",frame.getWidth(),frame.getHeight());
     }
-
+#endif
     if (mType == DisplayDevice::DISPLAY_PRIMARY) {
         mClientOrientation = orientation;
         orientation = (mHardwareOrientation + orientation) % 4;
--
2.7.4

```
