# android Presentation双屏异显记录
Presentation的例子网上有很多，记录一个调试中出现的问题
按照大部分的例子调试，退出软件后，在启动软件，异显就无法显示了
主要修改两个地方

1.增加两个权限
```java
    <uses-permission android:name= "android.permission.SYSTEM_ALERT_WINDOW" />
    <uses-permission android:name= "android.permission.SYSTEM_OVERLAY_WINDOW"  />
```
2.修改Presentation继承类在onCreate中增加
```java
getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
```
这样不管应用退出多少次，异显都能正常显示
