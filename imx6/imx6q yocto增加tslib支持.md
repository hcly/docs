# imx6q yocto增加tslib支持


## yocto修改
修改sources/meta-qt5/recipes-qt/qt5/qtbase_git.bb
将
```
PACKAGECONFIG_DEFAULT ?= "dbus udev evdev widgets tools libs freetype tests sql-sqlite
```
修改为
```
PACKAGECONFIG_DEFAULT ?= "dbus udev evdev tslib widgets tools libs freetype tests sql-sqlite
```
增加tslib支持,这时只有tslib库没有校准程序，还需要增加tslib-calibrate
修改yocto根目录下编译脚本
```
#!/bin/bash
#imx yocto project L4.14.98_2.0.0_ga
DISTRO=fsl-imx-fb
BUILD_DIR=build-fb
#fsl-imx-xwayland
MACHINE=imx6qsabresd
TOP=`pwd`
BUILD_ALL=true
BUILD_UBOOT=false
BUILD_KERNEL=false
BUILD_MODULE=false
BUILD_SYSTEM=false
BUILD_TOOLCHAIN=false
CLEAN_BUILD=false
BUILD_SDK=false
UBOOT_MOD=u-boot-with-spl.bin
KERNEL_MOD=uImage

CPU_NUM=$(cat /proc/cpuinfo |grep processor|wc -l)
CPU_NUM=$((CPU_NUM+1))
IMAGE_DIR=build-fb/tmp/deploy/images/imx6qsabresd


function parse_args()
{
    TEMP=`getopt -o "b:t:c" -- "$@"`
    eval set -- "$TEMP"

    while true; do
        case "$1" in
			-b ) DISTRO=fsl-imx-$2; BUILD_DIR=build-$2; IMAGE_DIR=build-$2/tmp/deploy/images/imx6qsabresd; shift 2 ;;
            -c ) CLEAN_BUILD=true; BUILD_ALL=false; shift 1 ;;
            -t ) case "$2" in
                    u-boot  ) BUILD_ALL=false; BUILD_UBOOT=true ;;
                    kernel  ) BUILD_ALL=false; BUILD_KERNEL=true ;;
                    module  ) BUILD_ALL=false; BUILD_MODULE=true ;;
					system 	) BUILD_ALL=false; BUILD_SYSTEM=true ;;
					clean 	) BUILD_ALL=false; CLEAN_BUILD=true ;;
					sdk 	) BUILD_ALL=false; BUILD_SDK=true ;;
					toolchain ) BUILD_ALL=false; BUILD_TOOLCHAIN=true ;;
                    none    ) BUILD_ALL=false ;;
                 esac
				 shift 2 ;;
            -- ) break ;;
            *  ) echo "invalid option $1"; usage; exit 1 ;;
        esac
    done
}

function build_uboot()
{
    if [ ${BUILD_UBOOT} == "true" ]; then
		bitbake -c compile -f -v u-boot-imx
		bitbake -c deploy -f -v u-boot-imx
	fi
}

function build_kernel()
{
    if [ ${BUILD_KERNEL} == "true" ]; then
		bitbake -c menuconfig -v linux-imx
		#bitbake -c compile -f -v linux-imx
		#bitbake -c compile_kernelmodules -f -v linux-imx
		#bitbake -c deploy -f -v linux-imx
	fi
}

function build_system()
{
    if [ ${BUILD_SYSTEM} == "true" ]; then
		#bitbake fsl-image-gui
		#bitbake core-image-minimal
		bitbake fsl-image-mfgtool-initramfs
	fi
}

function build_gui()
{
    if [ ${BUILD_ALL} == "true" ]; then
		#bitbake fsl-image-gui
		bitbake fsl-image-validation-imx
	fi
}

function build_qt5()
{
    if [ ${BUILD_ALL} == "true" ]; then
		#bitbake fsl-image-gui
		#bitbake core-image-minimal
		bitbake fsl-image-qt5-validation-imx
	fi
	#cp ./$IMAGE_DIR/core-image-minimal-imx6qsabresd.tar.bz2 ./
	#cp ./$IMAGE_DIR/fsl-image-qt5-validation-imx-imx6qsabresd.tar.bz2 ./
	#chmod 777 ./core-image-minimal-imx6qsabresd.tar.bz2
	#chmod 777 ./fsl-image-qt5-validation-imx-imx6qsabresd.tar.bz2
	#echo "image path $IMAGE_DIR"
}

function build_toolchain()
{
    if [ ${BUILD_TOOLCHAIN} == "true" ] ; then
		#gcc toolchain
		bitbake meta-toolchain
		#qt5 toolchain
		bitbake meta-toolchain-qt5
	fi
}

function build_sdk()
{
    if [ ${BUILD_SDK} == "true" ]; then
		bitbake fsl-image-qt5-validation-imx -c populate_sdk
		#bitbake meta-toolchain
		#bitbake meta-toolchain-qt5
	fi
}

function clean_build()
{
    if [ ${CLEAN_BUILD} == "true" ]; then
		echo "start clean"
		bitbake -c cleanall core-image-minimal
		bitbake -c cleanall fsl-image-qt5-validation-imx
		#bitbake -c cleanall meta-toolchain
		#bitbake -c cleanall meta-toolchain-qt5
	fi
}

parse_args $@
echo "DISTRO:$DISTRO BUILD_DIR:$BUILD_DIR "
CONF_DIR=$BUILD_DIR
source ./fsl-setup-release.sh -b $BUILD_DIR
echo "conf:$TOP/$CONF_DIR/conf/local.conf"
echo "IMAGE_INSTALL_append= \"qtvirtualkeyboard \"" >> $TOP/$CONF_DIR/conf/local.conf
echo "IMAGE_INSTALL_append= \"ppp \"" >> $TOP/$CONF_DIR/conf/local.conf
echo "IMAGE_INSTALL_append= \"tslib-calibrate \"" >> $TOP/$CONF_DIR/conf/local.conf
# Remove connman
#echo "IMAGE_INSTALL_remove = \"connman\"" >> $TOP/$CONF_DIR/conf/local.conf
#echo "IMAGE_INSTALL_remove = \"connman-client\"" >> $TOP/$CONF_DIR/conf/local.conf
#echo "IMAGE_INSTALL_remove = \"connman-gnome\"" >> $TOP/$CONF_DIR/conf/local.conf
# Add NetworkManager
#echo "IMAGE_INSTALL_append = \"networkmanager\"" >> $TOP/$CONF_DIR/conf/local.conf
#echo "IMAGE_INSTALL_append = \"modemmanager\"" >> $TOP/$CONF_DIR/conf/local.conf

clean_build
build_uboot
build_kernel
build_system
build_sdk
build_qt5
#build_gui
```
增加了
```
echo "IMAGE_INSTALL_append= \"tslib-calibrate \"" >> $TOP/$CONF_DIR/conf/local.conf
```
执行./build.sh -b fb
使用时增加环境变量
```
export QT_QPA_EGLFS_TSLIB=1
export TSLIB_TSDEVICE=/dev/input/touchscreen0
```
这里QT_QPA_EGLFS_TSLIB环境变量一定要在TSLIB_TSDEVICE之前,要不然ts_calibrate点击会没反应
在增加自动校准，功能
```
if [ -c ${TSLIB_TSDEVICE} ]; then
	if [ ! -f /etc/pointercal ] ; then
		ts_calibrate
	fi
fi
```
最后启动默认demo
/usr/share/qt5everywheredemo-1.0/QtDemo
触摸正常

## 参考链接
<https://blog.csdn.net/swikon/article/details/78788419?utm_source=blogxgwz9>

<https://www.toradex.com/community/questions/12391/ts-calibrate-in-latest-imx6-apalis.html>
