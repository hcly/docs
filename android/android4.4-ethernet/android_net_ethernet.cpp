/*
 * Copyright 2009, The Android-x86 Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Yi Sun <beyounn@gmail.com>
 */

#define LOG_TAG "ethernet"

#include "jni.h"
#include <inttypes.h>
#include <utils/misc.h>
#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <net/if_arp.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#define ETH_PKG_NAME "android/net/ethernet/EthernetNative"

namespace android {
    static struct fieldIds {
        jclass dhcpInfoClass;
        jmethodID constructorId;
        jfieldID ipaddress;
        jfieldID gateway;
        jfieldID netmask;
        jfieldID dns1;
        jfieldID dns2;
        jfieldID serverAddress;
        jfieldID leaseDuration;
    } dhcpInfoFieldIds;

    typedef struct _interface_info_t {
        unsigned int i;                            /* interface index        */
        char *name;                       /* name (eth0, eth1, ...) */
        struct _interface_info_t *next;
    } interface_info_t;

    interface_info_t *interfaces = NULL;
    interface_info_t *interfaces_old = NULL;
    int total_int = 0;
#define NL_SOCK_INV      -1
#define RET_STR_SZ       4096
#define NL_POLL_MSG_SZ   8*1024
#define SYSFS_PATH_MAX   256
    static const char SYSFS_CLASS_NET[]     = "/sys/class/net";
    static int nl_socket_msg = NL_SOCK_INV;
    static struct sockaddr_nl addr_msg;
    static int nl_socket_poll = NL_SOCK_INV;
    static struct sockaddr_nl addr_poll;
    static int getinterfacename(int index, char *name, size_t len);
    static void free_int_list();
    static void free_int_list_old();
    static void backup_int_list() ;
    static char * find_removed_int() ;
    static int netlink_init_interfaces_list(void);
    static interface_info_t *find_info_by_index(unsigned int index) {
        interface_info_t *info = interfaces;
        while( info) {
            if (info->i == index)
                return info;
            info = info->next;
        }
        return NULL;
    }

    static char flags_desc[512];
    
    static char * flag_desc_tbl[32] = {
            "UP",
            "BC",
            "DBG",
            "LOOPBACK",

            "PPP",
            "NT",
            "RUNNING",
            "NOARP",

            "PROMISC",
            "ALLMULTI",
            "MASTER",
            "SLAVE",

            "MC",
            "PORTSEL",
            "AUTOMEDIA",
            "DYNAMIC",

            "LINK_UP",
            "DORMANT",
            "UNDEFINED",
            "UNDEFINED",

            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",

            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",
            "UNDEFINED",

        };

    char* if_flags_desc(int flags, char *desc) {
        desc[0] = '\0';
        for (int i = 0; i< 18; i++) {
            if (flags & (1<<i)) {
                strcat(desc, " ");
                strcat(desc, flag_desc_tbl[i]);
            }
        }

        return desc;
    }
        
    static jstring android_net_ethernet_waitForEvent(JNIEnv *env,
                                                     jobject clazz)
    {
        char *buff;
        struct nlmsghdr *nh;
        struct ifinfomsg *einfo;
        struct iovec iov;
        struct msghdr msg;
        char *result = NULL;
        char rbuf[4096];
        unsigned int left;
        interface_info_t *info;
        int len;
        int last_total_cnt;
        ALOGV("Poll events from ethernet devices");
        /*
         *wait on uevent netlink socket for the ethernet device
         */
        buff = (char *)malloc(NL_POLL_MSG_SZ);
        if (!buff) {
            ALOGE("Allocate poll buffer failed");
            goto error;
        }

        iov.iov_base = buff;
        iov.iov_len = NL_POLL_MSG_SZ;
        msg.msg_name = (void *)&addr_msg;
        msg.msg_namelen =  sizeof(addr_msg);
        msg.msg_iov =  &iov;
        msg.msg_iovlen =  1;
        msg.msg_control =  NULL;
        msg.msg_controllen =  0;
        msg.msg_flags =  0;
        last_total_cnt = total_int;

        if((len = recvmsg(nl_socket_poll, &msg, 0))>= 0) {
            ALOGV("recvmsg get data");
            backup_int_list();
            free_int_list();
            netlink_init_interfaces_list();
            result = rbuf;
            left = 4096;
            rbuf[0] = '\0';
            for (nh = (struct nlmsghdr *) buff; NLMSG_OK (nh, len);
                 nh = NLMSG_NEXT (nh, len))
            {

                if (nh->nlmsg_type == NLMSG_DONE){
                    ALOGE("Did not find useful eth interface information");
                    goto error;
                }

                if (nh->nlmsg_type == NLMSG_ERROR){

                    /* Do some error handling. */
                    ALOGE("Read device name failed");
                    goto error;
                }

                einfo = (struct ifinfomsg *)NLMSG_DATA(nh);

                if (nh->nlmsg_type == RTM_DELLINK ||
                    nh->nlmsg_type == RTM_NEWLINK ||
                    nh->nlmsg_type == RTM_DELADDR ||
                    nh->nlmsg_type == RTM_NEWADDR) {
                    int type = nh->nlmsg_type;
                    const char *desc = (type == RTM_DELLINK) ? "DELLINK" : 
                             ((type == RTM_NEWLINK) ? "NEWLINK": 
                             ((type == RTM_DELADDR) ? "DELADDR" : "NEWADDR"));
                    
                    if (einfo->ifi_flags & IFF_POINTOPOINT){
                        ALOGI("Event from PPP interface, ignore it");
                        continue;
                    }

                    if (einfo->ifi_flags & IFF_LOOPBACK){
                        ALOGI("Event from LOOP interface, ignore it");
                        continue;
                    }

                    ALOGI("event: %s(%d), flags=0X%X", desc, type, einfo->ifi_flags);
                    ALOGI("flags:%s", if_flags_desc(einfo->ifi_flags, flags_desc));

                    if (type == RTM_NEWLINK &&
                        (!(einfo->ifi_flags & IFF_LOWER_UP))) {
                            if (total_int > last_total_cnt) {
                                ALOGI("Had device add: last = %d; now = %d", last_total_cnt, total_int);
                            } else {
                                type = RTM_DELLINK;
                            }
                    }

                    if ((info = find_info_by_index
                          (((struct ifinfomsg*) NLMSG_DATA(nh))->ifi_index)) != NULL)
                        snprintf(result,left, "%s:%d:",info->name,type);
                    else if (type == RTM_DELLINK){
                        /*when link removed, we cann`t find it from interfaces, try
                        get it by compare interfaces and interfaces_old to find it*/
                        if(total_int>0){
                            snprintf(result,left, "%s:%d:",find_removed_int(),type);
                        }
                        else{
                            snprintf(result,left, "(pulledout):%d:",type);
                        }
                    }
                    left = left - strlen(result);
                    result =(char *)(result + strlen(result));
                }

            }
            rbuf[4096 - left] = '\0';

            if (rbuf[0]) ALOGI("poll state :%s",rbuf);
        }


    error:
        if(buff)
            free(buff);
        return env->NewStringUTF(4096 - left > 0 ? rbuf : NULL);
    }

    static int netlink_send_dump_request(int sock, int type, int family) {
        int ret;
        char buf[4096];
        struct sockaddr_nl snl;
        struct nlmsghdr *nlh;
        struct rtgenmsg *g;

        memset(&snl, 0, sizeof(snl));
        snl.nl_family = AF_NETLINK;

        memset(buf, 0, sizeof(buf));
        nlh = (struct nlmsghdr *)buf;
        g = (struct rtgenmsg *)(buf + sizeof(struct nlmsghdr));

        nlh->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtgenmsg));
        nlh->nlmsg_flags = NLM_F_REQUEST|NLM_F_DUMP;
        nlh->nlmsg_type = type;
        g->rtgen_family = family;

        ret = sendto(sock, buf, nlh->nlmsg_len, 0, (struct sockaddr *)&snl,
                     sizeof(snl));
        if (ret < 0) {
            perror("netlink_send_dump_request sendto");
            return -1;
        }

        return ret;
    }

    static void free_int_list() {
        interface_info_t *tmp = interfaces;
        while(tmp) {
            if (tmp->name) free(tmp->name);
            interfaces = tmp->next;
            free(tmp);
            tmp = interfaces;
            total_int--;
        }
        if (total_int != 0 )
        {
            ALOGE("Wrong interface count found");
            total_int = 0;
        }
    }

    /*free old interfaces list*/
    static void free_int_list_old() {
        interface_info_t *tmp = interfaces_old;
        while(tmp) {
            if (tmp->name) free(tmp->name);
            interfaces_old = tmp->next;
            free(tmp);
            tmp = interfaces_old;
        }
    }

    /*backup interfaces to interfaces_old*/
    static void backup_int_list() {
        free_int_list_old();
        interface_info_t *tmp = interfaces;
        interface_info_t *intfinfo;
        while(tmp) {
            intfinfo = (interface_info_t *)
            malloc(sizeof(struct _interface_info_t));
            if (intfinfo == NULL) {
                ALOGE("malloc in netlink_init_interfaces_table");
                return;
            }
            /* copy the interface name (eth0, eth1, ...) */
            intfinfo->name = strndup((char *) tmp->name, SYSFS_PATH_MAX);
            intfinfo->i = tmp->i;
            intfinfo->next = interfaces_old;
            interfaces_old = intfinfo;
            tmp = tmp->next;
        }
    }

    /* find removed interface by comparing interfaces and interfaces_old*/
    static char * find_removed_int() {
        interface_info_t *tmp1 = interfaces_old;
        interface_info_t *tmp2 = interfaces;
        while (tmp1) {
            int found = 0;
            while(tmp2) {
                if ( 0 == strcmp(tmp1->name, tmp2->name)){
                    found = 1;
                    break;
                }
                tmp2 = tmp2->next;
            }
            if (!found)
                return tmp1->name;
            tmp1 = tmp1->next;
        }
        return NULL;
    }

    static void add_int_to_list(interface_info_t *node) {
        /*
         *Todo: Lock here!!!!
         */
        node->next = interfaces;
        interfaces = node;
        total_int ++;
    }

    static int netlink_init_interfaces_list(void) {
        int ret = -1;
        DIR  *netdir;
        struct dirent *de;
        char path[SYSFS_PATH_MAX];
        interface_info_t *intfinfo;
        int index;
        FILE *ifidx;
        #define MAX_FGETS_LEN 4
        char idx[MAX_FGETS_LEN+1];

        if ((netdir = opendir(SYSFS_CLASS_NET)) != NULL) {
             while((de = readdir(netdir))!=NULL) {
                if ((!strcmp(de->d_name,".")) || (!strcmp(de->d_name,".."))
                    ||(!strcmp(de->d_name,"lo")) || (!strcmp(de->d_name,"wmaster0")) ||
                    (!strcmp(de->d_name,"pan0")) || (!strncmp(de->d_name,"ppp", strlen("ppp")))||
                    (!strncmp(de->d_name, "ifb", 3)) || (!strcmp(de->d_name,"mon.p2p0")) ||
                    (!strcmp(de->d_name,"p2p0")) || (!strcmp(de->d_name,"mon.wlan0")))
                    continue;
                /* ignore wireless interfaces */
                snprintf(path, SYSFS_PATH_MAX,"%s/%s/phy80211",SYSFS_CLASS_NET,de->d_name);
                if (access(path, F_OK)) {
                    snprintf(path, SYSFS_PATH_MAX,"%s/%s/wireless",SYSFS_CLASS_NET,de->d_name);
                    if (!access(path, F_OK))
                        continue;
                } else {
                    continue;
                }

                /* ignore tunnel interfaces */
                snprintf(path, SYSFS_PATH_MAX, "%s/%s/type", SYSFS_CLASS_NET, de->d_name);
                FILE *typefd;
                if ((typefd = fopen(path, "r")) != NULL) {
                    char typestr[MAX_FGETS_LEN + 1];
                    int type = 0;
                    memset(typestr, 0, MAX_FGETS_LEN + 1);
                    if (fgets(typestr, MAX_FGETS_LEN, typefd) != NULL) {
                        type = strtoimax(typestr, NULL, 10);
                    }
                    fclose(typefd);
                    if (type >= ARPHRD_TUNNEL && type < ARPHRD_IEEE802_TR)
                        continue;
                }

                snprintf(path, SYSFS_PATH_MAX,"%s/%s/ifindex",SYSFS_CLASS_NET,de->d_name);
                if ((ifidx = fopen(path,"r")) != NULL ) {
                    memset(idx,0,MAX_FGETS_LEN+1);
                    if(fgets(idx,MAX_FGETS_LEN,ifidx) != NULL) {
                        index = strtoimax(idx,NULL,10);
                    } else {
                        ALOGE("Can not read %s",path);
                        fclose(ifidx);
                        continue;
                    }
                    fclose(ifidx);
                } else {
                    ALOGE("Can not open %s for read",path);
                    continue;
                }
                /* make some room! */
                intfinfo = (interface_info_t *)
                    malloc(sizeof(struct _interface_info_t));
                if (intfinfo == NULL) {
                    ALOGE("malloc in netlink_init_interfaces_table");
                    closedir(netdir);
                    goto error;
                }
                /* copy the interface name (eth0, eth1, ...) */
                intfinfo->name = strndup((char *) de->d_name, SYSFS_PATH_MAX);
                intfinfo->i = index;
                //ALOGI("interface %s:%d found",intfinfo->name,intfinfo->i);
                add_int_to_list(intfinfo);
            }
            closedir(netdir);
        }
        ret = 0;
    error:
        return ret;
    }

    /* Detect mii */
    static int detect_mii(const char *ifname)
    {
        int retval = -1;
        int skfd = -1;
        struct ifreq ifr;
        unsigned short *data, mii_val;
        unsigned phy_id;

        /* Open a socket. */
        if (( skfd = socket( AF_INET, SOCK_DGRAM, 0 ) ) < 0 ) {
            printf("socket error\n");
            return -1;
        }

        /* Get the vitals from the interface. */
        strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
        if (ioctl(skfd, SIOCGMIIPHY, &ifr) < 0) {
            fprintf(stderr, "SIOCGMIIPHY on %s failed: %s\n", ifname,
            strerror(errno));
            (void) close(skfd);
            return -2;
        }

        data = (unsigned short *)(&ifr.ifr_data);
        phy_id = data[0];
        data[1] = 1;

        if (ioctl(skfd, SIOCGMIIREG, &ifr) < 0) {
            fprintf(stderr, "SIOCGMIIREG on %s failed: %s\n", ifr.ifr_name,
            strerror(errno));
            (void) close(skfd);
            return -3;
        }

        close(skfd);
        mii_val = data[3];

        return(((mii_val & 0x0016) == 0x0004) ? 0 : 1);
    }

#include <linux/rtnetlink.h>  //for ifinfomsg
#include <linux/netlink.h> //for nlmsghdr{}

static int get_if_flags(char *ifname, unsigned int *flags)
{
static unsigned short s_port_id = getpid();
#define ERR_OK 0
#define ERR_IF_NOT_FOUND -1
#define ERR_OTHER -2

    int nSocket, nLen, nAttrLen;
    char szBuffer[4096];
    int ret = ERR_OTHER;

    struct {
        struct nlmsghdr nh;
        struct ifinfomsg ifi;
    }struReq;

    struct sockaddr_nl struAddr;
    struct nlmsghdr *pNLH;
    struct ifinfomsg *pIIM;
    struct rtattr *pstruAttr;

    nSocket = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
    if(nSocket < 0)
    {
        ALOGE("failed to create socket:%s\n", strerror(errno));
        return ret;
    }

    memset(&struAddr, 0, sizeof(struAddr));
    struAddr.nl_family = AF_NETLINK;
    struAddr.nl_pid = 0; //s_port_id++;
    struAddr.nl_groups = 0;
    if(bind(nSocket, (struct sockaddr *)&struAddr, sizeof(struAddr)) < 0)
    {
        ALOGE("failed to bind socket:%s\n", strerror(errno));
        ret = ERR_OTHER;
        goto exit_func;
    }

    memset(&struReq, 0, sizeof(struReq));
    struReq.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struReq));
    struReq.nh.nlmsg_type = RTM_GETLINK;
    struReq.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    struReq.ifi.ifi_family = AF_UNSPEC;
    struReq.ifi.ifi_type = ARPHRD_ETHER;
    memset(&struAddr, 0, sizeof(struAddr));
    struAddr.nl_family = AF_NETLINK;
    struAddr.nl_pid = 0;
    struAddr.nl_groups = 0;

    if(sendto(nSocket, &struReq, struReq.nh.nlmsg_len, 0,
        (struct sockaddr *)&struAddr, sizeof(struAddr)) < 0)
    {
        ALOGD("failed to send data:%s\n", strerror(errno));
        ret = ERR_OTHER;
        goto exit_func;
    }

    memset(szBuffer, 0, sizeof(szBuffer));
    while((nLen = recv(nSocket, szBuffer, sizeof(szBuffer), 0)))
    {
        pNLH = (struct nlmsghdr *)szBuffer;

        while(NLMSG_OK(pNLH, nLen))
        {
            //ALOGD( "NLMsg type is %d, len is %d\n", pNLH->nlmsg_type, pNLH->nlmsg_len );
            if(pNLH->nlmsg_type == NLMSG_DONE) {
                ALOGD( "NLMsg type is DONE, BREAK!!!\n");
                ret = ERR_IF_NOT_FOUND;
                goto exit_func;
            }

            if(pNLH->nlmsg_type == NLMSG_ERROR)
            {
                struct nlmsgerr *pstruError;

                pstruError = (struct nlmsgerr *)NLMSG_DATA(pNLH);
                ALOGE( "got error[%s]\n", strerror(-pstruError -> error));
                ret = ERR_OTHER;
                goto exit_func;
            }

            pIIM = (struct ifinfomsg*)NLMSG_DATA(pNLH);
            *flags = pIIM->ifi_flags;

            pstruAttr = IFLA_RTA(pIIM);
            nAttrLen = NLMSG_PAYLOAD(pNLH, sizeof(struct ifinfomsg));

            while(RTA_OK(pstruAttr, nAttrLen))
            {
                switch(pstruAttr->rta_type)
                {
                    case IFLA_IFNAME:
                        //ALOGD( "##############%s found\n", (char *)RTA_DATA(pstruAttr));
                        if ( 0 == strcmp( ifname,(char *)RTA_DATA(pstruAttr))) {
                            ALOGV( "##############%s match\n", ifname);
                            ret = ERR_OK;
                            goto exit_func;
                        }
                        break;

                    default:
                        break;

                }

                pstruAttr = RTA_NEXT(pstruAttr, nAttrLen);
            }

            pNLH = NLMSG_NEXT(pNLH, nLen);
        }

        memset(szBuffer, 0, sizeof(szBuffer));
    }

exit_func:
    close(nSocket);

    return ret;
}

static int is_wire_inserted(const char *name)
{
    unsigned flags;
    int ret;

    ret = get_if_flags((char*)name, &flags);
    if (ret != 0) {
        ALOGD( "%s(%s) failed with ret = %d\n", __FUNCTION__, name, ret);
        return 0;
    }

    ALOGD( "%s: flags = 0x%x, IFF_LOWER_%s\n", name, flags,
          (flags & IFF_LOWER_UP) ? "UP" : "DOWN");

    return (flags & IFF_LOWER_UP) ? 1 : 0;
}


    static jint android_net_ethernet_initEthernetNative(JNIEnv *env,
                                                        jobject clazz)
    {
        int ret = -1;

        ALOGI("==>%s",__FUNCTION__);
        memset(&addr_msg, 0, sizeof(sockaddr_nl));
        addr_msg.nl_family = AF_NETLINK;
        memset(&addr_poll, 0, sizeof(sockaddr_nl));
        addr_poll.nl_family = AF_NETLINK;
        addr_poll.nl_pid = 0;//getpid();
        addr_poll.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR;

        /*
         *Create connection to netlink socket
         */
        nl_socket_msg = socket(AF_NETLINK,SOCK_RAW,NETLINK_ROUTE);
        if (nl_socket_msg <= 0) {
            ALOGE("Can not create netlink msg socket");
            goto error;
        }
        if (bind(nl_socket_msg, (struct sockaddr *)(&addr_msg),
                 sizeof(struct sockaddr_nl))) {
            ALOGE("Can not bind to netlink msg socket");
            goto error;
        }

        nl_socket_poll = socket(AF_NETLINK,SOCK_RAW,NETLINK_ROUTE);
        if (nl_socket_poll <= 0) {
            ALOGE("Can not create netlink poll socket");
            goto error;
        }

        errno = 0;
        if(bind(nl_socket_poll, (struct sockaddr *)(&addr_poll),
                sizeof(struct sockaddr_nl))) {
            ALOGE("Can not bind to netlink poll socket,%s",strerror(errno));

            goto error;
        }

        if ((ret = netlink_init_interfaces_list()) < 0) {
            ALOGE("Can not collect the interface list");
            goto error;
        }
        ALOGD("%s exited with success",__FUNCTION__);
        return ret;
    error:
        ALOGD("%s exited with error",__FUNCTION__);
        if (nl_socket_msg >0)
            close(nl_socket_msg);
        if (nl_socket_poll >0)
            close(nl_socket_poll);
        return ret;
    }

    static jstring android_net_ethernet_getInterfaceName(JNIEnv *env,
                                                         jobject clazz,
                                                         jint index)
    {
        int i = 0;
        interface_info_t *info;
        ALOGI("User ask for device name on %d, list:%X, total:%d",index,
             (unsigned int)interfaces, total_int);
        info= interfaces;
        if (total_int != 0 && index <= (total_int -1)) {
            while (info != NULL) {
                if (index == i) {
                    ALOGI("Found :%s",info->name);
                    return env->NewStringUTF(info->name);
                }
                info = info->next;
                i ++;
            }
        }
        ALOGI("No device name found");
        return env->NewStringUTF(NULL);
    }


    static jint android_net_ethernet_getInterfaceCnt() {
        return total_int;
    }

    static jint android_net_ethernet_isInterfaceAdded(JNIEnv *env,
                                                         jobject clazz,
                                                         jstring ifname) {
        int retval = 0;
        const char * ethname = env->GetStringUTFChars(ifname, NULL);
        if (ethname == NULL) {
            ALOGE("Device name NULL!");
            return 0;
        }

        retval = is_wire_inserted(ethname)? 0 : 1;

        env->ReleaseStringUTFChars(ifname, ethname);
        return retval;
    }

    static JNINativeMethod gEthernetMethods[] = {
        {"waitForEvent", "()Ljava/lang/String;",
         (void *)android_net_ethernet_waitForEvent},
        {"getInterfaceName", "(I)Ljava/lang/String;",
         (void *)android_net_ethernet_getInterfaceName},
        {"initEthernetNative", "()I",
         (void *)android_net_ethernet_initEthernetNative},
        {"getInterfaceCnt","()I",
         (void *)android_net_ethernet_getInterfaceCnt},
        {"isInterfaceAdded","(Ljava/lang/String;)I",
         (void *)android_net_ethernet_isInterfaceAdded}
    };

    int register_android_net_ethernet_EthernetManager(JNIEnv* env)
    {
        jclass eth = env->FindClass(ETH_PKG_NAME);
        ALOGI("Loading ethernet jni class");
        LOG_FATAL_IF( eth== NULL, "Unable to find class " ETH_PKG_NAME);

        dhcpInfoFieldIds.dhcpInfoClass =
            env->FindClass("android/net/DhcpInfo");
        if (dhcpInfoFieldIds.dhcpInfoClass != NULL) {
            dhcpInfoFieldIds.constructorId =
                env->GetMethodID(dhcpInfoFieldIds.dhcpInfoClass,
                                 "<init>", "()V");
            dhcpInfoFieldIds.ipaddress =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                                "ipAddress", "I");
            dhcpInfoFieldIds.gateway =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                                "gateway", "I");
            dhcpInfoFieldIds.netmask =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                                "netmask", "I");
            dhcpInfoFieldIds.dns1 =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass, "dns1", "I");
            dhcpInfoFieldIds.dns2 =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass, "dns2", "I");
            dhcpInfoFieldIds.serverAddress =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                                "serverAddress", "I");
            dhcpInfoFieldIds.leaseDuration =
                env->GetFieldID(dhcpInfoFieldIds.dhcpInfoClass,
                                "leaseDuration", "I");
        }

        return AndroidRuntime::registerNativeMethods(env,
                                                     ETH_PKG_NAME,
                                                     gEthernetMethods,
                                                     NELEM(gEthernetMethods));
    }

};
