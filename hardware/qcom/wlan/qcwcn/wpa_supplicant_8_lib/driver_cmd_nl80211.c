/*
 * Driver interaction with extended Linux CFG8021
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 */

#include "includes.h"
#include <sys/types.h>
#include <fcntl.h>
#include <net/if.h>
#include <netlink/object-api.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <linux/pkt_sched.h>

#include "common.h"
#include "linux_ioctl.h"
#include "driver_nl80211.h"
#include "wpa_supplicant_i.h"
#include "config.h"
#ifdef LINUX_EMBEDDED
#include <sys/ioctl.h>
#endif
#if defined(ANDROID) || defined(LINUX_EMBEDDED)
#include "android_drv.h"
#endif
#include "driver_cmd_nl80211_extn.h"

#define WPA_PS_ENABLED		0
#define WPA_PS_DISABLED		1
#define UNUSED(x)	(void)(x)

#define TWT_SETUP_WAKE_INTVL_MANTISSA_MAX       0xFFFF
#define TWT_SETUP_WAKE_DURATION_MAX             0xFF
#define TWT_SETUP_WAKE_INTVL_EXP_MAX            31
#define TWT_WAKE_INTERVAL_TU_FACTOR		1024

#define TWT_SETUP_STR        "twt_session_setup"
#define TWT_TERMINATE_STR    "twt_session_terminate"
#define TWT_PAUSE_STR        "twt_session_pause"
#define TWT_RESUME_STR       "twt_session_resume"
#define TWT_NUDGE_STR        "twt_session_nudge"
#define TWT_GET_PARAMS_STR   "twt_session_get_params"
#define TWT_GET_STATS_STR    "twt_session_get_stats"
#define TWT_CLEAR_STATS_STR  "twt_session_clear_stats"
#define TWT_GET_CAP_STR      "twt_get_capability"

#define TWT_SETUP_STRLEN         strlen(TWT_SETUP_STR)
#define TWT_TERMINATE_STR_LEN    strlen(TWT_TERMINATE_STR)
#define TWT_PAUSE_STR_LEN        strlen(TWT_PAUSE_STR)
#define TWT_RESUME_STR_LEN       strlen(TWT_RESUME_STR)
#define TWT_NUDGE_STR_LEN        strlen(TWT_NUDGE_STR)
#define TWT_GET_PARAMS_STR_LEN   strlen(TWT_GET_PARAMS_STR)
#define TWT_GET_STATS_STR_LEN    strlen(TWT_GET_STATS_STR)
#define TWT_CLEAR_STATS_STR_LEN  strlen(TWT_CLEAR_STATS_STR)
#define TWT_GET_CAP_STR_LEN      strlen(TWT_GET_CAP_STR)

#define TWT_CMD_NOT_EXIST -EINVAL
#define DEFAULT_IFNAME "wlan0"
#define TWT_RESP_BUF_LEN 512

#define SINGLE_SPACE_LEN 1
#define SINGLE_DIGIT_LEN 1

#define DIALOG_ID_STR           "dialog_id"
#define REQ_TYPE_STR            "req_type"
#define TRIG_TYPE_STR           "trig_type"
#define FLOW_TYPE_STR           "flow_type"
#define WAKE_INTR_EXP_STR       "wake_intr_exp"
#define PROTECTION_STR          "protection"
#define WAKE_TIME_STR           "wake_time"
#define WAKE_DUR_STR            "wake_dur"
#define WAKE_INTR_MANTISSA_STR  "wake_intr_mantissa"
#define BROADCAST_STR           "broadcast"
#define MIN_WAKE_INTVL_STR      "min_wake_intvl"
#define MAX_WAKE_INTVL_STR      "max_wake_intvl"
#define MIN_WAKE_DUR_STR        "min_wake_duration"
#define MAX_WAKE_DUR_STR        "max_wake_duration"
#define NEXT_TWT_STR            "next_twt"
#define NEXT2_TWT_STR           "next2_twt"
#define NEXT_TWT_SIZE_STR       "next_twt_size"
#define PAUSE_DURATION_STR      "pause_duration"

#define DIALOG_ID_STR_LEN               strlen(DIALOG_ID_STR)
#define REQ_TYPE_STR_LEN                strlen(REQ_TYPE_STR)
#define TRIG_TYPE_STR_LEN               strlen(TRIG_TYPE_STR)
#define FLOW_TYPE_STR_LEN               strlen(FLOW_TYPE_STR)
#define WAKE_INTR_EXP_STR_LEN           strlen(WAKE_INTR_EXP_STR)
#define PROTECTION_STR_LEN              strlen(PROTECTION_STR)
#define WAKE_TIME_STR_LEN               strlen(WAKE_TIME_STR)
#define WAKE_DUR_STR_LEN                strlen(WAKE_DUR_STR)
#define WAKE_INTR_MANTISSA_STR_LEN      strlen(WAKE_INTR_MANTISSA_STR)
#define BROADCAST_STR_LEN               strlen(BROADCAST_STR)
#define MIN_WAKE_INTVL_STR_LEN          strlen(MIN_WAKE_INTVL_STR)
#define MAX_WAKE_INTVL_STR_LEN          strlen(MAX_WAKE_INTVL_STR)
#define MIN_WAKE_DUR_STR_LEN            strlen(MIN_WAKE_DUR_STR)
#define MAX_WAKE_DUR_STR_LEN            strlen(MAX_WAKE_DUR_STR)
#define NEXT_TWT_STR_LEN		strlen(NEXT_TWT_STR)
#define NEXT2_TWT_STR_LEN		strlen(NEXT2_TWT_STR)
#define NEXT_TWT_SIZE_STR_LEN		strlen(NEXT_TWT_SIZE_STR)
#define PAUSE_DURATION_STR_LEN          strlen(PAUSE_DURATION_STR)

#define MAC_ADDR_STR "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC_ADDR_ARRAY(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]

#define TWT_CTRL_EVENT           "CTRL-EVENT-TWT"
#define TWT_SETUP_RESP           "CTRL-EVENT-TWT SETUP"
#define TWT_TEARDOWN_RESP        "CTRL-EVENT-TWT TERMINATE"
#define TWT_PAUSE_RESP           "CTRL-EVENT-TWT PAUSE"
#define TWT_RESUME_RESP          "CTRL-EVENT-TWT RESUME"
#define TWT_NOTIFY_RESP          "CTRL-EVENT-TWT NOTIFY"
#define TWT_SETUP_RESP_LEN       strlen(TWT_SETUP_RESP)
#define TWT_TEARDOWN_RESP_LEN    strlen(TWT_TEARDOWN_RESP)
#define TWT_PAUSE_RESP_LEN       strlen(TWT_PAUSE_RESP)
#define TWT_RESUME_RESP_LEN      strlen(TWT_RESUME_RESP)
#define TWT_NOTIFY_RESP_LEN      strlen(TWT_NOTIFY_RESP)

#define NL80211_ATTR_MAX_INTERNAL 256

static int twt_async_support = -1;

struct twt_setup_parameters {
	u8 dialog_id;
	u8 req_type;
	u8 trig_type;
	u8 flow_type;
	u8 wake_intr_exp;
	u8 protection;
	u32 wake_time;
	u32 wake_dur;
	u32 wake_intr_mantissa;
	u8 bcast;
	u32 min_wake_intvl;
	u32 max_wake_intvl;
	u32 min_wake_duration;
	u32 max_wake_duration;
};

struct twt_resume_parameters {
	u8 dialog_id;
	u8 next_twt;
	u32 next2_twt;
	u32 next_twt_size;
};

struct twt_nudge_parameters {
	u8 dialog_id;
	u32 wake_time;
	u32 next_twt_size;
};

struct twt_resp_info {
	char *reply_buf;
	int reply_buf_len;
	enum qca_wlan_twt_operation twt_oper;
	struct wpa_driver_nl80211_data *drv;
};

/* Return type for setBand*/
enum {
	SEND_CHANNEL_CHANGE_EVENT = 0,
	DO_NOT_SEND_CHANNEL_CHANGE_EVENT,
};

typedef struct android_wifi_priv_cmd {
	char *buf;
	int used_len;
	int total_len;
} android_wifi_priv_cmd;

static int drv_errors = 0;

static void wpa_driver_notify_country_change(void *ctx, char *cmd)
{
	if ((os_strncasecmp(cmd, "COUNTRY", 7) == 0) ||
	    (os_strncasecmp(cmd, "SETBAND", 7) == 0)) {
		union wpa_event_data event;

		os_memset(&event, 0, sizeof(event));
		event.channel_list_changed.initiator = REGDOM_SET_BY_USER;
		if (os_strncasecmp(cmd, "COUNTRY", 7) == 0) {
			event.channel_list_changed.type = REGDOM_TYPE_COUNTRY;
			if (os_strlen(cmd) > 9) {
				event.channel_list_changed.alpha2[0] = cmd[8];
				event.channel_list_changed.alpha2[1] = cmd[9];
			}
		} else {
			event.channel_list_changed.type = REGDOM_TYPE_UNKNOWN;
		}
		wpa_supplicant_event(ctx, EVENT_CHANNEL_LIST_CHANGED, &event);
	}
}

/**
 * twt_status_to_string() - Mapping twt response status into string
 * @status: enum qca_wlan_vendor_twt_status values
 *
 * This function will map the twt response status into corresponding
 * string.
 *
 * Return: pointer to the string
 */
static const char *twt_status_to_string(enum qca_wlan_vendor_twt_status status)
{
	switch (status) {
	case QCA_WLAN_VENDOR_TWT_STATUS_OK:
		return "QCA_WLAN_VENDOR_TWT_STATUS_OK";
	case QCA_WLAN_VENDOR_TWT_STATUS_TWT_NOT_ENABLED:
		return "QCA_WLAN_VENDOR_TWT_STATUS_TWT_NOT_ENABLED";
	case QCA_WLAN_VENDOR_TWT_STATUS_USED_DIALOG_ID:
		return "QCA_WLAN_VENDOR_TWT_STATUS_USED_DIALOG_ID";
	case QCA_WLAN_VENDOR_TWT_STATUS_SESSION_BUSY:
		return "QCA_WLAN_VENDOR_TWT_STATUS_SESSION_BUSY";
	case QCA_WLAN_VENDOR_TWT_STATUS_SESSION_NOT_EXIST:
		return "QCA_WLAN_VENDOR_TWT_STATUS_SESSION_NOT_EXIST";
	case QCA_WLAN_VENDOR_TWT_STATUS_NOT_SUSPENDED:
		return "QCA_WLAN_VENDOR_TWT_STATUS_NOT_SUSPENDED";
	case QCA_WLAN_VENDOR_TWT_STATUS_INVALID_PARAM:
		return "QCA_WLAN_VENDOR_TWT_STATUS_INVALID_PARAM";
	case QCA_WLAN_VENDOR_TWT_STATUS_NOT_READY:
		return "QCA_WLAN_VENDOR_TWT_STATUS_NOT_READY";
	case QCA_WLAN_VENDOR_TWT_STATUS_NO_RESOURCE:
		return "QCA_WLAN_VENDOR_TWT_STATUS_NO_RESOURCE";
	case QCA_WLAN_VENDOR_TWT_STATUS_NO_ACK:
		return "QCA_WLAN_VENDOR_TWT_STATUS_NO_ACK";
	case QCA_WLAN_VENDOR_TWT_STATUS_NO_RESPONSE:
		return "QCA_WLAN_VENDOR_TWT_STATUS_NO_RESPONSE";
	case QCA_WLAN_VENDOR_TWT_STATUS_DENIED:
		return "QCA_WLAN_VENDOR_TWT_STATUS_DENIED";
	case QCA_WLAN_VENDOR_TWT_STATUS_UNKNOWN_ERROR:
		return "QCA_WLAN_VENDOR_TWT_STATUS_UNKNOWN_ERROR";
	case QCA_WLAN_VENDOR_TWT_STATUS_ALREADY_SUSPENDED:
		return "QCA_WLAN_VENDOR_TWT_STATUS_ALREADY_SUSPENDED";
	case QCA_WLAN_VENDOR_TWT_STATUS_IE_INVALID:
	       return "QCA_WLAN_VENDOR_TWT_STATUS_IE_INVALID";
	case QCA_WLAN_VENDOR_TWT_STATUS_PARAMS_NOT_IN_RANGE:
		return "QCA_WLAN_VENDOR_TWT_STATUS_PARAMS_NOT_IN_RANGE";
	case QCA_WLAN_VENDOR_TWT_STATUS_PEER_INITIATED_TERMINATE:
		return "QCA_WLAN_VENDOR_TWT_STATUS_PEER_INITIATED_TERMINATE";
	case QCA_WLAN_VENDOR_TWT_STATUS_ROAM_INITIATED_TERMINATE:
		return "QCA_WLAN_VENDOR_TWT_STATUS_ROAM_INITIATED_TERMINATE";
	default:
		return "INVALID TWT STATUS";
	}
}

static int check_for_twt_cmd(char **cmd)
{
	if (os_strncasecmp(*cmd, TWT_SETUP_STR, TWT_SETUP_STRLEN) == 0) {
		*cmd += (TWT_SETUP_STRLEN + 1);
		return QCA_WLAN_TWT_SET;
	} else if (os_strncasecmp(*cmd, TWT_TERMINATE_STR,
				  TWT_TERMINATE_STR_LEN) == 0) {
		*cmd += (TWT_TERMINATE_STR_LEN + 1);
		return QCA_WLAN_TWT_TERMINATE;
	} else if (os_strncasecmp(*cmd, TWT_PAUSE_STR, TWT_PAUSE_STR_LEN) == 0) {
		*cmd += (TWT_PAUSE_STR_LEN + 1);
		return QCA_WLAN_TWT_SUSPEND;
	} else if (os_strncasecmp(*cmd, TWT_RESUME_STR, TWT_RESUME_STR_LEN) == 0) {
		*cmd += (TWT_RESUME_STR_LEN + 1);
		return QCA_WLAN_TWT_RESUME;
	} else if (os_strncasecmp(*cmd, TWT_GET_PARAMS_STR,
				  TWT_GET_PARAMS_STR_LEN) == 0) {
		*cmd += (TWT_GET_PARAMS_STR_LEN + 1);
		return QCA_WLAN_TWT_GET;
	} else if (os_strncasecmp(*cmd, TWT_NUDGE_STR,
				  TWT_NUDGE_STR_LEN) == 0) {
		*cmd += (TWT_NUDGE_STR_LEN + 1);
		return QCA_WLAN_TWT_NUDGE;
	} else if (os_strncasecmp(*cmd, TWT_GET_STATS_STR,
				  TWT_GET_STATS_STR_LEN) == 0) {
		*cmd += (TWT_GET_STATS_STR_LEN + 1);
		return QCA_WLAN_TWT_GET_STATS;
	} else if (os_strncasecmp(*cmd, TWT_CLEAR_STATS_STR,
				  TWT_CLEAR_STATS_STR_LEN) == 0) {
		*cmd += (TWT_CLEAR_STATS_STR_LEN + 1);
		return QCA_WLAN_TWT_CLEAR_STATS;
	} else if (os_strncasecmp(*cmd, TWT_GET_CAP_STR,
				  TWT_GET_CAP_STR_LEN) == 0) {
		*cmd += (TWT_GET_CAP_STR_LEN + 1);
		return QCA_WLAN_TWT_GET_CAPABILITIES;
	} else {
		wpa_printf(MSG_DEBUG, "Not a TWT command");
		return TWT_CMD_NOT_EXIST;
	}

	wpa_printf(MSG_DEBUG, "Not a TWT command");
	return TWT_CMD_NOT_EXIST;
}

static int pack_nlmsg_vendor_hdr(struct nl_msg *drv_nl_msg,
				 struct wpa_driver_nl80211_data *drv,
				 char *ifname)
{
	int ret;
	int ifindex;

	genlmsg_put(drv_nl_msg, NL_AUTO_PORT, NL_AUTO_SEQ,
		    drv->global->nl80211_id, 0, 0,
		    NL80211_CMD_VENDOR, 0);

	ret = nla_put_u32(drv_nl_msg, NL80211_ATTR_VENDOR_ID, OUI_QCA);
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Failed to put vendor id");
		return ret;
	}

	ret = nla_put_u32(drv_nl_msg, NL80211_ATTR_VENDOR_SUBCMD,
			  QCA_NL80211_VENDOR_SUBCMD_CONFIG_TWT);
	if (ret < 0) {
		wpa_printf(MSG_DEBUG, "nl put twt vendor subcmd failed");
		return ret;
	}

	if (ifname && (strlen(ifname) > 0))
		ifindex = if_nametoindex(ifname);
	else
		ifindex = if_nametoindex(DEFAULT_IFNAME);

	ret = nla_put_u32(drv_nl_msg, NL80211_ATTR_IFINDEX, ifindex);
	if (ret < 0) {
		wpa_printf(MSG_DEBUG, "nl put iface: %s failed", ifname);
		return ret;
	}
	return ret;
}

static u32 get_u32_from_string(char *cmd_string, int *ret)
{
	u32 val = 0;
	char *cmd = cmd_string;

	while (*cmd != ' ')
		cmd++;

	*ret = 0;
	errno = 0;
	val = strtol(cmd_string, NULL, 10);
	if (errno == ERANGE || (errno != 0 && val == 0)) {
		wpa_printf(MSG_ERROR, "invalid value");
		*ret = -EINVAL;
        }
	return val;
}

static u8 get_u8_from_string(char *cmd_string, int *ret)
{
	char *cmd = cmd_string;
	u8 val = 0;

	while (*cmd != ' ')
		cmd++;

	*ret = 0;
	errno = 0;
	val = strtol(cmd_string, NULL, 10) & 0xFF;
	if (errno == ERANGE || (errno != 0 && val == 0)) {
		wpa_printf(MSG_ERROR, "invalid value");
		*ret = -EINVAL;
        }
	return val;
}

char *move_to_next_str(char *cmd)
{
	while (*cmd != ' ')
		cmd++;

	while (*cmd == ' ')
		cmd++;

	return cmd;
}

static int is_binary(u8 value) {
	if(value == 0 || value == 1)
		return 0;
	return -1;
}

static
void print_setup_cmd_values(struct twt_setup_parameters *twt_setup_params)
{
	wpa_printf(MSG_DEBUG, "TWT: setup dialog_id: %x",
		   twt_setup_params->dialog_id);
	wpa_printf(MSG_DEBUG, "TWT: setup req type: %d ",
		   twt_setup_params->req_type);
	wpa_printf(MSG_DEBUG, "TWT: setup trig type: %d ",
		   twt_setup_params->trig_type);
	wpa_printf(MSG_DEBUG, "TWT: setup flow type: 0x%x",
		   twt_setup_params->flow_type);
	wpa_printf(MSG_DEBUG, "TWT: setup wake exp: 0x%x",
		   twt_setup_params->wake_intr_exp);
	wpa_printf(MSG_DEBUG, "TWT: setup protection: 0x%x",
		   twt_setup_params->protection);
	wpa_printf(MSG_DEBUG, "TWT: setup wake time: 0x%x",
		   twt_setup_params->wake_time);
	wpa_printf(MSG_DEBUG, "TWT: setup wake dur: 0x%x",
		   twt_setup_params->wake_dur);
	wpa_printf(MSG_DEBUG, "TWT: setup wake intr mantissa: 0x%x",
		   twt_setup_params->wake_intr_mantissa);
	wpa_printf(MSG_DEBUG, "TWT: setup bcast: %d ",
		   twt_setup_params->bcast);
	wpa_printf(MSG_DEBUG, "TWT: min wake intvl: %d ",
		   twt_setup_params->min_wake_intvl);
	wpa_printf(MSG_DEBUG, "TWT: max wake intvl: %d ",
		   twt_setup_params->max_wake_intvl);
	wpa_printf(MSG_DEBUG, "TWT: min wake duration: %d ",
		   twt_setup_params->min_wake_duration);
	wpa_printf(MSG_DEBUG, "TWT: max wake duration: %d ",
		   twt_setup_params->max_wake_duration);
}

static int check_cmd_input(char *cmd_string)
{
	u32 cmd_string_len;

	if (!cmd_string) {
		wpa_printf(MSG_ERROR, "cmd string null");
		return -EINVAL;
	}

	cmd_string_len = strlen(cmd_string);

	wpa_printf(MSG_DEBUG, "TWT: cmd string - %s len = %u", cmd_string,
		   cmd_string_len);
	if (cmd_string_len < DIALOG_ID_STR_LEN + SINGLE_SPACE_LEN +
			     SINGLE_DIGIT_LEN) {
		wpa_printf(MSG_ERROR, "TWT: Dialog_id parameter missing");
		return -EINVAL;
	}

	return 0;
}

static
int process_twt_setup_cmd_string(char *cmd,
				 struct twt_setup_parameters *twt_setup_params)
{
	int ret = 0;

	if (!twt_setup_params) {
		wpa_printf(MSG_ERROR, "cmd or twt_setup_params null");
		return -EINVAL;
	}

	if (check_cmd_input(cmd))
		return -EINVAL;

	wpa_printf(MSG_DEBUG, "process twt setup command string: %s", cmd);
	while (*cmd == ' ')
		cmd++;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) == 0) {
		cmd += (DIALOG_ID_STR_LEN + 1);
		twt_setup_params->dialog_id = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	if (os_strncasecmp(cmd, REQ_TYPE_STR, REQ_TYPE_STR_LEN) == 0) {
		cmd += (REQ_TYPE_STR_LEN + 1);
		twt_setup_params->req_type = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	if (os_strncasecmp(cmd, TRIG_TYPE_STR, TRIG_TYPE_STR_LEN) == 0) {
		cmd += (TRIG_TYPE_STR_LEN + 1);
		twt_setup_params->trig_type = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (is_binary(twt_setup_params->trig_type)) {
			wpa_printf(MSG_ERROR, "Invalid trigger type");
			return -EINVAL;
		}
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, FLOW_TYPE_STR, FLOW_TYPE_STR_LEN) == 0) {
		cmd += (FLOW_TYPE_STR_LEN + 1);
		twt_setup_params->flow_type = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (is_binary(twt_setup_params->flow_type)) {
			wpa_printf(MSG_ERROR, "Invalid flow type");
			return -EINVAL;
		}
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, WAKE_INTR_EXP_STR, WAKE_INTR_EXP_STR_LEN) == 0) {
		cmd += (WAKE_INTR_EXP_STR_LEN + 1);
		twt_setup_params->wake_intr_exp = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (twt_setup_params->wake_intr_exp >
		    TWT_SETUP_WAKE_INTVL_EXP_MAX) {
			wpa_printf(MSG_DEBUG, "Invalid wake_intr_exp %u",
				   twt_setup_params->wake_intr_exp);
			return -EINVAL;
		}
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, PROTECTION_STR, PROTECTION_STR_LEN) == 0) {
		cmd += (PROTECTION_STR_LEN + 1);
		twt_setup_params->protection = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (is_binary(twt_setup_params->protection)) {
			wpa_printf(MSG_ERROR, "Invalid protection value");
			return -EINVAL;
		}
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, WAKE_TIME_STR, WAKE_TIME_STR_LEN) == 0) {
		cmd += (WAKE_TIME_STR_LEN + 1);
		twt_setup_params->wake_time = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, WAKE_DUR_STR, WAKE_DUR_STR_LEN) == 0) {
		cmd += (WAKE_DUR_STR_LEN + 1);
		twt_setup_params->wake_dur = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (twt_setup_params->wake_dur == 0 ||
		    twt_setup_params->wake_dur > TWT_SETUP_WAKE_DURATION_MAX) {
			wpa_printf(MSG_ERROR, "Invalid wake_dura_us %u",
				   twt_setup_params->wake_dur);
			return -EINVAL;
		}

		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, WAKE_INTR_MANTISSA_STR,
		    WAKE_INTR_MANTISSA_STR_LEN) == 0) {
		cmd += (WAKE_INTR_MANTISSA_STR_LEN + 1);
		twt_setup_params->wake_intr_mantissa = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		if (twt_setup_params->wake_intr_mantissa >
		    TWT_SETUP_WAKE_INTVL_MANTISSA_MAX) {
			wpa_printf(MSG_ERROR, "Invalid wake_intr_mantissa %u",
				   twt_setup_params->wake_intr_mantissa);
			return -EINVAL;
		}

		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, BROADCAST_STR, BROADCAST_STR_LEN) == 0) {
		cmd += (BROADCAST_STR_LEN + 1);
		twt_setup_params->bcast = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (is_binary(twt_setup_params->bcast)) {
			wpa_printf(MSG_ERROR, "Invalid broadcast value");
			return -EINVAL;
		}
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, MIN_WAKE_INTVL_STR, MIN_WAKE_INTVL_STR_LEN) == 0) {
		cmd += (MIN_WAKE_INTVL_STR_LEN + 1);
		twt_setup_params->min_wake_intvl = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, MAX_WAKE_INTVL_STR, MAX_WAKE_INTVL_STR_LEN) == 0) {
		cmd += (MAX_WAKE_INTVL_STR_LEN + 1);
		twt_setup_params->max_wake_intvl = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, MIN_WAKE_DUR_STR, MIN_WAKE_DUR_STR_LEN) == 0) {
		cmd += (MIN_WAKE_DUR_STR_LEN + 1);
		twt_setup_params->min_wake_duration = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	if (strncmp(cmd, MAX_WAKE_DUR_STR, MAX_WAKE_DUR_STR_LEN) == 0) {
		cmd += (MAX_WAKE_DUR_STR_LEN + 1);
		twt_setup_params->max_wake_duration = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		cmd = move_to_next_str(cmd);
	}

	print_setup_cmd_values(twt_setup_params);

	return 0;
}

static
int prepare_twt_setup_nlmsg(struct nl_msg *nlmsg,
			    struct twt_setup_parameters *twt_setup_params)
{
	struct nlattr *twt_attr;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_SET)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_SET");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				  QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (twt_attr == NULL)
		goto fail;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID,
		       twt_setup_params->dialog_id)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
		goto fail;
	}

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_REQ_TYPE,
		       twt_setup_params->req_type)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put req type");
		goto fail;
	}

	if (twt_setup_params->trig_type) {
		if (nla_put_flag(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_TRIGGER)
				 ) {
			wpa_printf(MSG_DEBUG, "TWT: Failed to put trig type");
			goto fail;
		}
	}

	/*0 - Announced/ 1 - Unannounced*/
	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_TYPE,
		       twt_setup_params->flow_type)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put flow type");
		goto fail;
	}

	if (nla_put_u8(nlmsg,
		       QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL_EXP,
		       twt_setup_params->wake_intr_exp)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put wake exp");
		goto fail;
	}

	if (twt_setup_params->protection) {
		if (nla_put_flag(nlmsg,
		    QCA_WLAN_VENDOR_ATTR_TWT_SETUP_PROTECTION)) {
			wpa_printf(MSG_DEBUG,
				   "TWT: Failed to add protection");
			goto fail;
		}
	}

	/*offset to add with TBTT after which 1st SP will start*/
	if (nla_put_u32(nlmsg,
			QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_TIME,
			twt_setup_params->wake_time)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put wake time");
		goto fail;
	}

	/*TWT Wake Duration in units of us, must be <= 65280*/
	if (nla_put_u32(nlmsg,
			QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_DURATION,
			twt_setup_params->wake_dur)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put wake dur");
		goto fail;
	}

	if (nla_put_u32(nlmsg,
		QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL2_MANTISSA,
		twt_setup_params->wake_intr_mantissa)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put wake intr mantissa");
		goto fail;
	}

	if (nla_put_u32(nlmsg,
		QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL_MANTISSA,
		twt_setup_params->wake_intr_mantissa/TWT_WAKE_INTERVAL_TU_FACTOR)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put wake intr mantissa");
		goto fail;
	}

	if (twt_setup_params->bcast) {
		if (nla_put_flag(nlmsg,
			QCA_WLAN_VENDOR_ATTR_TWT_SETUP_BCAST)) {
			wpa_printf(MSG_DEBUG, "TWT: Failed to put bcast");
			goto fail;
		}
	}

	if (nla_put_u32(nlmsg,
		QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MIN_WAKE_INTVL,
		twt_setup_params->min_wake_intvl)) {
		wpa_printf(MSG_ERROR, "TWT: Failed to put min wake intr ");
		goto fail;
	}

	if (nla_put_u32(nlmsg,
		QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX_WAKE_INTVL,
		twt_setup_params->max_wake_intvl)) {
		wpa_printf(MSG_ERROR,"TWT: Failed to put max wake intr");
		goto fail;
	}

	if (nla_put_u32(nlmsg,
		QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MIN_WAKE_DURATION,
		twt_setup_params->min_wake_duration)) {
		wpa_printf(MSG_ERROR,"TWT: Failed to put min wake dur");
		goto fail;
	}

	if (nla_put_u32(nlmsg,
		QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX_WAKE_DURATION,
		twt_setup_params->max_wake_duration)) {
		wpa_printf(MSG_ERROR,"TWT: Failed to put max wake dur");
		goto fail;
	}

	nla_nest_end(nlmsg, twt_attr);
	wpa_printf(MSG_DEBUG, "TWT: setup command nla end");
	return 0;

fail:
	return -EINVAL;
}

static int prepare_twt_terminate_nlmsg(struct nl_msg *nlmsg, char *cmd)
{
	u8 dialog_id;
	struct nlattr *twt_attr;
	int ret = 0;

	if(check_cmd_input(cmd))
		return -EINVAL;

	while(*cmd == ' ')
		cmd++;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) == 0) {
		cmd += (DIALOG_ID_STR_LEN + 1);
		dialog_id = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
	} else {
		wpa_printf(MSG_ERROR, "TWT: no dialog_id found");
		goto fail;
	}

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_TERMINATE)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_TERMINATE");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				  QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (twt_attr == NULL)
		goto fail;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID,
		       dialog_id)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
		goto fail;
	}

	nla_nest_end(nlmsg, twt_attr);
	wpa_printf(MSG_DEBUG, "TWT: terminate sent with dialog_id: %x",
		   dialog_id);

	return 0;
fail:
	return -EINVAL;
}

static int prepare_twt_pause_nlmsg(struct nl_msg *nlmsg, char *cmd)
{
	u8 dialog_id;
	struct nlattr *twt_attr;
	int ret = 0;

	if(check_cmd_input(cmd))
		return -EINVAL;

	while(*cmd == ' ')
		cmd++;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) == 0) {
		cmd += (DIALOG_ID_STR_LEN + 1);
		dialog_id = get_u8_from_string(cmd, &ret);
		if(ret < 0)
			return ret;
	} else {
		wpa_printf(MSG_ERROR, "TWT: no dialog_id found");
		goto fail;
	}

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_SUSPEND)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_TERMINATE");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (twt_attr == NULL)
		goto fail;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID,
		       dialog_id)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
		goto fail;
	}

	nla_nest_end(nlmsg, twt_attr);
	wpa_printf(MSG_DEBUG, "TWT: pause sent with dialog_id: %x", dialog_id);

	return 0;
fail:
	return -EINVAL;
}

static
int process_twt_resume_cmd_string(char *cmd,
				  struct twt_resume_parameters *resume_params)
{
	int ret = 0;

	if (!resume_params) {
		wpa_printf(MSG_ERROR, "TWT: cmd or resume_params null");
		return -EINVAL;
	}

	if(check_cmd_input(cmd))
		return -EINVAL;

	while(*cmd == ' ')
		cmd++;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) != 0) {
		wpa_printf(MSG_ERROR, "TWT: dialog ID parameter is required");
		return -EINVAL;
	}
	cmd += (DIALOG_ID_STR_LEN + 1);
	resume_params->dialog_id = get_u8_from_string(cmd, &ret);
	if (ret < 0)
		return ret;
	cmd = move_to_next_str(cmd);

	if (os_strncasecmp(cmd, NEXT_TWT_STR, NEXT_TWT_STR_LEN) == 0) {
		cmd += (NEXT_TWT_STR_LEN + 1);
		resume_params->next_twt = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		wpa_printf(MSG_DEBUG, "TWT: NEXT TWT %d", resume_params->next_twt);
		cmd = move_to_next_str(cmd);
	}

	if (os_strncasecmp(cmd, NEXT2_TWT_STR, NEXT2_TWT_STR_LEN) == 0) {
		cmd += (NEXT2_TWT_STR_LEN + 1);
		resume_params->next2_twt = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		wpa_printf(MSG_DEBUG, "TWT: NEXT2 TWT %d", resume_params->next2_twt);
		cmd = move_to_next_str(cmd);
	}

	if (os_strncasecmp(cmd, NEXT_TWT_SIZE_STR, NEXT_TWT_SIZE_STR_LEN) != 0) {
		wpa_printf(MSG_ERROR, "TWT: next_twt_size parameter is required");
		return -EINVAL;
	}
	cmd += (NEXT_TWT_SIZE_STR_LEN + 1);
	resume_params->next_twt_size = get_u32_from_string(cmd, &ret);
	if (ret < 0)
		return ret;

	return 0;
}

static
int prepare_twt_resume_nlmsg(struct nl_msg *nlmsg,
			     struct twt_resume_parameters *resume_params)
{
	struct nlattr *twt_attr;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_RESUME)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_RESUME");
		return -EINVAL;
	}

	twt_attr = nla_nest_start(nlmsg,QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (twt_attr == NULL)
		return -EINVAL;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_RESUME_FLOW_ID,
		       resume_params->dialog_id)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
		return -EINVAL;
	}
	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_RESUME_NEXT_TWT,
		       resume_params->next_twt)) {
		wpa_printf(MSG_DEBUG, "TWT: next_twt");
		return -EINVAL;
	}
	if (nla_put_u32(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_RESUME_NEXT2_TWT,
			resume_params->next2_twt)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put next2_twt");
		return -EINVAL;
	}
	if (nla_put_u32(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_RESUME_NEXT_TWT_SIZE,
			resume_params->next_twt_size)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put next_twt_size");
		return -EINVAL;
	}
	nla_nest_end(nlmsg, twt_attr);

	wpa_printf(MSG_DEBUG,"TWT: resume dialog_id: 0x%x next_twt (us): 0x%x next2_twt (us): 0x%x next_twt_size: %u",
		   resume_params->dialog_id, resume_params->next_twt,
		   resume_params->next2_twt,resume_params->next_twt_size);

	return 0;
}

/**
 * process_twt_nudge_cmd_string()- processes command string for nudge command.
 *
 * @Param cmd: expects the command
 * @Param nudge_params: return parsed nudge parameters
 *
 * @Returns 0 on Success, -EINVAL on Failure
 */
static
int process_twt_nudge_cmd_string(char *cmd,
				 struct twt_nudge_parameters *nudge_params)
{
	int ret = 0;

	if (!nudge_params) {
		wpa_printf(MSG_ERROR, "TWT: nudge_params null");
		return -EINVAL;
	}

	if(check_cmd_input(cmd))
		return -EINVAL;

	while(*cmd == ' ')
		cmd++;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) != 0) {
		wpa_printf(MSG_ERROR, "TWT: dialog_id parameter is required");
		return -EINVAL;
	}
	cmd += (DIALOG_ID_STR_LEN + 1);
	nudge_params->dialog_id = get_u8_from_string(cmd, &ret);
	if (ret < 0)
		return ret;
	cmd = move_to_next_str(cmd);

	if (os_strncasecmp(cmd, PAUSE_DURATION_STR, PAUSE_DURATION_STR_LEN) == 0) {
		cmd += (PAUSE_DURATION_STR_LEN + 1);
		nudge_params->wake_time = get_u32_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		wpa_printf(MSG_DEBUG, "TWT: WAKE TIME %d", nudge_params->wake_time);
		cmd = move_to_next_str(cmd);
	}

	if (os_strncasecmp(cmd, NEXT_TWT_SIZE_STR, NEXT_TWT_SIZE_STR_LEN) != 0) {
		wpa_printf(MSG_ERROR, "TWT: next_twt_size parameter is required");
		return -EINVAL;
	}
	cmd += (NEXT_TWT_SIZE_STR_LEN + 1);
	nudge_params->next_twt_size = get_u32_from_string(cmd, &ret);
	if (ret < 0)
		return ret;

	return 0;
}

/**
 * prepare_twt_nudge_nlmsg()- prepare twt_session_nudge command .
 *
 * @Param nlmsg: nl command buffer
 * @Param nudge_params: nudge parameters to prepare command
 *
 * @Returns 0 on Success, -EINVAL on Failure
 */
static
int prepare_twt_nudge_nlmsg(struct nl_msg *nlmsg,
			    struct twt_nudge_parameters *nudge_params)
{
	struct nlattr *twt_attr;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_NUDGE)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put twt operation");
		return -EINVAL;
	}

	twt_attr = nla_nest_start(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (twt_attr == NULL)
		return -EINVAL;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_NUDGE_FLOW_ID,
		       nudge_params->dialog_id)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
		return -EINVAL;
	}

	if (nla_put_u32(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_NUDGE_WAKE_TIME,
		        nudge_params->wake_time)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put wake_time");
		return -EINVAL;
	}

	if (nla_put_u32(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_NUDGE_NEXT_TWT_SIZE,
			nudge_params->next_twt_size)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put next_twt_size");
		return -EINVAL;
	}
	nla_nest_end(nlmsg, twt_attr);

	wpa_printf(MSG_DEBUG,"TWT: nudge dialog_id: 0x%x wake_time(us): 0x%x next_twt_size: %u",
		   nudge_params->dialog_id, nudge_params->wake_time,
		   nudge_params->next_twt_size);

	return 0;
}

/**
 * prepare_twt_clear_stats_nlmsg()- prepare twt_session_clear_stats command.
 *
 * @Param nlmsg: nl command buffer
 * @Param cmd: command string
 *
 * @Returns 0 on Success, -EINVAL on Failure
 */
static int prepare_twt_clear_stats_nlmsg(struct nl_msg *nlmsg, char *cmd)
{
	struct nlattr *twt_attr;
	u8 dialog_id;
	int ret = 0;

	while(*cmd == ' ')
		cmd++;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_CLEAR_STATS)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_CLEAR_STATS");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				  QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (!twt_attr)
		return -EINVAL;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) == 0) {
		cmd += DIALOG_ID_STR_LEN + 1;

		dialog_id = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_STATS_FLOW_ID,
			       dialog_id)) {
			wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
			return -EINVAL;
		}
		wpa_printf(MSG_DEBUG, "TWT: clear_stats dialog_id:%d", dialog_id);
		cmd = move_to_next_str(cmd);
	} else {
		wpa_printf(MSG_DEBUG, "TWT: dialog_id not found");
		goto fail;
	}

	nla_nest_end(nlmsg, twt_attr);
	return 0;
fail:
	return -EINVAL;
}

/**
 * prepare_twt_get_stats_nlmsg()- prepare twt_session_get_stats command.
 *
 * @Param nlmsg: nl command buffer
 * @Param cmd: command string
 *
 * @Returns 0 on Success, -EINVAL on Failure
 */
static int prepare_twt_get_stats_nlmsg(struct nl_msg *nlmsg, char *cmd)
{
	struct nlattr *twt_attr;
	u8 dialog_id;
	int ret = 0;

	while(*cmd == ' ')
		cmd++;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_GET_STATS)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_GET_STATS");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				  QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (!twt_attr)
		return -EINVAL;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) == 0) {
		cmd += DIALOG_ID_STR_LEN + 1;

		dialog_id = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;
		if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_STATS_FLOW_ID,
			       dialog_id)) {
			wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
			return -EINVAL;
		}
		wpa_printf(MSG_DEBUG, "TWT: get_stats dialog_id:%d", dialog_id);
		cmd = move_to_next_str(cmd);
	} else {
		wpa_printf(MSG_DEBUG, "TWT: dialog_id not found");
		goto fail;
	}

	nla_nest_end(nlmsg, twt_attr);
	return 0;
fail:
	return -EINVAL;
}

int wpa_driver_nl80211_oem_event(struct wpa_driver_nl80211_data *drv,
					   u32 vendor_id, u32 subcmd,
					   u8 *data, size_t len)
{
	int ret = WPA_DRIVER_OEM_STATUS_ENOSUPP, lib_n;
	static wpa_driver_oem_cb_table_t *oem_cb_table = NULL;

	if (wpa_driver_oem_initialize(&oem_cb_table) != WPA_DRIVER_OEM_STATUS_FAILURE &&
	    oem_cb_table) {

		for (lib_n = 0;
		     oem_cb_table[lib_n].wpa_driver_driver_cmd_oem_cb != NULL;
		     lib_n++)
		{
			if(oem_cb_table[lib_n].wpa_driver_nl80211_driver_oem_event) {
				ret = oem_cb_table[lib_n].wpa_driver_nl80211_driver_oem_event(
						drv, vendor_id,subcmd, data, len);
				if (ret == WPA_DRIVER_OEM_STATUS_SUCCESS ) {
					break;
				} else if (ret == WPA_DRIVER_OEM_STATUS_ENOSUPP) {
					continue;
				} else if (ret == WPA_DRIVER_OEM_STATUS_FAILURE) {
					wpa_printf(MSG_DEBUG, "%s: Received error: %d",
							__func__, ret);
					break;
				}
			}
		}
	}

	return ret;
}

static int prepare_twt_get_params_nlmsg(struct nl_msg *nlmsg, char *cmd)
{
	struct nlattr *twt_attr;
	u8 dialog_id;
	int ret = 0;

	while(*cmd == ' ')
		cmd++;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_GET)) {
		wpa_printf(MSG_DEBUG, "TWT: Failed to put QCA_WLAN_TWT_GET");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				  QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (!twt_attr)
		return -EINVAL;

	if (os_strncasecmp(cmd, DIALOG_ID_STR, DIALOG_ID_STR_LEN) == 0) {
		cmd += DIALOG_ID_STR_LEN + 1;

		dialog_id = get_u8_from_string(cmd, &ret);
		if (ret < 0)
			return ret;

		if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID,
			       dialog_id)) {
			wpa_printf(MSG_DEBUG, "TWT: Failed to put dialog_id");
			return -EINVAL;
		}
		wpa_printf(MSG_DEBUG, "TWT: get_param dialog_id:%d", dialog_id);
		cmd = move_to_next_str(cmd);
	} else {
		wpa_printf(MSG_ERROR, "TWT: dialog_id not found");
		goto fail;
	}

	nla_nest_end(nlmsg, twt_attr);
	return 0;
fail:
	return -EINVAL;
}

/**
* prepare_twt_get_cap_nlmsg()- processes and packs get capability command.
* The command is expected in below format:
* TWT_GET_CAP
*
* @Param nlmsg: stores the nlmsg
* @Param cmd: expects the command
*
* @Returns 0 on Success, -EINVAL on Failure
*/
static int prepare_twt_get_cap_nlmsg(struct nl_msg *nlmsg, char *cmd)
{
	struct nlattr *twt_attr;

	if (nla_put_u8(nlmsg, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION,
		       QCA_WLAN_TWT_GET_CAPABILITIES)) {
		wpa_printf(MSG_ERROR,"TWT: Failed to put QCA_WLAN_TWT_GET_CAPABILITIES");
		goto fail;
	}

	twt_attr = nla_nest_start(nlmsg,
				  QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS);
	if (twt_attr == NULL)
		goto fail;
	nla_nest_end(nlmsg, twt_attr);
	return 0;
fail:
	return -EINVAL;
}

static int pack_nlmsg_twt_params(struct nl_msg *twt_nl_msg, char *cmd,
				 enum qca_wlan_twt_operation type)
{
	struct nlattr *attr;
	int ret = 0;

	attr = nla_nest_start(twt_nl_msg, NL80211_ATTR_VENDOR_DATA);
	if (attr == NULL)
		return -EINVAL;

	switch (type) {
	case QCA_WLAN_TWT_SET:
	{
		struct twt_setup_parameters setup_params = {0};

		if (process_twt_setup_cmd_string(cmd, &setup_params))
			return -EINVAL;
		ret = prepare_twt_setup_nlmsg(twt_nl_msg, &setup_params);
		break;
	}
	case QCA_WLAN_TWT_TERMINATE:
		ret = prepare_twt_terminate_nlmsg(twt_nl_msg, cmd);
		break;
	case QCA_WLAN_TWT_SUSPEND:
		ret = prepare_twt_pause_nlmsg(twt_nl_msg, cmd);
		break;
	case QCA_WLAN_TWT_RESUME:
	{
		struct twt_resume_parameters resume_params = {0};

		if (process_twt_resume_cmd_string(cmd, &resume_params))
			return -EINVAL;
		ret = prepare_twt_resume_nlmsg(twt_nl_msg, &resume_params);
		break;
	}
	case QCA_WLAN_TWT_NUDGE:
	{
		struct twt_nudge_parameters nudge_params = {0};

		if (process_twt_nudge_cmd_string(cmd, &nudge_params))
			return -EINVAL;
		ret = prepare_twt_nudge_nlmsg(twt_nl_msg, &nudge_params);
		break;
	}
	case QCA_WLAN_TWT_GET_CAPABILITIES:
		ret = prepare_twt_get_cap_nlmsg(twt_nl_msg, cmd);
		break;
	case QCA_WLAN_TWT_CLEAR_STATS:
		ret = prepare_twt_clear_stats_nlmsg(twt_nl_msg, cmd);
		break;
	case QCA_WLAN_TWT_GET_STATS:
		ret = prepare_twt_get_stats_nlmsg(twt_nl_msg, cmd);
		break;
	case QCA_WLAN_TWT_GET:
		ret = prepare_twt_get_params_nlmsg(twt_nl_msg, cmd);
		break;
	default:
		wpa_printf(MSG_DEBUG, "Unsupported command: %d", type);
		ret = -EINVAL;
		break;
	}

	if (!ret)
		nla_nest_end(twt_nl_msg, attr);

	return ret;
}

char *result_copy_to_buf(char *src, char *dst_buf, int *dst_len)
{
	int str_len, remaining = 0;

	remaining = *dst_len;
	str_len = strlen(src);
	remaining = remaining - (str_len + 1);

	if (remaining <= 0) {
		wpa_printf(MSG_ERROR, "destination buffer length not enough");
		return NULL;
	}
	os_memcpy(dst_buf, src, str_len);

	*dst_len = remaining;
	*(dst_buf + str_len) = ' ';

	return (dst_buf + str_len + 1);
}

static int
unpack_twt_get_params_resp(struct nlattr **tb, char *buf, int buf_len)
{
	int cmd_id, val, len;
	uint8_t exp;
	uint32_t value;
	unsigned long wake_tsf;
	char temp[TWT_RESP_BUF_LEN];
	char *start = buf;

	os_memset(temp, 0, TWT_RESP_BUF_LEN);

	/* Mac Address */
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAC_ADDR;
	if (!tb[cmd_id]) {
		wpa_printf(MSG_ERROR, "twt_get_params resp: no mac_addr");
		return -EINVAL;
	}
	os_snprintf(temp, TWT_RESP_BUF_LEN, "<mac_addr " MAC_ADDR_STR,
		    MAC_ADDR_ARRAY((char *)nla_data(tb[cmd_id])));
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Flow Id */
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID;
	if (!tb[cmd_id]) {
		wpa_printf(MSG_ERROR, "twt_get_params resp: no dialog_id");
		return -EINVAL;
	}
	val = nla_get_u8(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "dialog_id %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;


	/* Broadcast */
	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_BCAST;
	if (tb[cmd_id])
		val = nla_get_flag(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "bcast %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Trigger Type */
	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_TRIGGER;
	if (tb[cmd_id])
		val = nla_get_flag(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "trig_type %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Announce */
	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_TYPE;
	if (tb[cmd_id])
		val = nla_get_flag(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "flow_type %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Protection */
	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_PROTECTION;
	if (tb[cmd_id])
		val = nla_get_flag(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "protection %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* info */
	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_TWT_INFO_ENABLED;
	if (tb[cmd_id])
		val = nla_get_flag(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "info_enabled %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Wake Duration */
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_DURATION;
	if (!tb[cmd_id]) {
		wpa_printf(MSG_ERROR, "twt_get_params resp: no wake duration");
		return -EINVAL;
	}
	value = nla_get_u32(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_dur %d", value);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Wake Interval Mantissa */
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL2_MANTISSA;
	if (!tb[cmd_id]) {
		cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL_MANTISSA;
		if (!tb[cmd_id]) {
			wpa_printf(MSG_ERROR, "twt_get_params resp: no wake mantissa");
			return -EINVAL;
		}
		value = nla_get_u32(tb[cmd_id]);
		value = value * TWT_WAKE_INTERVAL_TU_FACTOR;
	} else {
		value = nla_get_u32(tb[cmd_id]);
	}
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_intvl_mantis %d", value);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* Wake Interval Exponent */
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL_EXP;
	if (!tb[cmd_id]) {
		wpa_printf(MSG_ERROR, "twt_get_params resp: no wake intvl exp");
		return -EINVAL;
	}
	exp = nla_get_u8(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_intvl_exp %d", exp);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	/* TSF Value */
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_TIME_TSF;
	if (!tb[cmd_id]) {
		wpa_printf(MSG_ERROR, "twt_get_params resp: no wake time tsf");
		return -EINVAL;
	}
	wake_tsf = nla_get_u64(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_time_tsf 0x%lx>", wake_tsf);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_STATE;
	if (!tb[cmd_id]) {
		wpa_printf(MSG_ERROR, "twt_get_params resp: no state info");
		return -EINVAL;
	}
	value = nla_get_u32(tb[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "state %d", value);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	len = (buf - start);
	*buf = '\0';

	return len;
}

static int wpa_get_twt_setup_resp_val(struct nlattr **tb2, char *buf,
				      int buf_len)
{
	uint32_t wake_intvl_exp = 0, wake_intvl_mantis = 0;
	int cmd_id, val;
	uint32_t value;
	unsigned long wake_tsf;
	char temp[TWT_RESP_BUF_LEN];

	buf = result_copy_to_buf(TWT_SETUP_RESP, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	os_memset(temp, 0, TWT_RESP_BUF_LEN);
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "TWT dialog id missing");
		return -EINVAL;
	}

	val = nla_get_u8(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "dialog_id %d ", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_STATUS;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "TWT resp status missing");
		return -EINVAL;
	}

	val = nla_get_u8(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "status %d ", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	os_memset(temp, 0, TWT_RESP_BUF_LEN);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "(%s)", twt_status_to_string(val));
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_RESP_TYPE;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "TWT resp type missing");
		return -EINVAL;
	}
	val = nla_get_u8(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "resp_reason %d ", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL_EXP;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "TWT_SETUP_WAKE_INTVL_EXP is must");
		return -EINVAL;
	}
	wake_intvl_exp = nla_get_u8(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_intvl_exp %d ",
		    wake_intvl_exp);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_BCAST;
	if (tb2[cmd_id])
		val = nla_get_flag(tb2[cmd_id]);

	os_snprintf(temp, TWT_RESP_BUF_LEN, "bcast %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_TRIGGER;
	if (tb2[cmd_id])
		val = nla_get_flag(tb2[cmd_id]);

	os_snprintf(temp, TWT_RESP_BUF_LEN, "trig_type %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_TYPE;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "TWT_SETUP_FLOW_TYPE is must");
		return -EINVAL;
	}
	val = nla_get_u8(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "flow_type %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_PROTECTION;
	if (tb2[cmd_id])
		val = nla_get_flag(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "protection %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	value = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_TIME;
	if (tb2[cmd_id])
		value = nla_get_u32(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_time 0x%x", value);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_DURATION;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "TWT_SETUP_WAKE_DURATION is must");
		return -EINVAL;
	}
	value = nla_get_u32(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_dur %d", value);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL2_MANTISSA;
	if (!tb2[cmd_id]) {
		cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_INTVL_MANTISSA;
		if (!tb2[cmd_id]) {
			wpa_printf(MSG_ERROR, "SETUP_WAKE_INTVL_MANTISSA is must");
			return -EINVAL;
		}
		wake_intvl_mantis = nla_get_u32(tb2[cmd_id]);
		wake_intvl_mantis = wake_intvl_mantis * TWT_WAKE_INTERVAL_TU_FACTOR;
	} else {
		wake_intvl_mantis = nla_get_u32(tb2[cmd_id]);
	}
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_intvl %d", wake_intvl_mantis);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	wake_tsf = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_WAKE_TIME_TSF;
	if (tb2[cmd_id])
		wake_tsf = nla_get_u64(tb2[cmd_id]);
	os_snprintf(temp, TWT_RESP_BUF_LEN, "wake_tsf 0x%lx", wake_tsf);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	val = 0;
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_TWT_INFO_ENABLED;
	if (tb2[cmd_id])
		val = nla_get_flag(tb2[cmd_id]);

	os_snprintf(temp, TWT_RESP_BUF_LEN, "info_enabled %d", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;
	*buf = '\0';

	return 0;
}

/**
 * unpack_twt_get_params_nlmsg()- unpacks and prints the twt get_param
 * response recieved from driver synchronously for twt_session_get_params.
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -1 on Failure
 */
static int unpack_twt_get_params_nlmsg(struct nl_msg **tb, char *buf, int buf_len)
{
	int ret, rem, id, len = 0, num_twt_sessions = 0;
	struct nlattr *config_attr[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX + 1];
	struct nlattr *setup_attr[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];
	struct nlattr *attr;

	if (nla_parse_nested(config_attr, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX,
			     tb[NL80211_ATTR_VENDOR_DATA], NULL)) {
		wpa_printf(MSG_ERROR, "twt_get_params: nla_parse_nested fail");
		return -EINVAL;
	}

	id = QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS;
	attr = config_attr[id];
	if (!attr) {
		wpa_printf(MSG_ERROR, "twt_get_params: config_twt_params fail");
		return -EINVAL;
	}

	num_twt_sessions = 0;
	nla_for_each_nested(attr, config_attr[id], rem) {
		num_twt_sessions++;
		if (nla_parse(setup_attr, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
			      nla_data(attr), nla_len(attr), NULL)) {
			wpa_printf(MSG_ERROR, "twt_get_params: nla_parse fail");
			return -EINVAL;
		}
		ret = unpack_twt_get_params_resp(setup_attr, buf + len,
						 buf_len - len);
		if (ret < 0)
			return ret;
		len += ret;
	}

	wpa_printf(MSG_ERROR, "twt_get_params: number of twt sessions = %d",
		   num_twt_sessions);

	return 0;
}

/**
 * wpa_get_twt_stats_resp_val()- parse twt get_stats response
 * recieved from driver synchronously for twt_session_get_stats.
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -1 on Failure
 */
static int wpa_get_twt_stats_resp_val(struct nlattr **tb2, char *buf,
				      int buf_len)
{
	int cmd_id, len;
	u32  val = 0;
	u8 val1 = 0;
	char temp[TWT_RESP_BUF_LEN];
	char *start = buf;

	os_memset(temp, 0, TWT_RESP_BUF_LEN);

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_FLOW_ID;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats flow id missing", __func__);
	} else {
		val1 = nla_get_u8(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "flow_id %u", val1);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats flow id : %u", val1);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_NUM_SP_ITERATIONS;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats num sp iterations missing", __func__);
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "num_sp_iteration %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT num sp Iterations : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_MIN_WAKE_DURATION;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats min wake duration missing", __func__);
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "min_wake_dur %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT min wake duration : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_MAX_WAKE_DURATION;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats max wake duration missing", __func__);
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "max_wake_dur %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT Max wake duration : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_SESSION_WAKE_DURATION;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats sess_wake_dur missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "session_wake_dur %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats session wake duration : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_AVG_WAKE_DURATION;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats avg_wake_dur missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "avg_wake_dur %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats avg wake duration : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_AVERAGE_TX_MPDU;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats average tx mpdu missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "tx_mpdu %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats average tx mpdu : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_AVERAGE_RX_MPDU;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats average rx mpdu missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "rx_mpdu %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats average rx mpdu : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_AVERAGE_TX_PACKET_SIZE;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats average tx packet size missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "tx_pkt_size %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats average tx packet size : %u", val);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_STATS_AVERAGE_RX_PACKET_SIZE;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR,"%s TWT stats average rx packet size missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u32(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "rx_pkt_size %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
		wpa_printf(MSG_INFO,"TWT stats average rx packet size : %u", val);
	}
	len = (buf - start);
	*buf = '\0';

	return len;
}

/**
 * unpack_twt_get_stats_nlmsg()- unpacks and prints the twt get_stats
 * response recieved from driver synchronously for twt_session_get_stats.
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -1 on Failure
 */
static
int unpack_twt_get_stats_nlmsg(struct nl_msg **tb, char *buf, int buf_len)
{
	int ret, rem, id, len = 0, num_twt_sessions = 0;
	struct nlattr *config_attr[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX + 1];
	struct nlattr *setup_attr[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];
	struct nlattr *attr;

	if (nla_parse_nested(config_attr, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX,
			     tb[NL80211_ATTR_VENDOR_DATA], NULL)) {
		wpa_printf(MSG_ERROR, "twt_get_stats: nla_parse_nested fail");
		return -EINVAL;
	}

	id = QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS;
	attr = config_attr[id];
	if (!attr) {
		wpa_printf(MSG_ERROR, "twt_get_stats: config_twt_params fail");
		return -EINVAL;
	}

	num_twt_sessions = 0;
	nla_for_each_nested(attr, config_attr[id], rem) {
		num_twt_sessions++;
		if (nla_parse(setup_attr, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
				nla_data(attr), nla_len(attr), NULL)) {
			wpa_printf(MSG_ERROR, "twt_get_stats: nla_parse fail");
			return -EINVAL;
		}
		ret = wpa_get_twt_stats_resp_val(setup_attr, buf + len,
		                                 buf_len - len);
		if (ret < 0)
			return ret;
		len += ret;
	}
	wpa_printf(MSG_INFO,"twt_get_stats: number of twt sessions = %d", num_twt_sessions);

	return 0;
}

static int wpa_get_twt_capabilities_resp_val(struct nlattr **tb2, char *buf,
					     int buf_len)
{
	int cmd_id;
	u16 msb, lsb;
	u32 val;
	char temp[TWT_RESP_BUF_LEN];

	os_memset(temp, 0, TWT_RESP_BUF_LEN);

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_CAPABILITIES_SELF;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_INFO,"%s TWT self capabilities missing", __func__);
		return -EINVAL;
	} else {
		msb = nla_get_u16(tb2[cmd_id]);
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_CAPABILITIES_PEER;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_INFO,"%s TWT peer capabilities missing", __func__);
		return -EINVAL;
	} else {
		lsb  = nla_get_u16(tb2[cmd_id]);
	}
	wpa_printf(MSG_INFO,"TWT self_capab:%d, TWT peer_capab:%d", msb, lsb);
	val = (msb << 16) | lsb;
	os_snprintf(temp, TWT_RESP_BUF_LEN, "0x%x", val);
	buf = result_copy_to_buf(temp, buf, &buf_len);
	if (!buf)
		return -EINVAL;
	*buf = '\0';

	return 0;
}

/**
 * unpack_twt_get_capab_nlmsg()- unpacks and prints the twt get capabilities
 * response recieved from driver synchronously for TWT_GET_CAP command.
 * The response is printed in below hex-format:
 * 0xHIGHLOW
 * HIGH: Self capabilities
 * LOW:  Peer capabilities
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -1 on Failure
 */
static int unpack_twt_get_capab_nlmsg(struct nl_msg **tb, char *buf, int buf_len)
{
	int ret, id;
	struct nlattr *config_attr[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX + 1];
	struct nlattr *setup_attr[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];
	struct nlattr *attr;

	if (nla_parse_nested(config_attr, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX,
			     tb[NL80211_ATTR_VENDOR_DATA], NULL)) {
		wpa_printf(MSG_ERROR, "twt_get_capability: nla_parse_nested fail");
		return -EINVAL;
	}

	id = QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS;
	attr = config_attr[id];
	if (!attr) {
		wpa_printf(MSG_ERROR, "twt_get_capability: config_twt_params fail");
		return -EINVAL;
	}

	if (nla_parse(setup_attr, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
		      nla_data(attr), nla_len(attr), NULL)) {
		wpa_printf(MSG_ERROR, "twt_get_capability: nla_parse fail");
		return -EINVAL;
	}

	ret = wpa_get_twt_capabilities_resp_val(setup_attr, buf, buf_len);
	return ret;
}

/**
 * unpack_twt_setup_nlmsg()- unpacks twt_session_setup response recieved
 * The response is printed in below format:
 * CTRL-EVENT-TWT SETUP dialog_id <dialog_id> status <status> ..
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -EINVAL on invalid response
 */
static int unpack_twt_setup_nlmsg(struct nlattr **tb, char *buf, int buf_len)
{
	int ret = 0;
	struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];

	if (nla_parse_nested(tb2, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
			     tb[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS], NULL)) {
		wpa_printf(MSG_ERROR, "nla_parse failed\n");
		return -EINVAL;
	}

	ret = wpa_get_twt_setup_resp_val(tb2, buf, buf_len);

	return ret;
}

static int unpack_nlmsg_twt_params(struct nl_msg *twt_nl_msg,
				   enum qca_wlan_twt_operation type,
				   char *buf, int buf_len)
{
	int ret = 0;
	struct nlattr *tb[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(twt_nl_msg));

	nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	switch (type) {
	case QCA_WLAN_TWT_GET:
		ret = unpack_twt_get_params_nlmsg(tb, buf, buf_len);
		break;
	case QCA_WLAN_TWT_GET_STATS:
		ret = unpack_twt_get_stats_nlmsg(tb, buf, buf_len);
		break;
	case QCA_WLAN_TWT_GET_CAPABILITIES:
		ret = unpack_twt_get_capab_nlmsg(tb, buf, buf_len);
		break;
	default:
		wpa_printf(MSG_DEBUG, "Unsupported command: %d", type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int twt_response_handler(struct nl_msg *msg, void *arg)
{
	struct twt_resp_info *info = (struct twt_resp_info *) arg;
	struct wpa_driver_nl80211_data *drv = NULL;
	int ret;

	drv = info->drv;
	ret = unpack_nlmsg_twt_params(msg, info->twt_oper, info->reply_buf,
				      info->reply_buf_len);
	wpa_printf(MSG_DEBUG, "%s - twt_oper %d", __func__, info->twt_oper);
	if (!ret)
		wpa_msg(drv->ctx, MSG_INFO,
			TWT_CTRL_EVENT " %s : OK", info->reply_buf);
	else
		wpa_msg(drv->ctx, MSG_INFO,
			TWT_CTRL_EVENT " %s : Error = %d",
			info->reply_buf, ret);

	return ret;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *err = (int *)arg;

	*err = 0;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = (int *)arg;

	*ret = 0;
	return NL_SKIP;
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
			 void *arg)
{
	int *ret = (int *)arg;

	*ret = err->error;
	wpa_printf(MSG_DEBUG, "%s received : %d - %s", __func__,
		   err->error, strerror(err->error));
	return NL_SKIP;
}

static int no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

int send_nlmsg_get_resp(struct nl_sock *drv_nl_sock, struct nl_msg *drv_nl_msg,
			int (*valid_handler)(struct nl_msg *, void *),
			void *valid_data)
{
	struct nl_cb *cb;
	int err;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb)
		return -ENOMEM;

	err = nl_send_auto_complete(drv_nl_sock, drv_nl_msg);
	if (err < 0) {
		wpa_printf(MSG_ERROR,
			   "nl_send_auto_complete: failed with err=%d", err);
		goto free_mem;
	}

	err = 1;

	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);
	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);
	if (valid_handler)
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM,
			  valid_handler, valid_data);

	while (err > 0) {
		int res = nl_recvmsgs(drv_nl_sock, cb);

		if (res < 0) {
			wpa_printf(MSG_ERROR,
				   "%s nl_recvmsgs failed: ret=%d, err=%d",
				    __func__, res, err);
		}
	}

free_mem:
	nl_cb_put(cb);
	return err;
}

struct features_info {
	u8 *flags;
	size_t flags_len;
};

static
int check_feature(enum qca_wlan_vendor_features feature,
		  struct features_info *info)
{
	size_t idx = feature / 8;

	return (idx < info->flags_len) &&
		(info->flags[idx] & BIT(feature % 8));
}

/* features_info_handler() - parse sync response for get_feature cmd
 *
 * @param msg: nl_msg buffer
 * @Param arg: feature infor
 *
 * @Returns 0 on Success, error code on invalid response
 */
static
int features_info_handler(struct nl_msg *msg, void *arg)
{
	struct genlmsghdr *mHeader;
	struct nlattr *mAttributes[NL80211_ATTR_MAX_INTERNAL + 1];
	struct nlattr *vendata, *attr;
	int datalen;

	struct features_info *info = (struct features_info *) arg;
	int status = 0;


	mHeader = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
	nla_parse(mAttributes, NL80211_ATTR_MAX_INTERNAL,
			  genlmsg_attrdata(mHeader, 0),
			  genlmsg_attrlen(mHeader, 0), NULL);

	if (mAttributes[NL80211_ATTR_VENDOR_DATA]) {
		vendata = nla_data(mAttributes[NL80211_ATTR_VENDOR_DATA]);
		datalen = nla_len(mAttributes[NL80211_ATTR_VENDOR_DATA]);
		if (!vendata) {
			wpa_printf(MSG_ERROR,"Vendor data not found");
			return -1;
		}
		struct nlattr *tb_vendor[QCA_WLAN_VENDOR_ATTR_MAX + 1];

		nla_parse(tb_vendor, QCA_WLAN_VENDOR_ATTR_MAX,
				vendata, datalen, NULL);

		attr = tb_vendor[QCA_WLAN_VENDOR_ATTR_FEATURE_FLAGS];
		if (attr) {
			int len = nla_len(attr);
			info->flags = os_malloc(len);
			if (info->flags != NULL) {
				os_memcpy(info->flags, nla_data(attr), len);
				info->flags_len = len;
			}
		} else {
			wpa_printf(MSG_ERROR,"VENDOR_ATTR_FEATURE_FLAGS not found");
		}
	} else {
		wpa_printf(MSG_ERROR,"NL80211_ATTR_VENDOR_DATA not found");
		status = -1;
	}

	return status;
}

/* pack_nlmsg_vendor_feature_hdr() - pack get_features command
 *
 * @param drv_nl_msg: nl_msg buffer
 * @Param drv: nl data
 * @Param ifname: interface name
 *
 * @Returns 0 on Success, error code on invalid response
 */
static
int pack_nlmsg_vendor_feature_hdr(struct nl_msg *drv_nl_msg,
				  struct wpa_driver_nl80211_data *drv,
				  char *ifname)
{
	int ret;
	int ifindex;

	genlmsg_put(drv_nl_msg, NL_AUTO_PORT, NL_AUTO_SEQ,
		    drv->global->nl80211_id, 0, 0,
		    NL80211_CMD_VENDOR, 0);

	ret = nla_put_u32(drv_nl_msg, NL80211_ATTR_VENDOR_ID, OUI_QCA);
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "Failed to put vendor id");
		return ret;
	}

	ret = nla_put_u32(drv_nl_msg, NL80211_ATTR_VENDOR_SUBCMD,
			  QCA_NL80211_VENDOR_SUBCMD_GET_FEATURES);
	if (ret < 0) {
		wpa_printf(MSG_DEBUG, "nl put twt vendor subcmd failed");
		return ret;
	}

	if (ifname && (strlen(ifname) > 0))
		ifindex = if_nametoindex(ifname);
	else
		ifindex = if_nametoindex(DEFAULT_IFNAME);

	ret = nla_put_u32(drv_nl_msg, NL80211_ATTR_IFINDEX, ifindex);
	if (ret < 0) {
		wpa_printf(MSG_DEBUG, "nl put iface: %s failed", ifname);
		return ret;
	}
	return ret;
}

/* check_wifi_twt_async_feature() - check if driver supports twt async feature
 *
 * @Param drv: nl data
 * @Param ifname: interface name
 *
 * @Returns 1 if twt async feature is supported, 0 otherwise
 */
static int check_wifi_twt_async_feature(struct wpa_driver_nl80211_data *drv,
					char *ifname)
{
	struct nl_msg *nlmsg;
	struct features_info info;
	int ret;

	if (twt_async_support != -1) {
		return twt_async_support;
	}

        nlmsg = nlmsg_alloc();
	if (!nlmsg) {
		wpa_printf(MSG_ERROR, "nlmg alloc failure");
		return -ENOMEM;
	}

	pack_nlmsg_vendor_feature_hdr(nlmsg, drv, ifname);
	os_memset(&info, 0, sizeof(info));
	ret = send_nlmsg_get_resp((struct nl_sock *)drv->global->nl, nlmsg,
				  features_info_handler, &info);

	if (ret || !info.flags) {
		nlmsg_free(nlmsg);
		return 0;
	}

	if (check_feature(QCA_WLAN_VENDOR_FEATURE_TWT_ASYNC_SUPPORT, &info)) {
		twt_async_support = 1;
	} else {
		twt_async_support = 0;
	}

	os_free(info.flags);
	nlmsg_free(nlmsg);
	return twt_async_support;
}

static int wpa_driver_twt_cmd_handler(struct wpa_driver_nl80211_data *drv,
				      char *ifname,
				      enum qca_wlan_twt_operation twt_oper,
				      char *param, char *buf,
				      size_t buf_len, int *status)
{
	struct nl_msg *twt_nl_msg;
	struct twt_resp_info reply_info;
	int ret = 0;

	if (!param) {
		wpa_printf(MSG_ERROR, "%s:TWT cmd args missing\n", __func__);
		return -EINVAL;
	}

	if (!buf) {
		wpa_printf(MSG_ERROR, "buf is NULL");
		return -EINVAL;
	}

	wpa_printf(MSG_DEBUG, "TWT DRIVER cmd: %s", param);

	memset(&reply_info, 0, sizeof(struct twt_resp_info));
	os_memset(buf, 0, buf_len);

	reply_info.twt_oper = twt_oper;
	reply_info.reply_buf = buf;
	reply_info.reply_buf_len = buf_len;
	reply_info.drv = drv;

	twt_nl_msg = nlmsg_alloc();
	if (!twt_nl_msg) {
		wpa_printf(MSG_ERROR, "nlmg alloc failure");
		return -ENOMEM;
	}

	ret = pack_nlmsg_vendor_hdr(twt_nl_msg, drv, ifname);
	if (ret)
		goto free_mem;

	ret = pack_nlmsg_twt_params(twt_nl_msg, param, twt_oper);
	if (ret)
		goto free_mem;

	switch(twt_oper) {
	case QCA_WLAN_TWT_GET:
	case QCA_WLAN_TWT_GET_CAPABILITIES:
	case QCA_WLAN_TWT_GET_STATS:
		*status = send_nlmsg_get_resp((struct nl_sock *)drv->global->nl,
					      twt_nl_msg, twt_response_handler,
					      &reply_info);
		if (*status != 0) {
			wpa_printf(MSG_ERROR, "Failed to send nlmsg - err %d", *status);
			ret = -EINVAL;
		}
		break;
	case QCA_WLAN_TWT_CLEAR_STATS:
		*status = send_nlmsg_get_resp((struct nl_sock *)drv->global->nl,
					      twt_nl_msg, NULL, NULL);
		if (*status != 0) {
			wpa_printf(MSG_ERROR, "Failed to send nlmsg - err %d", *status);
			ret = -EINVAL;
		}
		break;
	case QCA_WLAN_TWT_SET:
	case QCA_WLAN_TWT_TERMINATE:
	case QCA_WLAN_TWT_SUSPEND:
	case QCA_WLAN_TWT_RESUME:
	case QCA_WLAN_TWT_NUDGE:
		if(check_wifi_twt_async_feature(drv, ifname) == 0) {
			wpa_printf(MSG_ERROR, "Asynchronous TWT Feature is missing");
			ret = -EINVAL;
		} else {
			*status = send_nlmsg_get_resp((struct nl_sock *)drv->global->nl,
						      twt_nl_msg, NULL, NULL);
			if (*status != 0) {
				wpa_printf(MSG_ERROR, "Failed to send nlmsg - err %d", *status);
				ret = -EINVAL;
			}
		}
		break;
	default:
		wpa_printf(MSG_ERROR, "nlmg send failure");
		ret = -EINVAL;
		goto free_mem;
	}

	wpa_printf(MSG_ERROR, "sent nlmsg - status %d", *status);
free_mem:
	if (twt_nl_msg)
		nlmsg_free(twt_nl_msg);
	return ret;
}

/**
 * unpack_twt_terminate_event()- unpacks twt_session_terminate response recieved
 * The response is printed in below format:
 * CTRL-EVENT-TWT TERMINATE dialog_id <dialog_id> status <status>
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -EINVAL on invalid response
 */
static
int unpack_twt_terminate_event(struct nlattr **tb, char *buf, int buf_len)
{
	int cmd_id;
	u8 val;
	char temp[TWT_RESP_BUF_LEN];
	struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];

	if (nla_parse_nested(tb2, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
			     tb[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS], NULL)) {
		wpa_printf(MSG_ERROR, "nla_parse failed");
		return -EINVAL;
	}

	buf = result_copy_to_buf(TWT_TEARDOWN_RESP, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	os_memset(temp, 0, TWT_TEARDOWN_RESP_LEN);
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "%s TWT dialog id missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u8(tb2[cmd_id]);
		if(val == 255) {
			val = 0;
		}
		os_snprintf(temp, TWT_RESP_BUF_LEN, "dialog_id %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_STATUS;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "%s TWT resp status missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u8(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "status %u ", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;

		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "(%s)", twt_status_to_string(val));
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
	}
	*buf = '\0';

	return 0;
}

/**
 * unpack_twt_suspend_event()- unpacks twt_session_pause response recieved
 * The response is printed in below format:
 * CTRL-EVENT-TWT PAUSE dialog_id <dialog_id> status <status>
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -EINVAL on invalid response
 */
static
int unpack_twt_suspend_event(struct nlattr **tb, char *buf, int buf_len)
{
	int cmd_id;
	u8 val;
	char temp[TWT_RESP_BUF_LEN];
	struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];

	if (nla_parse_nested(tb2, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
			     tb[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS], NULL)) {
		wpa_printf(MSG_ERROR, "nla_parse failed");
		return -1;
	}

	buf = result_copy_to_buf(TWT_PAUSE_RESP, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	os_memset(temp, 0, TWT_PAUSE_RESP_LEN);
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_FLOW_ID;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "%s TWT dialog id missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u8(tb2[cmd_id]);
		if(val == 255) {
			val = 0;
		}
		os_snprintf(temp, TWT_RESP_BUF_LEN, "dialog_id %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_STATUS;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "%s TWT resp status missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u8(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "status %u ", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;

		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "(%s)", twt_status_to_string(val));
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
	}
	*buf = '\0';

	return 0;
}

/**
 * unpack_twt_resume_event()- unpacks twt_session_resume response recieved
 * The response is printed in below format:
 * CTRL-EVENT-TWT RESUME dialog_id <dialog_id> status <status>
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -EINVAL on invalid response
 */
static
int unpack_twt_resume_event(struct nlattr **tb, char *buf, int buf_len)
{
	int cmd_id;
	u8 val;
	char temp[TWT_RESP_BUF_LEN];
	struct nlattr *tb2[QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX + 1];

	if (nla_parse_nested(tb2, QCA_WLAN_VENDOR_ATTR_TWT_SETUP_MAX,
			     tb[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_PARAMS], NULL)) {
		wpa_printf(MSG_ERROR, "nla_parse failed");
		return -1;
	}

	buf = result_copy_to_buf(TWT_RESUME_RESP, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	os_memset(temp, 0, TWT_RESUME_RESP_LEN);
	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_RESUME_FLOW_ID;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "%s TWT dialog id missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u8(tb2[cmd_id]);
		if(val == 255) {
			val = 0;
		}
		os_snprintf(temp, TWT_RESP_BUF_LEN, "dialog_id %u", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
	}

	cmd_id = QCA_WLAN_VENDOR_ATTR_TWT_SETUP_STATUS;
	if (!tb2[cmd_id]) {
		wpa_printf(MSG_ERROR, "%s TWT resp status missing", __func__);
		return -EINVAL;
	} else {
		val = nla_get_u8(tb2[cmd_id]);
		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "status %u ", val);
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;

		os_memset(temp, 0, TWT_RESP_BUF_LEN);
		os_snprintf(temp, TWT_RESP_BUF_LEN, "(%s)", twt_status_to_string(val));
		buf = result_copy_to_buf(temp, buf, &buf_len);
		if (!buf)
			return -EINVAL;
	}
	*buf = '\0';

	return 0;
}

/**
 * unpack_twt_notify_event()- prints the twt notify response recieved from driver
 * asynchronously for QCA_WLAN_TWT_SETUP_READY_NOTIFY event.
 * The response is printed in below format:
 * CTRL-EVENT-TWT NOTIFY
 *
 * @Param tb: vendor nl data
 * @Param buf: stores the response
 * @Param buf_len: length of the response buffer
 *
 * @Returns 0 on Success, -EINVAL on invalid response
 */
int unpack_twt_notify_event(struct nlattr **tb, char *buf, int buf_len)
{
	char temp[TWT_RESP_BUF_LEN];

	os_memset(temp, 0, TWT_RESP_BUF_LEN);

	buf = result_copy_to_buf(TWT_NOTIFY_RESP, buf, &buf_len);
	if (!buf)
		return -EINVAL;

	*buf = '\0';
        return 0;
}

/**
 * wpa_driver_twt_async_resp_handler()- handler for asynchronous twt vendor event
 * recieved from the driver.
 *
 * @Param drv- wpa_driver_nl80211_data
 * @Param vendor_id- vendor id for vendor specific command
 * @Param subcmd- subcmd as defined by enum qca_nl80211_vendor_subcmds
 * @Param data- vendor data
 * @Param len- vendor data length
 *
 * @Returns 0 for Success, -1 for Failure
 */
static int wpa_driver_twt_async_resp_event(struct wpa_driver_nl80211_data *drv,
					   u32 vendor_id, u32 subcmd, u8 *data, size_t len)
{
	int ret = 0;
	char *buf;
	buf = (char *)malloc(TWT_RESP_BUF_LEN);
	int buf_len = TWT_RESP_BUF_LEN;
	struct nlattr *tb[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX + 1];
	u8 twt_operation_type;

	if (!buf)
		return -1;

	ret = nla_parse(tb, QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_MAX,
			(struct nlattr *) data, len, NULL);
	if (ret) {
		wpa_printf(MSG_ERROR, "nla_parse failed %d", ret);
		goto fail;
	}

	memset(buf, 0, TWT_RESP_BUF_LEN);

	twt_operation_type = nla_get_u8(tb[QCA_WLAN_VENDOR_ATTR_CONFIG_TWT_OPERATION]);

	switch(twt_operation_type) {
	case QCA_WLAN_TWT_SET:
		ret = unpack_twt_setup_nlmsg(tb, buf, buf_len);
		break;
	case QCA_WLAN_TWT_TERMINATE:
		ret = unpack_twt_terminate_event(tb, buf, buf_len);
		break;
	case QCA_WLAN_TWT_SUSPEND:
		ret = unpack_twt_suspend_event(tb, buf, buf_len);
		break;
	case QCA_WLAN_TWT_RESUME:
		ret = unpack_twt_resume_event(tb, buf, buf_len);
		break;
	case QCA_WLAN_TWT_SETUP_READY_NOTIFY:
		ret = unpack_twt_notify_event(tb, buf, buf_len);
		break;
	default:
		ret = -1;
	}

	if (ret) {
		wpa_printf(MSG_ERROR, "Async event parsing failed for operation %d",
			   twt_operation_type);
		goto fail;
	}
	wpa_printf(MSG_ERROR,"%s", buf);
	wpa_msg(drv->ctx, MSG_INFO, "%s", buf);
fail:
	free(buf);
	return ret;
}

int wpa_driver_nl80211_driver_event(struct wpa_driver_nl80211_data *drv,
				    u32 vendor_id, u32 subcmd,
				    u8 *data, size_t len)
{
	int ret = -1;
	wpa_printf(MSG_INFO, "wpa_driver_nld80211 vendor event recieved");

	ret = wpa_driver_nl80211_oem_event(drv, vendor_id, subcmd,
					data, len);

	if (ret != WPA_DRIVER_OEM_STATUS_ENOSUPP)
		return ret;

	switch(subcmd) {
	case QCA_NL80211_VENDOR_SUBCMD_CONFIG_TWT:
		ret = wpa_driver_twt_async_resp_event(drv, vendor_id, subcmd,
						      data, len);
	break;
	default:
		wpa_printf(MSG_DEBUG, "Unsupported vendor event recieved %d",
			   subcmd);
	}
	return ret;
}

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
				  size_t buf_len )
{
	struct i802_bss *bss = priv;
	struct wpa_driver_nl80211_data *drv = NULL;
	struct wpa_driver_nl80211_data *driver;
	struct ifreq ifr;
	android_wifi_priv_cmd priv_cmd;
	int ret = 0, status = 0, lib_n = 0;
	static wpa_driver_oem_cb_table_t *oem_cb_table = NULL;

	if (bss) {
		drv = bss->drv;
	} else {
		if (os_strncasecmp(cmd, "SET_AP_SUSPEND", 14)) {
			wpa_printf(MSG_ERROR, "%s: bss is NULL for cmd %s\n",
				   __func__, cmd);
			return -EINVAL;
		}
	}

	if (wpa_driver_oem_initialize(&oem_cb_table) != WPA_DRIVER_OEM_STATUS_FAILURE &&
	    oem_cb_table) {

		for (lib_n = 0;
		     oem_cb_table[lib_n].wpa_driver_driver_cmd_oem_cb != NULL;
		     lib_n++)
		{
			ret = oem_cb_table[lib_n].wpa_driver_driver_cmd_oem_cb(
					priv, cmd, buf, buf_len, &status);
			if (ret == WPA_DRIVER_OEM_STATUS_SUCCESS ) {
				return strlen(buf);
			} else if (ret == WPA_DRIVER_OEM_STATUS_ENOSUPP) {
				continue;
			} else if ((ret == WPA_DRIVER_OEM_STATUS_FAILURE) &&
				   (status != 0)) {
				wpa_printf(MSG_DEBUG, "%s: Received error: %d",
						__func__, ret);
				return -1;
			}
		}
		/* else proceed with legacy handling as below */
	}

	if (!drv) {
		wpa_printf(MSG_ERROR, "%s: drv is NULL for cmd %s\n",
			   __func__, cmd);
		return -EINVAL;
	}

	if (os_strcasecmp(cmd, "START") == 0) {
		dl_list_for_each(driver, &drv->global->interfaces, struct wpa_driver_nl80211_data, list) {
			linux_set_iface_flags(drv->global->ioctl_sock, driver->first_bss->ifname, 1);
			wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STARTED");
		}
	} else if (os_strcasecmp(cmd, "MACADDR") == 0) {
		u8 macaddr[ETH_ALEN] = {};

		ret = linux_get_ifhwaddr(drv->global->ioctl_sock, bss->ifname, macaddr);
		if (!ret)
			ret = os_snprintf(buf, buf_len,
					  "Macaddr = " MACSTR "\n", MAC2STR(macaddr));
	} else if ((ret = check_for_twt_cmd(&cmd)) != TWT_CMD_NOT_EXIST) {
		enum qca_wlan_twt_operation twt_oper = ret;
		u8 is_twt_feature_supported = 0;

		if (oem_cb_table) {
			for (lib_n = 0;
			     oem_cb_table[lib_n].wpa_driver_driver_cmd_oem_cb != NULL;
			     lib_n++)
			{
				if (oem_cb_table[lib_n].wpa_driver_oem_feature_check_cb) {
					if (oem_cb_table[lib_n].wpa_driver_oem_feature_check_cb(FEATURE_TWT_SUPPORT))
						is_twt_feature_supported = 1;
				}
			}
		}

		if (is_twt_feature_supported) {
			wpa_printf(MSG_ERROR, "%s: TWT feature already supported by oem lib\n", __func__);
			ret = -EINVAL;
		} else {
			ret = wpa_driver_twt_cmd_handler(drv, bss->ifname, twt_oper, cmd, buf, buf_len,
							 &status);
			if (ret)
				ret = os_snprintf(buf, buf_len, "TWT failed for operation %d", twt_oper);
		}
	} else { /* Use private command */
		memset(&ifr, 0, sizeof(ifr));
		memset(&priv_cmd, 0, sizeof(priv_cmd));
		os_memcpy(buf, cmd, strlen(cmd) + 1);
		os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);

		priv_cmd.buf = buf;
		priv_cmd.used_len = buf_len;
		priv_cmd.total_len = buf_len;
		ifr.ifr_data = &priv_cmd;

		if ((ret = ioctl(drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr)) < 0) {
			wpa_printf(MSG_ERROR, "%s: failed to issue private commands\n", __func__);
		} else {
			drv_errors = 0;
			if((os_strncasecmp(cmd, "SETBAND", 7) == 0) &&
				ret == DO_NOT_SEND_CHANNEL_CHANGE_EVENT) {
				return 0;
			}

			ret = 0;
			if ((os_strcasecmp(cmd, "LINKSPEED") == 0) ||
			    (os_strcasecmp(cmd, "RSSI") == 0) ||
			    (os_strstr(cmd, "GET") != NULL))
				ret = strlen(buf);
			else if (os_strcasecmp(cmd, "P2P_DEV_ADDR") == 0)
				wpa_printf(MSG_DEBUG, "%s: P2P: Device address ("MACSTR")",
					__func__, MAC2STR(buf));
			else if (os_strcasecmp(cmd, "P2P_SET_PS") == 0)
				wpa_printf(MSG_DEBUG, "%s: P2P: %s ", __func__, buf);
			else if (os_strcasecmp(cmd, "P2P_SET_NOA") == 0)
				wpa_printf(MSG_DEBUG, "%s: P2P: %s ", __func__, buf);
			else if (os_strcasecmp(cmd, "STOP") == 0) {
				wpa_printf(MSG_DEBUG, "%s: %s ", __func__, buf);
				dl_list_for_each(driver, &drv->global->interfaces, struct wpa_driver_nl80211_data, list) {
					linux_set_iface_flags(drv->global->ioctl_sock, driver->first_bss->ifname, 0);
					wpa_msg(drv->ctx, MSG_INFO, WPA_EVENT_DRIVER_STATE "STOPPED");
				}
			}
			else
				wpa_printf(MSG_DEBUG, "%s %s len = %d, %zu", __func__, buf, ret, buf_len);
			wpa_driver_notify_country_change(drv->ctx, cmd);
		}
	}
	return ret;
}

int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration)
{
	char buf[MAX_DRV_CMD_SIZE];

	memset(buf, 0, sizeof(buf));
	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
	snprintf(buf, sizeof(buf), "P2P_SET_NOA %d %d %d", count, start, duration);
	return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf)+1);
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len)
{
	UNUSED(priv), UNUSED(buf), UNUSED(len);
	/* Return 0 till we handle p2p_presence request completely in the driver */
	return 0;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
	char buf[MAX_DRV_CMD_SIZE];

	memset(buf, 0, sizeof(buf));
	wpa_printf(MSG_DEBUG, "%s: Entry", __func__);
	snprintf(buf, sizeof(buf), "P2P_SET_PS %d %d %d", legacy_ps, opp_ps, ctwindow);
	return wpa_driver_nl80211_driver_cmd(priv, buf, buf, strlen(buf) + 1);
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
				 const struct wpabuf *proberesp,
				 const struct wpabuf *assocresp)
{
	UNUSED(priv), UNUSED(beacon), UNUSED(proberesp), UNUSED(assocresp);
	return 0;
}
