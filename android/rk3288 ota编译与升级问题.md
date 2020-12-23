# rk3288 adndroid7 ota编译与升级问题记录
android7 make otapackage编译
首先修改
device/rockchip/rk3288/BoardConfig.mk
```java
BOARD_USE_SPARSE_SYSTEM_IMAGE := true
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := false
```
后出现编译报错
```java
Traceback (most recent call last):
  File "./build/tools/releasetools/ota_from_target_files", line 2206, in <module>
    main(sys.argv[1:])
  File "./build/tools/releasetools/ota_from_target_files", line 2162, in main
    WriteFullOTAPackage(input_zip, output_zip)
  File "./build/tools/releasetools/ota_from_target_files", line 728, in WriteFullOTAPackage
    vendor_diff = common.BlockDifference("vendor", vendor_tgt)
  File "/home/zhaoni/workspace/rk3288/king_rp_3288_7.1/build/tools/releasetools/common.py", line 1397, in __init__
    _, self.device = GetTypeAndDevice("/" + partition, OPTIONS.info_dict)
  File "/home/zhaoni/workspace/rk3288/king_rp_3288_7.1/build/tools/releasetools/common.py", line 1618, in GetTypeAndDevice
    return (PARTITION_TYPES[fstab[mount_point].fs_type],
KeyError: '/vendor'
ninja: build stopped: subcommand failed.
build/core/ninja.mk:148: recipe for target 'ninja_wrapper' failed
make: *** [ninja_wrapper] Error 1
```
KeyError: '/vendor'
修改
device/rockchip/common/recovery.fstab
增加一个vendor分区
```java
/dev/block/rknand_vendor                           /vendor               ext4            defaults                  defaults
```
最后能编译生成update.zip文件，但用这个update.zip升级后系统无法启动，目前还不知道什么原因
最后采用了update.img进行升级系统
修改
device/rockchip/rk3288/BoardConfig.mk
```java
BOARD_USE_SPARSE_SYSTEM_IMAGE := false
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true
```
这样修改后生成的镜像，可以制作TF卡更新卡，同时也支持recovery更新
修改device/rockchip/rk3288/parameter.txt
```java
MACHINE_MODEL:ease3288
这个MACHINE_MODEL需要和device/rockchip/rk3288/rk3288.mk中PRODUCT_MODEL一样不然无法更新
```
修改
bootable/recovery/recovery.cpp
```java
在1906行左右增加strcpy(updatepath, update_rkimage);
         else
             bAutoUpdateComplete=true;
     	strcpy(updatepath, update_rkimage);//add by leijie
     }else if (factory_mode != NULL){

```
如果不增加这个使用update.img更新后，提示删除的时候无法正常删除update.img文件
修改完成后编译生成update.img文件，拷贝到TF或者U盘根目录，或者内部存储/sdcard/目录下系统就会提示更新
如果是拷贝到内部存储/sdcard/目录下需要重启才会提示更新






