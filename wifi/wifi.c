/*
 * Copyright 2008, The Android Open Source Project
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
 */

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
// MStar Android Patch Begin
#include <ctype.h>
#include <net/if.h>
#include <sys/inotify.h>
#include <sys/wait.h>
// MStar Android Patch End

#include "hardware_legacy/wifi.h"
#include "libwpa_client/wpa_ctrl.h"

#define LOG_TAG "WifiHW"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "private/android_filesystem_config.h"
// MStar Android Patch Begin
#undef HAVE_LIBC_SYSTEM_PROPERTIES
// MStar Android Patch End
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#endif

static struct wpa_ctrl *ctrl_conn;
static struct wpa_ctrl *monitor_conn;

/* socket pair used to exit from a blocking read */
static int exit_sockets[2];

extern int do_dhcp();
extern int ifc_init();
extern void ifc_close();
extern char *dhcp_lasterror();
extern void get_dhcp_info();
extern int init_module(void *, unsigned long, const char *);
extern int delete_module(const char *, unsigned int);

// MStar Android Patch Begin
// declare some function avoid some warning message
extern int ifc_up(const char *name);
extern int ifc_down(const char *name);
static int is_wifi_device(int vid,int pid);
static int badname(const char *name);
int get_wifi_device_num();
void wifi_close_sockets();
void wifi_wpa_ctrl_cleanup(void);
// MStar Android Patch End

static char primary_iface[PROPERTY_VALUE_MAX];
// TODO: use new ANDROID_SOCKET mechanism, once support for multiple
// sockets is in

// MStar Android Patch Begin
//realtek_cu
#define WIFI_DRIVER_MODULE_PATH         "/system/lib/modules"
#define REL_DRIVER_KO_1 "cfg80211" //the modules name is cfg80211
#define REL_DRIVER_KO_2 "8192cu"
//#define REL_DRIVER_KO_3 "cfg80211"
//original module name cfg80211
#define ORG_CFG80211 "cfg80211"

//REALTEK_SU
#define REL_SU_DRIVER_KO "8191su"
#define REL_ETU_DRIVER_KO "8812eu"
#define REL_8812_AUS "8812au"
#define REL_8188_ETU "8188etu"
#define REL_8712_U   "8712u"


//REALTEK_DU
#define REL_DU_DRIVER_KO "8192du"
//REALTEK_EU
#define REL_EU_DRIVER_KO "8192eu"

//atheros use the different cfg80211 ,so we must rename the cfg80211 ko to cfg80211_ath6k
//Atheros_2
#define ATH6K_DRIVER_KO_1 "compat"
#define ATH6K_DRIVER_KO_2 "cfg80211_ath6k" //the modules name is cfg80211
#define ATH6K_DRIVER_KO_3 "ath6kl_usb"

//Atheros 9375
#define ATH9375_DRIVER_KO_1 "compat_9375"
#define ATH9375_DRIVER_KO_2 "cfg80211_ath6k_9375"
#define ATH9375_DRIVER_KO_3 "ath6kl_usb_9375"

//ralink
#define RALINK_DRIVER_KO "ralink"

//ralink new driver for 5572&5372
#define RAL5572_DRIVER_KO_1 "cfg80211"
#define RAL5572_DRIVER_KO_2 "rt5572sta"

//ralink softap driver for 5572
#define RAL5572_AP_DRIVER_KO_1 "rtutil5572ap"
#define RAL5572_AP_DRIVER_KO_2 "rt5572ap"
#define RAL5572_AP_DRIVER_KO_3 "rtnet5572ap"

#define STA_INF "wlan0"
#define P2P_INF "p2p0"

//bcm
#define BCM_DRIVER_KO_1 "bcm_usbshim"
#define BCM_DRIVER_KO_2 "wl"
// MStar Android Patch End

#ifndef WIFI_DRIVER_MODULE_ARG
#define WIFI_DRIVER_MODULE_ARG      ""
#endif
#ifndef WIFI_FIRMWARE_LOADER
#define WIFI_FIRMWARE_LOADER        ""
#endif
#define WIFI_TEST_INTERFACE        "sta"

#ifndef WIFI_DRIVER_FW_PATH_STA
#define WIFI_DRIVER_FW_PATH_STA    NULL
#endif
#ifndef WIFI_DRIVER_FW_PATH_AP
#define WIFI_DRIVER_FW_PATH_AP     NULL
#endif
#ifndef WIFI_DRIVER_FW_PATH_P2P
#define WIFI_DRIVER_FW_PATH_P2P    NULL
#endif

#ifndef WIFI_DRIVER_FW_PATH_PARAM
#define WIFI_DRIVER_FW_PATH_PARAM    "/sys/module/wlan/parameters/fwpath"
#endif

#define WIFI_DRIVER_LOADER_DELAY    1000000

// MStar Android Patch Begin
#define SYSFS_PATH_MAX 256
#define USB_FS_DIR "/dev/bus/usb"
//#define DRIVER_LINE_MAX 200
//#define DRIVER_MEMBER_NUM 15
//#define DRIVER_NAME_MAX_LEN 30

#undef WIFI_DRIVER_FW_PATH_STA
#define WIFI_DRIVER_FW_PATH_STA         "STA"
#undef WIFI_DRIVER_FW_PATH_AP
#define WIFI_DRIVER_FW_PATH_AP           "AP"
#undef WIFI_DRIVER_FW_PATH_P2P
#define WIFI_DRIVER_FW_PATH_P2P         "P2P"

#define RTL8192CU "RTL8192CU"
#define RTL8191SU "RTL8191SU"
#define RTL8192DU "RTL8192DU"
#define RTL8192EU "RTL8192EU"
#define ATH9271 "ATH9271"
#define ATH1021 "ATH1021"
#define ATH9375 "ATH9375"
#define RAL5370 "RAL5370"
#define BCM43236 "BCM43236"
#define RTL8812AUS "RTL8812AUS"
#define RTL8188ETU "RTL8188ETU"
#define RTL8712U   "RTL8712U"

//realtek
#define SUP_RTL8192CU 1
#define SUP_RTL8192DU 1
#define SUP_RTL8191SU 0
#define SUP_RTL8192EU 1
//atheros
#define SUP_ATH9271 0
#define SUP_ATH1021 1
#define SUP_ATH9375 1
//ralink 5370 5372 5570 5572
#define SUP_RAL5370 1
//bcm 43236
#define SUP_BCM43236 0
// MStar Android Patch End

static const char IFACE_DIR[]           = "/data/system/wpa_supplicant";

static const char DRIVER_MODULE_ARG[]   = WIFI_DRIVER_MODULE_ARG;
// MStar Android Patch Begin
static char WLAN_DRIVER[] = "wlan.driver";
// MStar Android Patch End
static const char FIRMWARE_LOADER[]     = WIFI_FIRMWARE_LOADER;
static const char DRIVER_PROP_NAME[]    = "wlan.driver.status";


static const char SUPP_CONFIG_TEMPLATE[]= "/system/etc/wifi/wpa_supplicant.conf";
// MStar Android Patch Begin
static const char P2P_CONFIG_TEMPLATE[] = "/system/etc/wifi/p2p_supplicant.conf";
// MStar Android Patch End
static const char SUPP_CONFIG_FILE[]    = "/data/misc/wifi/wpa_supplicant.conf";
static const char P2P_CONFIG_FILE[]     = "/data/misc/wifi/p2p_supplicant.conf";
static const char CONTROL_IFACE_PATH[]  = "/data/misc/wifi/sockets";
static const char MODULE_FILE[]         = "/proc/modules";

static const char IFNAME[]              = "IFNAME=";
#define IFNAMELEN			(sizeof(IFNAME) - 1)
static const char WPA_EVENT_IGNORE[]    = "CTRL-EVENT-IGNORE ";

// MStar Android Patch Begin
static const char SYSFS_CLASS_NET[]     = "/sys/class/net";
// MStar Android Patch End

static const char SUPP_ENTROPY_FILE[]   = WIFI_ENTROPY_FILE;
static unsigned char dummy_key[21] = { 0x02, 0x11, 0xbe, 0x33, 0x43, 0x35,
                                       0x68, 0x47, 0x84, 0x99, 0xa9, 0x2b,
                                       0x1c, 0xd3, 0xee, 0xff, 0xf1, 0xe2,
                                       0xf3, 0xf4, 0xf5 };

// MStar Android Patch Begin
/* Is either SUPPLICANT_NAME or P2P_SUPPLICANT_NAME */
static char supplicant_name[PROPERTY_VALUE_MAX] = "startSuppl";
/* Is either SUPP_PROP_NAME or P2P_PROP_NAME */
static char supplicant_prop_name[PROPERTY_KEY_MAX] = "init.svc.startSuppl";
// MStar Android Patch End

// MStar Android Patch Begin
//1 --> realtek 2--> ralink 3-->atheros 4-->atheros_2 5-->realtek_su 6-->realtek_du
static int which_device_loaded = 0;

static int has_device = 0;


struct vid_pid {
    unsigned short int vid;
    unsigned short int pid;
};

struct device_info {
    char device_name[20];
    int vid_pid_count;
    struct vid_pid *device_vid_pid;
    struct device_info *next;
    char capability; //p2p|softap|sta
    char order;
};

static struct device_info *g_device_info_head = NULL;

static struct device_info* get_device_info_head() {
    if (g_device_info_head == NULL) {
        int size = sizeof(struct device_info);
        //printf("sizeof struct device_info = %d \n",size);
        g_device_info_head = malloc(size);
        if (g_device_info_head == NULL) {
            ALOGD("malloc error!!\n");
            return NULL;
        }
        memset(g_device_info_head,0,size);
    }
    return g_device_info_head;
}

static void add_device_info(struct device_info *tmp_device_info)
{
    struct device_info *node_head = get_device_info_head();
    ALOGD("tmp_device_info name = %s\n",tmp_device_info->device_name);
    if (node_head && tmp_device_info) {
        struct device_info *curptr = node_head;
        if (curptr) {
            int count = 0;
            for (curptr = node_head;
                curptr->next != NULL;
                curptr = curptr->next) {
                count ++;
                //printf("move next! count = %d\n",count);
            };
            curptr->next = tmp_device_info;
        } else {
            ALOGD("curptr == NULL\n");
        }
    }
}

static void list_device_info() {
    struct device_info *tmp_head = get_device_info_head();
    struct device_info *curptr;
    ALOGD("==>enter func %s",__func__);
    for (curptr = tmp_head->next;curptr != NULL;curptr = curptr->next) {
        ALOGD("device name = %s",curptr->device_name);
        int count = curptr->vid_pid_count;
        int i = 0;
        ALOGD("count = %d",count);
        for (i = 0; i < count;i++) {
            ALOGD("device[%d].vid = %d :pid = %d",i,curptr->device_vid_pid[i].vid,curptr->device_vid_pid[i].pid);
        }
        ALOGD("capability = %d",curptr->capability);
    }
}

static int hex_str2int(const char* hex_str) {
    //ALOGD("%s hex_str = %s\n",__func__,hex_str);
    if ((strncmp(hex_str,"0x",strlen("0x")) == 0) || (strncmp(hex_str,"0X",strlen("0X")) == 0)) {
        char *hex_num;
        hex_num = hex_str + 2;
        //ALOGD("hex_num = %s\n",hex_num);
        int len = strlen(hex_num);
        int i;
        int n = 0;
        int temp = 0;
        for (i = 0;i < len;i++) {
            if (hex_num[i] >= 'a' && hex_num[i] <= 'f') {
                n = hex_num[i] - 'a' + 10;
            } else if (hex_num[i] >= 'A' && hex_num[i] <= 'F') {
                n = hex_num[i] - 'A' + 10;
            } else if (hex_num[i] >= '0' && hex_num[i] <= '9') {
                n = hex_num[i] - '0';
            } else {
                ALOGD(" %c ==>error hex num please check!!",hex_num[i]);
                break;
            }
            temp = temp*16 + n;

        }
        return temp;
    } else {
        ALOGD("hex_str = %s num are wrong !!Please check the config\n",hex_str);
        return 0;
    }
}

static int is_badline(const char *tmp_str) {
    if (tmp_str[0] == '\0' || tmp_str[0] == '#' || tmp_str[0] == '\n') {
       return 1;
    } else {
        return 0;
    }
}

static int parse_config(const char *file_path) {
    FILE *fp = NULL;

    g_device_info_head = get_device_info_head();
    //the member order of head mean the devices count
    g_device_info_head->order = 0;

    fp = fopen(file_path,"r");
    if (!fp) {
        ALOGE("Can not open wifi config %s,please check !",file_path);
        return 0;
    }

    char temp_str[128]="";
    memset(temp_str,0,sizeof(temp_str));
    char seps[] = "=";

    while (fgets(temp_str,sizeof(temp_str),fp)) {
        //ALOGD("len of temp_str = %d\n",strlen(temp_str));
        //skip badline
        if (is_badline(temp_str)) {
            ALOGW("skip space line!");
            continue;
        }
        temp_str[strlen(temp_str) - 1] = '\0';
        char *value=NULL;
        int count_vid_pid = 0;
        //ALOGD("temp_str = %s\n",temp_str);:
        //parse vendor name
        if (strncmp(temp_str,"wifi_vendor_name",strlen("wifi_vendor_name")) == 0) {
            ALOGD("=============");
            struct device_info *tmp_device_info =NULL;
            tmp_device_info = malloc(sizeof(struct device_info));
            if (tmp_device_info == NULL) {
                ALOGD("Malloc error!");
                fclose(fp);
                return 0;
            }

            memset(tmp_device_info,0,sizeof(struct device_info));
            strtok(temp_str,seps);
            value = strtok(NULL,seps);
            ALOGD("value = %s",value);
            strcpy(tmp_device_info->device_name,value);

            //parse order
            memset(temp_str,0,sizeof(temp_str));
            //fgets(temp_str,sizeof(temp_str),fp);
            while (fgets(temp_str,sizeof(temp_str),fp)) {
                if (is_badline(temp_str)) {
                    ALOGW("skip the badline!!,next should be order");
                    continue;
                }

                temp_str[strlen(temp_str) - 1] = '\0';
                if (strncmp(temp_str,"order",strlen("order")) == 0) {
                    strtok(temp_str,seps);
                    value = strtok(NULL,seps);
                    //ALOGD("value = %d\n",atoi(value));
                    tmp_device_info->order = atoi(value);

                    if (g_device_info_head->order < tmp_device_info->order) {
                        //maybe the order is discontinuous,so the order of node_head mean the bigest num of order
                        g_device_info_head->order = tmp_device_info->order;
                    } else if (g_device_info_head->order == tmp_device_info->order) {
                        ALOGE("####################################");
                        ALOGE("ERROR in wifi.cfg!!!! wifi.cfg %s 'order' number,should not the same num!!",tmp_device_info->device_name);
                        ALOGE("####################################");
                        return 0;
                    }
                } else {
                    ALOGE("ERROR in wifi.cfg!!! here should be order num!,please check wifi.cfg temp_str = %s",temp_str);
                    return 0;
                }
                break;
            }

            //parse count_vid_pid
            memset(temp_str,0,sizeof(temp_str));
            //fgets(temp_str,sizeof(temp_str),fp);
            while (fgets(temp_str,sizeof(temp_str),fp)) {
                if (is_badline(temp_str) == 1) {
                    ALOGW("skip the badline!!next should be count_vid_pid");
                    continue;
                }
                temp_str[strlen(temp_str) - 1] = '\0';
                if (strncmp(temp_str,"count_vid_pid",strlen("count_vid_pid")) == 0) {
                    strtok(temp_str,seps);
                    value = strtok(NULL,seps);
                    //ALOGD("value = %d\n",atoi(value));
                    count_vid_pid = atoi(value);
                } else {
                    ALOGD("ERROR in wifi.cfg!!!here should be count_vid_pid,please check wifi.cfg temp_str = %s",temp_str);
                    return 0;
                }
                tmp_device_info->vid_pid_count = count_vid_pid;
                break;
            }
            memset(temp_str,0,sizeof(temp_str));
            int count = 0;
            tmp_device_info->device_vid_pid = malloc(count_vid_pid * sizeof(struct vid_pid));
            // only gets 'count_vid_pid' line ,so count < count_vid_pid must be first
            while (count < count_vid_pid && fgets(temp_str,sizeof(temp_str),fp)) {
                if (is_badline(temp_str) == 1) {
                    ALOGW("skip badline should be vid_pid");
                    continue;
                }
                temp_str[strlen(temp_str) - 1] = '\0';
                if (strncmp(temp_str,"vid_pid",strlen("vid_pid")) == 0) {
                    strtok(temp_str,seps);
                    value = strtok(NULL,seps);
                    //ALOGD("value = %s\n",value);
                    char *tmp_vid="";
                    char *tmp_pid="";
                    tmp_vid = strtok(value,":");
                    tmp_pid = strtok(NULL,":");
                    tmp_device_info->device_vid_pid[count].vid = hex_str2int(tmp_vid);
                    tmp_device_info->device_vid_pid[count].pid = hex_str2int(tmp_pid);
                    count ++;
                } else {
                    ALOGE("ERROR in wifi.cfg !!!here should be vid_pid,please check wifi.cfg temp_str = %s",temp_str);
                    return 0;
                }
            }

            //parse capability (sta|softap|p2p)
            int sta_capability = 0;
            memset(temp_str,0,sizeof(temp_str));
            while (fgets(temp_str,sizeof(temp_str),fp)) {
                if (is_badline(temp_str) == 1) {
                    ALOGW("skip badline , should be sta_capability");
                    continue;
                }
                temp_str[strlen(temp_str) - 1] = '\0';
                if (strncmp(temp_str,"sta",strlen("sta")) == 0) {
                    strtok(temp_str,seps);
                    value = strtok(NULL,seps);
                    sta_capability = atoi(value);
                } else {
                    ALOGE("ERROR in wifi.cfg !!! here should be sta_capability temp_str = %s",temp_str);
                    return 0;
                }
                break;
            }

            int softap_capability = 0;
            memset(temp_str,0,sizeof(temp_str));
            while (fgets(temp_str,sizeof(temp_str),fp)) {
                if (is_badline(temp_str) == 1) {
                    ALOGW("skip badline , should be softap_capability");
                    continue;
                }
                temp_str[strlen(temp_str) - 1] = '\0';
                if (strncmp(temp_str,"softap",strlen("softap")) == 0) {
                    strtok(temp_str,seps);
                    value = strtok(NULL,seps);
                    softap_capability = atoi(value);
                } else {
                    ALOGE("ERROR in wifi.cfg !!! here should be softap_capability temp_str = %s",temp_str);
                    return 0;
                }
                break;
            }

            int p2p_capability = 0;
            memset(temp_str,0,sizeof(temp_str));
            while (fgets(temp_str,sizeof(temp_str),fp)) {
                if (is_badline(temp_str) == 1) {
                    ALOGW("skip badline , should be softap_capability");
                    continue;
                }
                temp_str[strlen(temp_str) - 1] = '\0';
                if (strncmp(temp_str,"p2p",strlen("p2p")) == 0) {
                    strtok(temp_str,seps);
                    value = strtok(NULL,seps);
                    p2p_capability = atoi(value);
                } else {
                    ALOGE("ERROR in wifi.cfg !!! here should be softap_capability temp_str = %s",temp_str);
                    return 0;
                }
                break;
            }
            tmp_device_info->capability = p2p_capability*4 + softap_capability*2 + sta_capability;
            tmp_device_info->next = NULL;
            //add the device
            add_device_info(tmp_device_info);
        }
    }
    fclose(fp);
    return 1;
}

static struct device_info *get_device_by_order(int order) {
    ALOGD("==>enter func %s",__func__);
    struct device_info *tmp_head = get_device_info_head();
    struct device_info *curptr;
    for (curptr = tmp_head->next;curptr != NULL;curptr = curptr->next) {
        if (curptr->order == order) {
            return curptr;
        }
    }
    return NULL;
}

static struct device_info *get_device_by_name(const char *device_name) {
    ALOGD("==>enter func %s",__func__);
    struct device_info *tmp_head = get_device_info_head();
    struct device_info *curptr;
    for (curptr = tmp_head->next;curptr != NULL;curptr = curptr->next) {
        if (strcmp(curptr->device_name,device_name) == 0) {
            return curptr;
        }
    }
    return NULL;
}

static int get_device_capability (const char *device_name) {
    struct device_info *curptr = get_device_by_name(device_name);
    if (curptr != NULL) {
        return curptr->capability;
    }
    return 0;
}

int is_support_p2p() {
    if (get_wifi_device_num() < 1) {
        return 0;
    }
    char wlan_driver[PROPERTY_VALUE_MAX] = {'\0'};
    property_get(WLAN_DRIVER,wlan_driver, NULL);
    int capability = get_device_capability(wlan_driver);
    ALOGD("func %s capability = %d",__func__,capability);
    if ((capability & 0x04)>>2) {
        return 1;
    } else {
        return 0;
    }
}

int is_support_softap() {
    if (get_wifi_device_num() < 1) {
        return 0;
    }
    char wlan_driver[PROPERTY_VALUE_MAX] = {'\0'};
    property_get(WLAN_DRIVER,wlan_driver, NULL);
    int capability = get_device_capability(wlan_driver);
    ALOGD("func %s capability = %d",__func__,capability);
    if ((capability & 0x02) >> 1) {
        return 1;
    } else {
        return 0;
    }

}

void set_driver_property(int device_id) {
    ALOGD("enter func %s device_id = %d",__func__,device_id);
    struct device_info *curptr;
    curptr = get_device_by_order(device_id + 1);
    if (curptr != NULL) {
        property_set(WLAN_DRIVER,curptr->device_name);
    }
}

static int get_wifi_ifname_from_prop(char *ifname)
{
    ifname[0] = '\0';
    if (property_get("wifi.interface", ifname, WIFI_TEST_INTERFACE)
        && strcmp(ifname, WIFI_TEST_INTERFACE) != 0)
        return 0;

    ALOGE("Can't get wifi ifname from property \"wifi.interface\"");
    return -1;
}

static int check_wifi_ifname_from_proc(char *buf, char *target)
{
#define PROC_WIRELESS_PATH "/proc/net/wireless"
#define MAX_WIFI_IFACE_NUM 10

    char linebuf[1024];
    unsigned char wifi_ifcount = 0;
    char wifi_ifaces[MAX_WIFI_IFACE_NUM][IFNAMSIZ+1];
    int i, ret = -1;
    int match = -1; /* if matched, this means the index*/
    FILE *f = fopen(PROC_WIRELESS_PATH, "r");

    if (buf)
        buf[0] = '\0';

    if (!f) {
        ALOGE("Unable to read %s: %s", PROC_WIRELESS_PATH, strerror(errno));
        goto exit;
    }

    /* Get wifi interfaces form PROC_WIRELESS_PATH*/
    memset(wifi_ifaces, 0, MAX_WIFI_IFACE_NUM * (IFNAMSIZ+1));
    while (fgets(linebuf, sizeof(linebuf)-1, f)) {
        if (strchr(linebuf, ':')) {
            char *dest = wifi_ifaces[wifi_ifcount];
            char *p = linebuf;

            while (*p && isspace(*p)) {
                ++p;
            }
            while (*p && *p != ':') {
                *dest++ = *p++;
            }
            *dest = '\0';

            ALOGD("%s: find %s", __func__, wifi_ifaces[wifi_ifcount]);
            wifi_ifcount++;
            if (wifi_ifcount>=MAX_WIFI_IFACE_NUM) {
                ALOGD("%s: wifi_ifcount >= MAX_WIFI_IFACE_NUM(%u)", __func__, MAX_WIFI_IFACE_NUM);
                break;
            }
        }
    }
    fclose(f);

    if (target) {
        /* Try to find match */
        for (i = 0;i < wifi_ifcount;i++) {
            if (strcmp(target, wifi_ifaces[i]) == 0) {
                match = i;
                break;
            }
        }
    } else {
        /* No target, use the first wifi_iface as target*/
        match = 0;
    }

    if (buf && match >= 0)
        strncpy(buf, wifi_ifaces[match], IFNAMSIZ);

    if (match >= 0)
        ret = 0;
exit:
    return ret;
}

static int get_wifi_ifname_from_proc(char *ifname)
{
    return check_wifi_ifname_from_proc(ifname, NULL);
}

//@FIXME Merge
static char *wifi_ifname(int index)
{
#define WIFI_P2P_INTERFACE "p2p0"

    char primary_if[IFNAMSIZ+1];
    char second_if[IFNAMSIZ+1];

    //if (index == PRIMARY) {
        primary_iface[0] = '\0';
        if (get_wifi_ifname_from_prop(primary_if) == 0 &&
            check_wifi_ifname_from_proc(primary_iface, primary_if) == 0) {
            return primary_iface;
        }
    //} else if (index == SECONDARY) {
    //    if (check_wifi_ifname_from_proc(NULL, WIFI_P2P_INTERFACE) == 0)
    //        return WIFI_P2P_INTERFACE;
    //}
    return NULL;
}
// MStar Android Patch End

static int insmod(const char *filename, const char *args)
{
    void *module;
    unsigned int size;
    int ret;

    module = load_file(filename, &size);
    if (!module)
        return -1;

    ret = init_module(module, size, args);

    free(module);

    return ret;
}

static int rmmod(const char *modname)
{
    int ret = -1;
    int maxtry = 10;

    while (maxtry-- > 0) {
        ret = delete_module(modname, O_NONBLOCK | O_EXCL);
        if (ret < 0 && errno == EAGAIN)
            usleep(500000);
        else
            break;
    }

    if (ret != 0)
        ALOGD("Unable to unload driver module \"%s\": %s\n",
             modname, strerror(errno));
    return ret;
}

int do_dhcp_request(int *ipaddr, int *gateway, int *mask,
                    int *dns1, int *dns2, int *server, int *lease) {
    /* For test driver, always report success */
    if (strcmp(primary_iface, WIFI_TEST_INTERFACE) == 0)
        return 0;

    if (ifc_init() < 0)
        return -1;

    if (do_dhcp(primary_iface) < 0) {
        ifc_close();
        return -1;
    }
    ifc_close();
    get_dhcp_info(ipaddr, gateway, mask, dns1, dns2, server, lease);
    return 0;
}

const char *get_dhcp_error_string() {
    return dhcp_lasterror();
}

// MStar Android Patch Begin
static int system_call( const char* format, ... ) __attribute__
    ((format (printf, 1, 2)));
static int system_call(const char* format,...)
{
    char buf[256];
    char *arg[10],*delim=" ",*tmp,*command;
    int arg_index = 0,status;
    pid_t pid;
    va_list args;
    va_start(args,format);
    vsnprintf(buf,sizeof(buf),format,args);
    va_end(args);

    // if format  > 256
    if (strlen(buf) >= sizeof(buf) - 1) {
        ALOGD("buffer may have been truncated");
    }
    ALOGD("%s",buf);
    command = strtok(buf,delim);
    tmp = command;
    do {
        arg[arg_index] = malloc(sizeof(char) * (strlen(tmp) + 1));
        memset(arg[arg_index],'\0',(strlen(tmp) + 1));
        strncpy(arg[arg_index],tmp,strlen(tmp));
        arg_index ++;
    } while ((tmp = strtok(NULL,delim)));
    arg[arg_index] = NULL;

    pid = fork();
    if (pid == 0) {
        execvp(command,arg);
    }
    else if (pid < 0) {
        ALOGE("Create process fail");
    }

    arg_index --;
    for (;arg_index >= 0; arg_index --) {
        free(arg[arg_index]);
    }
    waitpid(pid,&status,0);

    return 0;
}

static int is_wifi_device(int vid,int pid) {
    struct device_info *tmp_head = get_device_info_head();
    struct device_info *curptr;
    //ALOGD("==>enter func %s\n",__func__);
    for (curptr = tmp_head->next;curptr != NULL;curptr = curptr->next) {
        int count = curptr->vid_pid_count;
        int i = 0;
        for (i = 0; i < count;i++) {
            if ((vid == curptr->device_vid_pid[i].vid) && (pid == curptr->device_vid_pid[i].pid)) {
                return curptr->order;
            }
        }

    }
    return 0;
}

//ignore some dir
static int badname(const char *name)
{
    //ALOGD("%s %s",__FUNCTION__,name);
    if (*name == '.' || isalpha(*name)) {
        return 1;
    }
    while (*name) {
        if (':'==(*name++)) {
            return 1;
        }
    }
    return 0;
}

static int find_existing_devices()
{
    int device_count = get_device_info_head()->order;
    //int device_count = g_device_info_head->order;
    int wifi_device_list[device_count];
    memset(wifi_device_list,0,sizeof(wifi_device_list));
    //int wifi_device_list[11] = {0,0,0,0,0,0,0,0,0,0,0};
    char devname[80],detail[8],busdir_path[32],sdio_path[48];
    DIR *busdir , *devdir , *sdiodir;
    struct dirent *de;
    int num,fd,pid,vid;
    int done = 0;
    num=0;

    busdir = opendir("/sys/bus/usb/devices");
    snprintf(busdir_path, sizeof busdir_path, "%s","/sys/bus/usb/devices");
    if (busdir == 0)
        return 0;

    while ((de = readdir(busdir)) != 0 && !done) {
        //ALOGE("enter %s %d %s/%s\n",__FUNCTION__,__LINE__,busdir_path,de->d_name);
        if (badname(de->d_name))
            continue;
        snprintf(devname, sizeof devname, "%s/%s/%s", busdir_path, de->d_name,"idProduct");
        memset(detail,0,8);

        fd = open(devname, O_RDONLY);
        if (fd < 0){
            ALOGE( " enter %d devname %s \n",__LINE__, strerror(errno));
        }
        read(fd,detail,8);
        sscanf(detail,"%x",&pid);
        close(fd);

        memset(detail,0,8);
        snprintf(devname, sizeof devname, "%s/%s/%s", busdir_path, de->d_name,"idVendor");
        fd = open(devname, O_RDONLY);
        if (fd < 0){
            ALOGE( " enter %d devname %s \n",__LINE__, strerror(errno));
        }
        read(fd,detail,8);
        close(fd);

        sscanf(detail,"%x",&vid);
        done = is_wifi_device(vid,pid);
        if (0 != done){
            wifi_device_list[done - 1]++;
            num++;
        }
        done=0;
    } //end of busdir while
    closedir(busdir);
    //find sdio device
    // MStar Android Patch Begin
    sdiodir = opendir("/sys/bus/sdio/devices");
    if (sdiodir != NULL) {
        while((de = readdir(sdiodir)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;
             snprintf(sdio_path,sizeof sdio_path,"/sys/bus/sdio/devices/%s/device",de->d_name);
             fd = open(sdio_path, O_RDONLY);
             if (fd < 0) {
                ALOGD("open %s fail :%s\n",sdio_path,strerror(errno));
                close(fd);
                continue;
             }
             read(fd,detail,8);
             sscanf(detail,"0x%x",&pid);
             memset(detail,8,0);
             close(fd);

             snprintf(sdio_path,sizeof sdio_path,"/sys/bus/sdio/devices/%s/vendor",de->d_name);
             fd = open(sdio_path, O_RDONLY);
             if (fd < 0) {
                ALOGD("open %s fail :%s\n",sdio_path,strerror(errno));
                close(fd);
                continue;
             }
             read(fd,detail,8);
             sscanf(detail,"0x%x",&vid);
             close(fd);
             done = is_wifi_device(vid,pid);
             if (done != 0) {
                wifi_device_list[done - 1]++;
                num++;
             }
        }
    }
    closedir(sdiodir);
    // MStar Android Patch End
    //default realtek is the inner wifi
    int j = 0;
    for (j = device_count - 1;j >= 0;j--) {
        if (wifi_device_list[j] >= 1) {
            ALOGD("wifi_device_list[%d] = %d",j,wifi_device_list[j]);
            set_driver_property(j);
            break;
        }
    }
    if (0 == num) {
        property_set(WLAN_DRIVER,"");
    }
    return num;
}

int get_wifi_device_num()
{
    int num;
    num = find_existing_devices();
    ALOGE("%s is %d",__FUNCTION__,num);
    return num;
}

// MStar Android Patch Begin
int load_prealloc_module(const char *module)
{
    char driver_status[PROPERTY_VALUE_MAX];
    char daemon_cmd[PROPERTY_VALUE_MAX * 2];

    snprintf(daemon_cmd, sizeof(daemon_cmd), "loadwifi:%s", module);

    sched_yield();
    property_set("ctl.start", daemon_cmd);
    usleep(500*1000);

    int count = 100;
    while (count-- > 0) {
        if (property_get("init.svc.loadwifi", driver_status, NULL)) {
            if (strcmp(driver_status, "stopped") == 0) {
                return 0;
            } else if (strcmp(driver_status, "failed") == 0) {
                return -1;
            }
        }
        ALOGD("func %s count = %d ",__func__,count);
        usleep(200000);
    }
    ALOGD("Time out loading mtprealloc7601Usta.ko!!");
    return -1;
}
// MStar Android Patch End

static int watch_fd = -1;

int init_detect_wifi_device()
{
    int res;
    int i = 0;

    // MStar Android Patch Begin
    if (load_prealloc_module("MTKM") != 0) {
        ALOGD("if you use mediatek wifi dongle, something may goes wrong!!!");
    }

    if (load_prealloc_module("RTKM") != 0) {
        ALOGD("if you use realtek wifi dongle, something may goes wrong!!!");
    }
    // MStar Android Patch End

    ALOGD("start to parse_config!");
    if (parse_config("/system/etc/wifi/wifi.cfg") == 1) {
        ALOGD("parse_config successfully!!");
    } else {
        ALOGE("ERROR in parse wifi.cfg !!please check the wifi.cfg!!");
    }
    char path[SYSFS_PATH_MAX];
    ALOGD("detect_wifi_device!!!");
    watch_fd = inotify_init();
    // add notify dir dynamic
    DIR *bus_dir;
    struct dirent *de;
    if ((bus_dir = opendir(USB_FS_DIR)) != NULL) {
        while ((de = readdir(bus_dir)) != NULL) {
            if ((!strcmp(de->d_name,".")) || (!strcmp(de->d_name,"..")))
                continue;
            snprintf(path,SYSFS_PATH_MAX,"%s/%s",USB_FS_DIR,de->d_name);
            ALOGD("path = %s",path);
            res = inotify_add_watch(watch_fd,path,IN_CREATE|IN_DELETE);
            if (res < 0) {
                ALOGD("inotify_add_watch failed!!");
            }
        }
        closedir(bus_dir);
    } else {
        return 0;
    }
    //end
#if 0
    for (i = 1;i < 6;i++) {
        snprintf(path,sizeof(path),"%s/%03d",USB_FS_DIR,i);
        ALOGD("path  = %s",path);
        res = inotify_add_watch(watch_fd,path,IN_CREATE|IN_DELETE);
        if (res < 0) {
            ALOGD("inotify_add_watch failed!!");
            //return -1;
        }
    }
#endif
    int device_count = get_wifi_device_num();
    ALOGD("============device_count = %d",device_count);
    if (has_device != device_count){
        has_device = device_count;
    }
    return 1;
}

int detect_wifi_device()
{
    int res;
    char buf[512];
    int len;
    read(watch_fd,&buf,sizeof(buf));
    ALOGD("there is a usb device  !");
    int device_count = get_wifi_device_num();
    ALOGD("===================device_count = %d",device_count);
    if (has_device == device_count) {
        return -1;
    } else if (has_device > device_count) {
        has_device = device_count;
        return 0;
    } else {
        has_device = device_count;
        return 1;
    }

}

static int get_driver_info()
{
    DIR  *netdir;
    struct dirent *de;
    char path[SYSFS_PATH_MAX];
    char link[SYSFS_PATH_MAX];
    int ret = 0;
    if ((netdir = opendir(SYSFS_CLASS_NET)) != NULL) {
        while ((de = readdir(netdir))!=NULL) {
            struct dirent **namelist = NULL;
            int cnt;
            if ((!strcmp(de->d_name,".")) || (!strcmp(de->d_name,"..")))
                continue;
            snprintf(path, SYSFS_PATH_MAX, "%s/%s/wireless", SYSFS_CLASS_NET, de->d_name);
            if (access(path, F_OK)) {
                snprintf(path, SYSFS_PATH_MAX, "%s/%s/phy80211", SYSFS_CLASS_NET, de->d_name);
                if (access(path, F_OK))
                    continue;
            } else {
                ret = 1;
            }

        }
    }
    closedir(netdir);
    ALOGD("func %s ret = %d\n",__func__,ret);
    return ret;
}
// MStar Android Patch End

int is_wifi_driver_loaded() {
    // MStar Android Patch Begin
    if (!get_driver_info()) {
        property_set(DRIVER_PROP_NAME,"unloaded");
        return 0;
    } else {
        property_set(DRIVER_PROP_NAME,"ok");
        return 1;
    }
    // MStar Android Patch End
}

// MStar Android Patch Begin
static int wifi_common_unload_driver(int mode)
{
    ALOGD("enter fuc %s mode : = %s\n",__func__,mode ? "softap":"sta");
    struct device_info *curptr = get_device_by_order(which_device_loaded);
    char wlan_driver[PROPERTY_VALUE_MAX] = {'\0'};
    if (curptr != NULL) {
        strcpy(wlan_driver,curptr->device_name);
    } else {
        ALOGD("cannot find device by order %d\n",which_device_loaded);
    }

    char driver_scripts_prop[30] = "init.svc.unloadwifi";
    char driver_status[PROPERTY_VALUE_MAX];
    char daemon_cmd[PROPERTY_VALUE_MAX * 2];
    if (mode == 0) {
        strcpy(driver_scripts_prop,"init.svc.unloadwifi");
        snprintf(daemon_cmd,sizeof(daemon_cmd),"unloadwifi:%s",wlan_driver);
    } else if (mode == 1) {
        strcpy(driver_scripts_prop,"init.svc.unloadapwifi");
        snprintf(daemon_cmd,sizeof(daemon_cmd),"unloadapwifi:%s",wlan_driver);
    }

    //property_set(DRIVER_PROP_NAME,"");
    sched_yield();
    property_set("ctl.start", daemon_cmd);
    usleep(500*1000);

    int count = 100;
    while (count-- > 0) {
        if (property_get(driver_scripts_prop, driver_status, NULL)) {
            if (strcmp(driver_status, "stopped") == 0) {
                //make sure the driver had removed!!
                if (!is_wifi_driver_loaded()) {
                    ALOGD("Unload %s driver successfully!!\n",wlan_driver);
                    return 0;
                }
            } else if (strcmp(driver_status, "failed") == 0) {
                return -1;
            }
        }
        ALOGD("func %s count = %d ",__func__,count);
        usleep(200000);
    }
    ALOGD("Failed to unload  %s driver\n",wlan_driver);
    return -1;
}

int wifi_unload_driver() {
    return wifi_common_unload_driver(0);
}

int wifi_ap_unload_driver() {
    return wifi_common_unload_driver(1);
}

static int wifi_common_load_driver(int mode)
{
    ALOGD("enter fuc %s mode : = %s\n",__func__,mode ? "softap":"sta");
    if (!get_wifi_device_num())
        return -1;

    char wlan_driver[PROPERTY_VALUE_MAX] = {'\0'};
    property_get(WLAN_DRIVER,wlan_driver, NULL);
    char driver_scripts_prop[30] = "init.svc.loadwifi";
    char driver_status[PROPERTY_VALUE_MAX];
    char daemon_cmd[PROPERTY_VALUE_MAX * 2];

    struct device_info *curptr = get_device_by_name(wlan_driver);
    if (curptr != NULL) {
        which_device_loaded = curptr->order;
    } else {
        ALOGD("func %s Cannot find device with name!",__func__);
    }

// move this action to scripts for different driver vendor modify by themself
#if 0
    if (is_wifi_driver_loaded()) {
        return 0;
    }
#endif

    if (mode == 0) {
        strcpy(driver_scripts_prop,"init.svc.loadwifi");
        snprintf(daemon_cmd,sizeof(daemon_cmd),"loadwifi:%s",wlan_driver);
    } else if (mode == 1) {
        strcpy(driver_scripts_prop,"init.svc.loadapwifi");
        snprintf(daemon_cmd,sizeof(daemon_cmd),"loadapwifi:%s",wlan_driver);
    }
    //property_set(DRIVER_PROP_NAME,"");
    sched_yield();
    property_set("ctl.start", daemon_cmd);
    usleep(500*1000);

    int count = 100;
    while (count-- > 0) {
        if (property_get(driver_scripts_prop, driver_status, NULL)) {
            if (strcmp(driver_status, "stopped") == 0) {
                return 0;
            } else if (strcmp(driver_status, "failed") == 0) {
                return -1;
            }
        }
        ALOGD("func %s count = %d ",__func__,count);
        usleep(200000);
    }
    ALOGD("Time out loading driver!!\n");
    wifi_common_unload_driver(mode);
    return -1;
}

int wifi_load_driver() {
    return wifi_common_load_driver(0);
}

int wifi_ap_load_driver() {
    return wifi_common_load_driver(1);
}
// MStar Android Patch End

int ensure_entropy_file_exists()
{
    int ret;
    int destfd;

    ret = access(SUPP_ENTROPY_FILE, R_OK|W_OK);
    if ((ret == 0) || (errno == EACCES)) {
        if ((ret != 0) &&
            (chmod(SUPP_ENTROPY_FILE, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)) {
            ALOGE("Cannot set RW to \"%s\": %s", SUPP_ENTROPY_FILE, strerror(errno));
            return -1;
        }
        return 0;
    }
    destfd = TEMP_FAILURE_RETRY(open(SUPP_ENTROPY_FILE, O_CREAT|O_RDWR, 0660));
    if (destfd < 0) {
        ALOGE("Cannot create \"%s\": %s", SUPP_ENTROPY_FILE, strerror(errno));
        return -1;
    }

    if (TEMP_FAILURE_RETRY(write(destfd, dummy_key, sizeof(dummy_key))) != sizeof(dummy_key)) {
        ALOGE("Error writing \"%s\": %s", SUPP_ENTROPY_FILE, strerror(errno));
        close(destfd);
        return -1;
    }
    close(destfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(SUPP_ENTROPY_FILE, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
             SUPP_ENTROPY_FILE, strerror(errno));
        unlink(SUPP_ENTROPY_FILE);
        return -1;
    }

    if (chown(SUPP_ENTROPY_FILE, AID_SYSTEM, AID_WIFI) < 0) {
        ALOGE("Error changing group ownership of %s to %d: %s",
             SUPP_ENTROPY_FILE, AID_WIFI, strerror(errno));
        unlink(SUPP_ENTROPY_FILE);
        return -1;
    }
    return 0;
}

// MStar Android Patch Begin
int check_config(const char *config_file) {

    int srcfd;
    int nread;
    char ifc[PROPERTY_VALUE_MAX];
    char *pbuf;
    struct stat sb;

    if (stat(config_file, &sb) != 0)
        return -1;

    pbuf = malloc(sb.st_size + PROPERTY_VALUE_MAX);
    if (!pbuf)
        return -1;
    srcfd = TEMP_FAILURE_RETRY(open(config_file, O_RDONLY));
    if (srcfd < 0) {
        ALOGE("Cannot open \"%s\": %s", config_file, strerror(errno));
        free(pbuf);
        return -1;
    }
    nread = TEMP_FAILURE_RETRY(read(srcfd, pbuf, sb.st_size));
    close(srcfd);
    //ALOGE("pbuf ======> %s====>%d",pbuf,sb.st_size);
    if (nread < 0) {
        ALOGE("Cannot read \"%s\": %s", config_file, strerror(errno));
        free(pbuf);
        return -1;
    }

    if (strstr(pbuf,"ctrl_interface=") != '\0') {
        ALOGE("%s is all right",config_file);
        free(pbuf);
        return 0;
    } else {
        ALOGE("%s is broken,need to recopy",config_file);
        free(pbuf);
        return -1;
    }
}
// MStar Android Patch End

int update_ctrl_interface(const char *config_file) {

    int srcfd, destfd;
    int nread;
    char ifc[PROPERTY_VALUE_MAX];
    char *pbuf;
    char *sptr;
    struct stat sb;

    if (stat(config_file, &sb) != 0)
        return -1;

    pbuf = malloc(sb.st_size + PROPERTY_VALUE_MAX);
    if (!pbuf)
        return 0;
    srcfd = TEMP_FAILURE_RETRY(open(config_file, O_RDONLY));
    if (srcfd < 0) {
        ALOGE("Cannot open \"%s\": %s", config_file, strerror(errno));
        free(pbuf);
        return 0;
    }
    nread = TEMP_FAILURE_RETRY(read(srcfd, pbuf, sb.st_size));
    close(srcfd);
    if (nread < 0) {
        ALOGE("Cannot read \"%s\": %s", config_file, strerror(errno));
        free(pbuf);
        return 0;
    }

    if (!strcmp(config_file, SUPP_CONFIG_FILE)) {
        property_get("wifi.interface", ifc, WIFI_TEST_INTERFACE);
    } else {
        // MStar Android Patch Begin
        property_get("wifi.interface", ifc, WIFI_TEST_INTERFACE);
        // MStar Android Patch End
    }
    /*
     * if there is a "ctrl_interface=<value>" entry, re-write it ONLY if it is
     * NOT a directory.  The non-directory value option is an Android add-on
     * that allows the control interface to be exchanged through an environment
     * variable (initialized by the "init" program when it starts a service
     * with a "socket" option).
     *
     * The <value> is deemed to be a directory if the "DIR=" form is used or
     * the value begins with "/".
     */
    if ((sptr = strstr(pbuf, "ctrl_interface=")) &&
        (!strstr(pbuf, "ctrl_interface=DIR=")) &&
        (!strstr(pbuf, "ctrl_interface=/"))) {
        char *iptr = sptr + strlen("ctrl_interface=");
        int ilen = 0;
        int mlen = strlen(ifc);
        int nwrite;
        // MStar Android Patch Begin
        if ((strncmp(ifc, iptr, mlen) != 0) && (strncmp("/",iptr,1) != 0)) {
        // MStar Android Patch End
            ALOGE("ctrl_interface != %s", ifc);
            while (((ilen + (iptr - pbuf)) < nread) && (iptr[ilen] != '\n'))
                ilen++;
            mlen = ((ilen >= mlen) ? ilen : mlen) + 1;
            memmove(iptr + mlen, iptr + ilen + 1, nread - (iptr + ilen + 1 - pbuf));
            memset(iptr, '\n', mlen);
            memcpy(iptr, ifc, strlen(ifc));
            destfd = TEMP_FAILURE_RETRY(open(config_file, O_RDWR, 0660));
            if (destfd < 0) {
                ALOGE("Cannot update \"%s\": %s", config_file, strerror(errno));
                free(pbuf);
                return -1;
            }
            TEMP_FAILURE_RETRY(write(destfd, pbuf, nread + mlen - ilen -1));
            close(destfd);
        }
    }
    free(pbuf);
    return 0;
}

// MStar Android Patch Begin
int is_config_p2p(const char *config_file)
{
    const char *p2p = "p2p";
    if (strstr(config_file, p2p) != NULL) {
         return 1;
    }
         return 0;
}
// MStar Android Patch End

int ensure_config_file_exists(const char *config_file)
{
    // MStar Android Patch Begin
    if (is_config_p2p(config_file)) {
        return ensure_config_file(config_file, P2P_CONFIG_TEMPLATE);
    } else {
        return ensure_config_file(config_file, SUPP_CONFIG_TEMPLATE);
    }
    // MStar Android Patch End
}

// MStar Android Patch Begin
int ensure_config_file(const char *config_file, const char *config_template)
{
    char buf[2048];
    int srcfd, destfd;
    struct stat sb;
    int nread;
    int ret;

    ret = access(config_file, R_OK|W_OK);
    if ((ret == 0) || (errno == EACCES)) {
        if ((ret != 0) &&
            (chmod(config_file, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP) != 0)) {
            ALOGE("Cannot set RW to \"%s\": %s", config_file, strerror(errno));
            return -1;
        }
        /* return if filesize is at least 10 bytes */
        if (stat(config_file, &sb) == 0 && sb.st_size > 10) {
            return update_ctrl_interface(config_file);
        }
    } else if (errno != ENOENT) {
        ALOGE("Cannot access \"%s\": %s", config_file, strerror(errno));
        return -1;
    }

    srcfd = TEMP_FAILURE_RETRY(open(config_template, O_RDONLY));
    if (srcfd < 0) {
        ALOGE("Cannot open \"%s\": %s", config_template, strerror(errno));
        return -1;
    }

    destfd = TEMP_FAILURE_RETRY(open(config_file, O_CREAT|O_RDWR, 0660));
    if (destfd < 0) {
        close(srcfd);
        ALOGE("Cannot create \"%s\": %s", config_file, strerror(errno));
        return -1;
    }

    while ((nread = TEMP_FAILURE_RETRY(read(srcfd, buf, sizeof(buf)))) != 0) {
        if (nread < 0) {
            ALOGE("Error reading \"%s\": %s", config_template, strerror(errno));
            close(srcfd);
            close(destfd);
            unlink(config_file);
            return -1;
        }
        TEMP_FAILURE_RETRY(write(destfd, buf, nread));
    }

    close(destfd);
    close(srcfd);

    /* chmod is needed because open() didn't set permisions properly */
    if (chmod(config_file, 0660) < 0) {
        ALOGE("Error changing permissions of %s to 0660: %s",
             config_file, strerror(errno));
        unlink(config_file);
        return -1;
    }

    if (chown(config_file, AID_SYSTEM, AID_WIFI) < 0) {
        ALOGE("Error changing group ownership of %s to %d: %s",
             config_file, AID_WIFI, strerror(errno));
        unlink(config_file);
        return -1;
    }
    return update_ctrl_interface(config_file);
}
// MStar Android Patch End

int wifi_start_supplicant(int p2p_supported)
{
    // MStar Android Patch Begin
    ALOGD("enter func %s p2p_supported = %d\n",__func__,p2p_supported);
    char wlan_driver[PROPERTY_VALUE_MAX] = {'\0'};
    property_get(WLAN_DRIVER,wlan_driver, NULL);
    char daemon_cmd[PROPERTY_VALUE_MAX * 2];
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 200; /* wait at most 20 seconds for completion */
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    const prop_info *pi;
    unsigned serial = 0, i;
#endif

    if (p2p_supported) {
        //strcpy(supplicant_name, RTW_SUPPL_CON_NAME);
        //strcpy(supplicant_prop_name, RTW_PROP_CON_NAME);

        /* Ensure p2p config file is created */
        if (ensure_config_file_exists(P2P_CONFIG_FILE) < 0) {
            ALOGE("Failed to create a p2p config file");
            return -1;
        }

        if (check_config(P2P_CONFIG_FILE) != 0) {
            if (unlink(P2P_CONFIG_FILE) < 0) {
                ALOGE("Can not remove wpa_supplicant.conf");
                return -1;
            }
            if (ensure_config_file_exists(P2P_CONFIG_FILE) < 0) {
                ALOGE("Wi-Fi will not be enabled");
                return -1;
            }
        }
    }

    /* Check whether already running */
    if (property_get(supplicant_prop_name, supp_status, NULL)
            && strcmp(supp_status, "running") == 0) {
        return 0;
    }

    /* Before starting the daemon, make sure its config file exists */
    if (ensure_config_file_exists(SUPP_CONFIG_FILE) < 0) {
        ALOGE("Wi-Fi will not be enabled");
        return -1;
    }
    if (check_config(SUPP_CONFIG_FILE) != 0) {
        if (unlink(SUPP_CONFIG_FILE) < 0) {
            ALOGE("Can not remove wpa_supplicant.conf");
            return -1;
        }
        if (ensure_config_file_exists(SUPP_CONFIG_FILE) < 0) {
            ALOGE("Wi-Fi will not be enabled");
            return -1;
        }
    }

    if (ensure_entropy_file_exists() < 0) {
        ALOGE("Wi-Fi entropy file was not created");
    }

    /* Clear out any stale socket files that might be left over. */
    wpa_ctrl_cleanup();

    /* Reset sockets used for exiting from hung state */
    exit_sockets[0] = exit_sockets[1] = -1;

#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
    /*
     * Get a reference to the status property, so we can distinguish
     * the case where it goes stopped => running => stopped (i.e.,
     * it start up, but fails right away) from the case in which
     * it starts in the stopped state and never manages to start
     * running at all.
     */
    pi = __system_property_find(supplicant_prop_name);
    if (pi != NULL) {
        serial = __system_property_serial(pi);
    }
#endif
    property_get("wifi.interface", primary_iface, WIFI_TEST_INTERFACE);
    snprintf(daemon_cmd,sizeof(daemon_cmd),"%s:%s",supplicant_name,wlan_driver);
    property_set("ctl.start", daemon_cmd);

    sched_yield();

    while (count-- > 0) {
#ifdef HAVE_LIBC_SYSTEM_PROPERTIES
        if (pi == NULL) {
            pi = __system_property_find(supplicant_prop_name);
        }
        if (pi != NULL) {
            __system_property_read(pi, NULL, supp_status);
            if (strcmp(supp_status, "running") == 0) {
                return 0;
            } else if (__system_property_serial(pi) != serial &&
                    strcmp(supp_status, "stopped") == 0) {
                return -1;
            }
        }
#else
        if (property_get(supplicant_prop_name, supp_status, NULL)) {
            if (strcmp(supp_status, "running") == 0)
                return 0;
        }
#endif
        ALOGD("func %s count = %d ",__func__,count);
        usleep(100000);
    }
    // MStar Android Patch End
    return -1;
}

int wifi_stop_supplicant(int p2p_supported)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds for completion */

    // MStar Android Patch Begin
    /** supplicant name is already been set when start supplicant
     ** there is no need to set another time,otherwise it will make mistake

    if (p2p_supported) {
        strcpy(supplicant_name, P2P_SUPPLICANT_NAME);
        strcpy(supplicant_prop_name, P2P_PROP_NAME);
    } else {
        strcpy(supplicant_name, SUPPLICANT_NAME);
        strcpy(supplicant_prop_name, SUPP_PROP_NAME);
    }
    */
    // MStar Android Patch End
    /* Check whether supplicant already stopped */
    if (property_get(supplicant_prop_name, supp_status, NULL)
        && strcmp(supp_status, "stopped") == 0) {
        return 0;
    }

    property_set("ctl.stop", supplicant_name);
    sched_yield();

    while (count-- > 0) {
        if (property_get(supplicant_prop_name, supp_status, NULL)) {
            if (strcmp(supp_status, "stopped") == 0)
                return 0;
        }
        usleep(100000);
    }
    ALOGE("Failed to stop supplicant");
    return -1;
}

int wifi_connect_on_socket_path(const char *path)
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};

    /* Make sure supplicant is running */
    if (!property_get(supplicant_prop_name, supp_status, NULL)
            || strcmp(supp_status, "running") != 0) {
        ALOGE("Supplicant not running, cannot connect");
        return -1;
    }

    ctrl_conn = wpa_ctrl_open(path);
    if (ctrl_conn == NULL) {
        ALOGE("Unable to open connection to supplicant on \"%s\": %s",
             path, strerror(errno));
        return -1;
    }
    monitor_conn = wpa_ctrl_open(path);
    if (monitor_conn == NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
        return -1;
    }
    if (wpa_ctrl_attach(monitor_conn) != 0) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, exit_sockets) == -1) {
        wpa_ctrl_close(monitor_conn);
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = monitor_conn = NULL;
        return -1;
    }

    return 0;
}

/* Establishes the control and monitor socket connections on the interface */
int wifi_connect_to_supplicant()
{
    static char path[PATH_MAX];

    if (access(IFACE_DIR, F_OK) == 0) {
        snprintf(path, sizeof(path), "%s/%s", IFACE_DIR, primary_iface);
    } else {
        snprintf(path, sizeof(path), "@android:wpa_%s", primary_iface);
    }
    return wifi_connect_on_socket_path(path);
}

int wifi_send_command(const char *cmd, char *reply, size_t *reply_len)
{
    int ret;
    if (ctrl_conn == NULL) {
        ALOGV("Not connected to wpa_supplicant - \"%s\" command dropped.\n", cmd);
        return -1;
    }
    ret = wpa_ctrl_request(ctrl_conn, cmd, strlen(cmd), reply, reply_len, NULL);
    if (ret == -2) {
        ALOGD("'%s' command timed out.\n", cmd);
        /* unblocks the monitor receive socket for termination */
        TEMP_FAILURE_RETRY(write(exit_sockets[0], "T", 1));
        return -2;
    } else if (ret < 0 || strncmp(reply, "FAIL", 4) == 0) {
        return -1;
    }
    if (strncmp(cmd, "PING", 4) == 0) {
        reply[*reply_len] = '\0';
    }
    return 0;
}

int wifi_ctrl_recv(char *reply, size_t *reply_len)
{
    int res;
    int ctrlfd = wpa_ctrl_get_fd(monitor_conn);
    struct pollfd rfds[2];

    memset(rfds, 0, 2 * sizeof(struct pollfd));
    rfds[0].fd = ctrlfd;
    rfds[0].events |= POLLIN;
    rfds[1].fd = exit_sockets[1];
    rfds[1].events |= POLLIN;
    res = TEMP_FAILURE_RETRY(poll(rfds, 2, -1));
    if (res < 0) {
        ALOGE("Error poll = %d", res);
        return res;
    }
    if (rfds[0].revents & POLLIN) {
        return wpa_ctrl_recv(monitor_conn, reply, reply_len);
    }

    /* it is not rfds[0], then it must be rfts[1] (i.e. the exit socket)
     * or we timed out. In either case, this call has failed ..
     */
    return -2;
}

int wifi_wait_on_socket(char *buf, size_t buflen)
{
    size_t nread = buflen - 1;
    int fd;
    fd_set rfds;
    int result;
    char *match, *match2;
    struct timeval tval;
    struct timeval *tptr;

    if (monitor_conn == NULL) {
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - connection closed");
    }

    result = wifi_ctrl_recv(buf, &nread);

    /* Terminate reception on exit socket */
    if (result == -2) {
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - connection closed");
    }

    if (result < 0) {
        ALOGD("wifi_ctrl_recv failed: %s\n", strerror(errno));
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - recv error");
    }
    buf[nread] = '\0';
    /* Check for EOF on the socket */
    if (result == 0 && nread == 0) {
        /* Fabricate an event to pass up */
        ALOGD("Received EOF on supplicant socket\n");
        return snprintf(buf, buflen, WPA_EVENT_TERMINATING " - signal 0 received");
    }
    /*
     * Events strings are in the format
     *
     *     IFNAME=iface <N>CTRL-EVENT-XXX 
     *        or
     *     <N>CTRL-EVENT-XXX 
     *
     * where N is the message level in numerical form (0=VERBOSE, 1=DEBUG,
     * etc.) and XXX is the event name. The level information is not useful
     * to us, so strip it off.
     */

    if (strncmp(buf, IFNAME, IFNAMELEN) == 0) {
        match = strchr(buf, ' ');
        if (match != NULL) {
            if (match[1] == '<') {
                match2 = strchr(match + 2, '>');
                if (match2 != NULL) {
                    nread -= (match2 - match);
                    memmove(match + 1, match2 + 1, nread - (match - buf) + 1);
                }
            }
        } else {
            return snprintf(buf, buflen, "%s", WPA_EVENT_IGNORE);
        }
    } else if (buf[0] == '<') {
        match = strchr(buf, '>');
        if (match != NULL) {
            nread -= (match + 1 - buf);
            memmove(buf, match + 1, nread + 1);
            ALOGV("supplicant generated event without interface - %s\n", buf);
        }
    } else {
        /* let the event go as is! */
        ALOGW("supplicant generated event without interface and without message level - %s\n", buf);
    }

    return nread;
}

int wifi_wait_for_event(char *buf, size_t buflen)
{
    return wifi_wait_on_socket(buf, buflen);
}

void wifi_close_sockets()
{
    if (ctrl_conn != NULL) {
        wpa_ctrl_close(ctrl_conn);
        ctrl_conn = NULL;
    }

    if (monitor_conn != NULL) {
        wpa_ctrl_close(monitor_conn);
        monitor_conn = NULL;
    }

    if (exit_sockets[0] >= 0) {
        close(exit_sockets[0]);
        exit_sockets[0] = -1;
    }

    if (exit_sockets[1] >= 0) {
        close(exit_sockets[1]);
        exit_sockets[1] = -1;
    }
}

void wifi_close_supplicant_connection()
{
    char supp_status[PROPERTY_VALUE_MAX] = {'\0'};
    int count = 50; /* wait at most 5 seconds to ensure init has stopped stupplicant */

    wifi_close_sockets();

    while (count-- > 0) {
        if (property_get(supplicant_prop_name, supp_status, NULL)) {
            if (strcmp(supp_status, "stopped") == 0)
                return;
        }
        usleep(100000);
    }
}

int wifi_command(const char *command, char *reply, size_t *reply_len)
{
    return wifi_send_command(command, reply, reply_len);
}

const char *wifi_get_fw_path(int fw_type)
{
    switch (fw_type) {
    case WIFI_GET_FW_PATH_STA:
        return WIFI_DRIVER_FW_PATH_STA;
    case WIFI_GET_FW_PATH_AP:
        return WIFI_DRIVER_FW_PATH_AP;
    case WIFI_GET_FW_PATH_P2P:
        return WIFI_DRIVER_FW_PATH_P2P;
    }
    return NULL;
}


// MStar Android Patch Begin
int is_wifi_driver_module_loaded(const char * driver_module) {
    char driver_status[PROPERTY_VALUE_MAX];
    FILE *proc;
    char line[30];

    if ((proc = fopen(MODULE_FILE, "r")) == NULL) {
        ALOGW("Could not open %s: %s", MODULE_FILE, strerror(errno));
        return 0;
    }
    while ((fgets(line, sizeof(line), proc)) != NULL) {
        if (strncmp(line,driver_module,strlen(driver_module)) == 0) {
            fclose(proc);
            return 1;
        }
    }
    fclose(proc);
    return 0;
}

// MStar Android Patch End

int wifi_change_fw_path(const char *fwpath)
{
    int len;
    int fd;
    int ret = 0;

    return ret;
}
