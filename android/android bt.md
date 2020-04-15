# android bt
## bt无法打开卡在设置波特率然后不断重启

```
01-01 08:01:14.190  1370  1398 I bt_hci_h4: hal_open
01-01 08:01:14.190  1370  1398 I bt_userial_vendor: userial vendor open: opening /dev/ttyS0
01-01 08:01:14.190  1370  1398 I bt_userial_vendor: device fd = 74 open
01-01 08:01:14.191  1370  1399 W bt_osi_thread: run_thread: thread id 1399, thread name hci_single_chann started
01-01 08:01:14.196  1370  1398 I bt_hwcfg: bt vendor lib: set UART baud 1500000
01-01 08:01:16.915   219   261 D alsa_route: route_set_controls() set route 24
01-01 08:01:16.916   219   261 D AudioHardwareTiny: close device
```
出现这种情况可以降低波特率，或者将设置波特率的延时加长,以rk3288 android7.1　ap6212为例
修改hardware/broadcom/libbt/include/vnd_rk30sdk.txt文件中的
```java
UART_TARGET_BAUD_RATE = 1500000
USERIAL_VENDOR_SET_BAUD_DELAY_US = 200000
```
