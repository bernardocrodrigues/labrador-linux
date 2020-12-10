// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2012 - 2018 Microchip Technology Inc., and its subsidiaries.
 * All rights reserved.
 */

#include <linux/etherdevice.h>

#include "wilc_wfi_netdevice.h"
#include "linux_wlan.h"
#include "wilc_wfi_cfgoperations.h"

#define HOST_IF_SCAN_TIMEOUT                    4000
#define HOST_IF_CONNECT_TIMEOUT                 9500

#define FALSE_FRMWR_CHANNEL			100

#define REAL_JOIN_REQ				0

#define INVALID_RSSI				100

/* Generic success will return 0 */
#define WILC_SUCCESS		0	/* Generic success */

/* Negative numbers to indicate failures */
/* Generic Fail */
#define	WILC_FAIL		-100
/* Busy with another operation*/
#define	WILC_BUSY		-101
/* A given argument is invalid*/
#define	WILC_INVALID_ARGUMENT	-102
/* An API request would violate the Driver state machine
 * (i.e. to start PID while not camped)
 */
#define	WILC_INVALID_STATE	-103
/* In copy operations if the copied data is larger than the allocated buffer*/
#define	WILC_BUFFER_OVERFLOW	-104
/* null pointer is passed or used */
#define WILC_NULL_PTR		-105
#define	WILC_EMPTY		-107
#define WILC_FULL		-108
#define	WILC_TIMEOUT		-109
/* The required operation have been canceled by the user*/
#define WILC_CANCELED		-110
/* The Loaded file is corruped or having an invalid format */
#define WILC_INVALID_FILE	-112
/* Cant find the file to load */
#define WILC_NOT_FOUND		-113
#define WILC_NO_MEM		-114
#define WILC_UNSUPPORTED_VERSION -115
#define WILC_FILE_EOF		-116

struct host_if_wpa_attr {
	u8 *key;
	const u8 *mac_addr;
	u8 *seq;
	u8 seq_len;
	u8 index;
	u8 key_len;
	u8 mode;
};

struct host_if_wep_attr {
	u8 *key;
	u8 key_len;
	u8 index;
	u8 mode;
	enum authtype auth_type;
};

union host_if_key_attr {
	struct host_if_wep_attr wep;
	struct host_if_wpa_attr wpa;
	struct host_if_pmkid_attr pmkid;
};

struct key_attr {
	enum KEY_TYPE type;
	u8 action;
	union host_if_key_attr attr;
};

struct send_buffered_eap {
	wilc_frmw_to_linux_t frmw_to_linux;
	free_eap_buf_param eap_buf_param;
	u8 *buff;
	unsigned int size;
	unsigned int pkt_offset;
	void *user_arg;
};

struct scan_attr {
	u8 src;
	u8 type;
	u8 *ch_freq_list;
	u8 ch_list_len;
	u8 *ies;
	size_t ies_len;
	wilc_scan_result result;
	void *arg;
	struct hidden_network hidden_network;
};

struct connect_attr {
	u8 *bssid;
	u8 *ssid;
	size_t ssid_len;
	u8 *ies;
	size_t ies_len;
	u8 security;
	wilc_connect_result result;
	void *arg;
	enum authtype auth_type;
	u8 ch;
	void *params;
};

struct rcvd_async_info {
	u8 *buffer;
	u32 len;
};

struct channel_attr {
	u8 set_ch;
};

struct beacon_attr {
	u32 interval;
	u32 dtim_period;
	u32 head_len;
	u8 *head;
	u32 tail_len;
	u8 *tail;
};

struct set_multicast {
	bool enabled;
	u32 cnt;
	u8 *mc_list;
};

struct del_all_sta {
	u8 del_all_sta[MAX_NUM_STA][ETH_ALEN];
	u8 assoc_sta;
};

struct del_sta {
	u8 mac_addr[ETH_ALEN];
};

struct power_mgmt_param {
	bool enabled;
	u32 timeout;
};

struct sta_inactive_t {
	u32 inactive_time;
	u8 mac[6];
};

struct host_if_wowlan_trigger {
	u8 wowlan_trigger;
};

struct tx_power {
	u8 tx_pwr;
};

struct bt_coex_mode {
	u8 bt_coex;
};

struct host_if_set_ant {
	u8 mode;
	u8 antenna1;
	u8 antenna2;
	u8 gpio_mode;
};

union message_body {
	struct scan_attr scan_info;
	struct connect_attr con_info;
	struct rcvd_net_info net_info;
	struct rcvd_async_info async_info;
	struct key_attr key_info;
	struct cfg_param_attr cfg_info;
	struct channel_attr channel_info;
	struct beacon_attr beacon_info;
	struct add_sta_param add_sta_info;
	struct del_sta del_sta_info;
	struct add_sta_param edit_sta_info;
	struct power_mgmt_param pwr_mgmt_info;
	struct sta_inactive_t mac_info;
	struct drv_handler drv;
	struct set_multicast multicast_info;
	struct op_mode mode;
	struct dev_mac_addr dev_mac_info;
	struct ba_session_info session_info;
	struct remain_ch remain_on_ch;
	struct reg_frame reg_frame;
	char *data;
	struct del_all_sta del_all_sta_info;
	struct send_buffered_eap send_buff_eap;
	struct tx_power tx_power;
	struct host_if_set_ant set_ant;
	struct host_if_wowlan_trigger wow_trigger;
	struct bt_coex_mode bt_coex_mode;
};

struct host_if_msg {
	union message_body body;
	struct wilc_vif *vif;
	struct work_struct work;
	void (*fn)(struct work_struct *ws);
	struct completion work_comp;
	bool is_sync;
};

struct join_bss_param {
	enum bss_types bss_type;
	u8 dtim_period;
	u16 beacon_period;
	u16 cap_info;
	u8 bssid[6];
	char ssid[MAX_SSID_LEN];
	u8 ssid_len;
	u8 supp_rates[MAX_RATES_SUPPORTED + 1];
	u8 ht_capable;
	u8 wmm_cap;
	u8 uapsd_cap;
	bool rsn_found;
	u8 rsn_grp_policy;
	u8 mode_802_11i;
	u8 rsn_pcip_policy[3];
	u8 rsn_auth_policy[3];
	u8 rsn_cap[2];
	u32 tsf;
	u8 noa_enabled;
	u8 opp_enabled;
	u8 ct_window;
	u8 cnt;
	u8 idx;
	u8 duration[4];
	u8 interval[4];
	u8 start_time[4];
};

static struct host_if_drv *terminated_handle;
static struct mutex hif_deinit_lock;

/* 'msg' should be free by the caller for syc */
static struct host_if_msg*
wilc_alloc_work(struct wilc_vif *vif, void (*work_fun)(struct work_struct *),
		bool is_sync)
{
	struct host_if_msg *msg;

	if (!work_fun)
		return ERR_PTR(-EINVAL);

	msg = kzalloc(sizeof(*msg), GFP_ATOMIC);
	if (!msg)
		return ERR_PTR(-ENOMEM);
	msg->fn = work_fun;
	msg->vif = vif;
	msg->is_sync = is_sync;
	if (is_sync)
		init_completion(&msg->work_comp);

	return msg;
}

static int wilc_enqueue_work(struct host_if_msg *msg)
{
	INIT_WORK(&msg->work, msg->fn);

	if (!msg->vif || !msg->vif->wilc || !msg->vif->wilc->hif_workqueue)
		return -EINVAL;

	if (!queue_work(msg->vif->wilc->hif_workqueue, &msg->work))
		return -EINVAL;

	return 0;
}

/* The idx starts from 0 to (NUM_CONCURRENT_IFC - 1), but 0 index used as
 * special purpose in wilc device, so we add 1 to the index to starts from 1.
 * As a result, the returned index will be 1 to NUM_CONCURRENT_IFC.
 */
int wilc_get_vif_idx(struct wilc_vif *vif)
{
	return vif->idx + 1;
}

/* We need to minus 1 from idx which is from wilc device to get real index
 * of wilc->vif[], because we add 1 when pass to wilc device in the function
 * wilc_get_vif_idx.
 * As a result, the index should be between 0 and (NUM_CONCURRENT_IFC - 1).
 */
static struct wilc_vif *wilc_get_vif_from_idx(struct wilc *wilc, int idx)
{
	int index = idx - 1;

	if (index < 0 || index >= NUM_CONCURRENT_IFC)
		return NULL;

	return wilc->vif[index];
}

void filter_shadow_scan(struct wilc_priv *priv, u8 *ch_freq_list,
			u8 ch_list_len)
{
	int i;
	int ch_index;
	int j;
	struct network_info *net_info;

	if (ch_list_len == 0)
		return;

	for (i = 0; i < priv->scanned_cnt;) {
		net_info = &priv->scanned_shadow[i];

		for (ch_index = 0; ch_index < ch_list_len; ch_index++)
			if (net_info->ch == (ch_freq_list[ch_index] + 1))
				break;

		/* filter only un-matched channels */
		if (ch_index != ch_list_len) {
			i++;
			continue;
		}

		kfree(net_info->ies);
		net_info->ies = NULL;

		kfree(net_info->join_params);
		net_info->join_params = NULL;

		for (j = i; (j < priv->scanned_cnt-1); j++)
			priv->scanned_shadow[j] = priv->scanned_shadow[j+1];

		priv->scanned_cnt--;
	}
}

static void handle_send_buffered_eap(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct send_buffered_eap *hif_buff_eap = &msg->body.send_buff_eap;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending bufferd eapol to WPAS\n");
	if (!hif_buff_eap->buff)
		goto out;

	if (hif_buff_eap->frmw_to_linux)
		hif_buff_eap->frmw_to_linux(vif, hif_buff_eap->buff,
					    hif_buff_eap->size,
					    hif_buff_eap->pkt_offset,
					    PKT_STATUS_BUFFERED);
	if (hif_buff_eap->eap_buf_param)
		hif_buff_eap->eap_buf_param(hif_buff_eap->user_arg);

	if (hif_buff_eap->buff != NULL) {
		kfree(hif_buff_eap->buff);
		hif_buff_eap->buff = NULL;
	}

out:
	kfree(msg);
}

static void handle_set_channel(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct channel_attr *hif_set_ch = &msg->body.channel_info;
	int ret;
	struct wid wid;

	wid.id = WID_CURRENT_CHANNEL;
	wid.type = WID_CHAR;
	wid.val = (char *)&hif_set_ch->set_ch;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set channel\n");
	kfree(msg);
}

static void handle_set_wfi_drv_handler(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct drv_handler *hif_drv_handler = &msg->body.drv;
	int ret;
	struct wid wid;
	u8 *currbyte, *buffer;
	struct host_if_drv *hif_drv;

	if (!vif->hif_drv || !hif_drv_handler)
		goto free_msg;

	hif_drv	= vif->hif_drv;

	buffer = kzalloc(DRV_HANDLER_SIZE, GFP_KERNEL);
	if (!buffer)
		goto free_msg;

	currbyte = buffer;
	*currbyte = hif_drv_handler->handler & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (u32)0 & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (u32)0 & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (u32)0 & DRV_HANDLER_MASK;
	currbyte++;
	*currbyte = (hif_drv_handler->ifc_id | (hif_drv_handler->mode << 1));

	wid.id = WID_SET_DRV_HANDLER;
	wid.type = WID_STR;
	wid.val = (s8 *)buffer;
	wid.size = DRV_HANDLER_SIZE;

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   hif_drv->driver_handler_id);
	if (ret)
		PRINT_ER(vif->ndev, "Failed to set driver handler\n");
	kfree(buffer);

free_msg:
	if (msg->is_sync)
		complete(&msg->work_comp);
	kfree(msg);
}

static void handle_set_operation_mode(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct op_mode *hif_op_mode = &msg->body.mode;
	int ret;
	struct wid wid;

	wid.id = WID_SET_OPERATION_MODE;
	wid.type = WID_INT;
	wid.val = (s8 *)&hif_op_mode->mode;
	wid.size = sizeof(u32);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set operation mode\n");
	kfree(msg);
}

static void handle_get_mac_address(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct dev_mac_addr *dev_mac_addr = &msg->body.dev_mac_info;
	int ret;
	struct wid wid;

	wid.id = WID_MAC_ADDR;
	wid.type = WID_STR;
	wid.val = dev_mac_addr->mac_addr;
	wid.size = ETH_ALEN;

	ret = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to get mac address\n");
	complete(&msg->work_comp);
	/* free 'msg' in the caller */
}

static void handle_set_mac_address(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct dev_mac_addr *dev_mac_addr = &msg->body.dev_mac_info;
	int ret;
	struct wid wid;

	wid.id = WID_MAC_ADDR;
	wid.type = WID_STR;
	wid.val = dev_mac_addr->mac_addr;
	wid.size = ETH_ALEN;

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Failed to set mac address\n");
	complete(&msg->work_comp);
	/* free 'msg' data later, in caller */
}

static void handle_cfg_param(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct cfg_param_attr *param = &msg->body.cfg_info;
	int ret;
	struct wid wid_list[32];
	int i = 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting CFG params\n");

	if (param->flag & RETRY_SHORT) {
		wid_list[i].id = WID_SHORT_RETRY_LIMIT;
		wid_list[i].val = (s8 *)&param->short_retry_limit;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}
	if (param->flag & RETRY_LONG) {
		wid_list[i].id = WID_LONG_RETRY_LIMIT;
		wid_list[i].val = (s8 *)&param->long_retry_limit;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}
	if (param->flag & FRAG_THRESHOLD) {
		wid_list[i].id = WID_FRAG_THRESHOLD;
		wid_list[i].val = (s8 *)&param->frag_threshold;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}
	if (param->flag & RTS_THRESHOLD) {
		wid_list[i].id = WID_RTS_THRESHOLD;
		wid_list[i].val = (s8 *)&param->rts_threshold;
		wid_list[i].type = WID_SHORT;
		wid_list[i].size = sizeof(u16);
		i++;
	}

	ret = wilc_send_config_pkt(vif, SET_CFG, wid_list,
				   i, wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev, "Error in setting CFG params\n");

	kfree(msg);
}

static void handle_scan(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct scan_attr *scan_info = &msg->body.scan_info;
	struct wiphy *wiphy = vif->ndev->ieee80211_ptr->wiphy;
	struct wilc_priv *priv = wiphy_priv(wiphy);
	int result = 0;
	struct wid wid_list[5];
	u32 index = 0;
	u32 i;
	u8 *buffer;
	u8 valuesize = 0;
	u8 *hdn_ntwk_wid_val = NULL;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct hidden_network *hidden_net = &scan_info->hidden_network;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WLAN_IFC);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting SCAN params\n");
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Scanning: In [%d] state\n",
		   hif_drv->hif_state);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		result = -EFAULT;
		goto error;
	}
	hif_drv->usr_scan_req.scan_result = scan_info->result;
	hif_drv->usr_scan_req.arg = scan_info->arg;

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state != HOST_IF_IDLE &&
		    hif_drv_p2p->hif_state != HOST_IF_CONNECTED) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. P2P_IFC is in state [%d]\n",
				   hif_drv_p2p->hif_state);
			result = -EBUSY;
			goto error;
		}
	}

	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state != HOST_IF_IDLE &&
	    hif_drv_wlan->hif_state != HOST_IF_CONNECTED) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. WLAN_IFC is in state [%d]\n",
				   hif_drv_wlan->hif_state);
			result = -EBUSY;
			goto error;
		}
	}
	if (vif->connecting) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't do scan in (CONNECTING) state\n");
		result = -EBUSY;
		goto error;
	}
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	if (vif->obtaining_ip) {
		PRINT_ER(vif->ndev, "Don't do obss scan\n");
		result = -EBUSY;
		goto error;
	}
#endif

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting SCAN params\n");
	hif_drv->usr_scan_req.ch_cnt = 0;

	wid_list[index].id = WID_SSID_PROBE_REQ;
	wid_list[index].type = WID_STR;

	for (i = 0; i < hidden_net->n_ssids; i++)
		valuesize += ((hidden_net->net_info[i].ssid_len) + 1);
	hdn_ntwk_wid_val = kmalloc(valuesize + 1, GFP_KERNEL);
	wid_list[index].val = hdn_ntwk_wid_val;
	if (wid_list[index].val) {
		buffer = wid_list[index].val;

		*buffer++ = hidden_net->n_ssids;

		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "In Handle_ProbeRequest number of ssid %d\n",
			 hidden_net->n_ssids);
		for (i = 0; i < hidden_net->n_ssids; i++) {
			*buffer++ = hidden_net->net_info[i].ssid_len;
			memcpy(buffer, hidden_net->net_info[i].ssid,
			       hidden_net->net_info[i].ssid_len);
			buffer += hidden_net->net_info[i].ssid_len;
		}

		wid_list[index].size = (s32)(valuesize + 1);
		index++;
	}

	wid_list[index].id = WID_INFO_ELEMENT_PROBE;
	wid_list[index].type = WID_BIN_DATA;
	wid_list[index].val = scan_info->ies;
	wid_list[index].size = scan_info->ies_len;
	index++;

	wid_list[index].id = WID_SCAN_TYPE;
	wid_list[index].type = WID_CHAR;
	wid_list[index].size = sizeof(char);
	wid_list[index].val = (s8 *)&scan_info->type;
	index++;

	wid_list[index].id = WID_SCAN_CHANNEL_LIST;
	wid_list[index].type = WID_BIN_DATA;

	if (scan_info->ch_freq_list &&
	    scan_info->ch_list_len > 0) {
		int i;

		for (i = 0; i < scan_info->ch_list_len; i++) {
			if (scan_info->ch_freq_list[i] > 0)
				scan_info->ch_freq_list[i] -= 1;
		}
	}

	wid_list[index].val = scan_info->ch_freq_list;
	wid_list[index].size = scan_info->ch_list_len;
	index++;

	wid_list[index].id = WID_START_SCAN_REQ;
	wid_list[index].type = WID_CHAR;
	wid_list[index].size = sizeof(char);
	wid_list[index].val = (s8 *)&scan_info->src;
	index++;

    /*
     * Remove APs from shadow scan list which are
     * not in the requested scan channels list
     */
	filter_shadow_scan(priv, scan_info->ch_freq_list,
			   scan_info->ch_list_len);

	result = wilc_send_config_pkt(vif, SET_CFG, wid_list,
				      index,
				      wilc_get_vif_idx(vif));

	if (result)
		PRINT_ER(vif->ndev, "Failed to send scan parameters\n");

error:
	if (result) {
		del_timer(&hif_drv->scan_timer);
		handle_scan_done(vif, SCAN_EVENT_ABORTED);
	}

	kfree(scan_info->ch_freq_list);
	scan_info->ch_freq_list = NULL;

	kfree(scan_info->ies);
	scan_info->ies = NULL;
	kfree(scan_info->hidden_network.net_info);
	scan_info->hidden_network.net_info = NULL;

	kfree(hdn_ntwk_wid_val);

	kfree(msg);
}

s32 handle_scan_done(struct wilc_vif *vif, enum scan_event evt)
{
	s32 result = 0;
	u8 abort_running_scan;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct user_scan_req *scan_req;
	u8 null_bssid[6] = {0};

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "handling scan done\n");

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return result;
	}

	if (evt == SCAN_EVENT_DONE) {
		if (memcmp(hif_drv->assoc_bssid, null_bssid, ETH_ALEN) == 0)
			hif_drv->hif_state = HOST_IF_IDLE;
		else
			hif_drv->hif_state = HOST_IF_CONNECTED;
	} else if (evt == SCAN_EVENT_ABORTED) {
		PRINT_INFO(vif->ndev, GENERIC_DBG, "Abort running scan\n");
		abort_running_scan = 1;
		wid.id = WID_ABORT_RUNNING_SCAN;
		wid.type = WID_CHAR;
		wid.val = (s8 *)&abort_running_scan;
		wid.size = sizeof(char);

		result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));

		if (result) {
			PRINT_ER(vif->ndev, "Failed to set abort running\n");
			result = -EFAULT;
		}
	}

	scan_req = &hif_drv->usr_scan_req;
	if (scan_req->scan_result) {
		scan_req->scan_result(evt, NULL, scan_req->arg, NULL);
		scan_req->scan_result = NULL;
	}

	return result;
}

static void handle_connect(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct connect_attr *conn_attr = &msg->body.con_info;
	int result = 0;
	struct wid wid_list[8];
	u32 wid_cnt = 0, dummyval = 0;
	u8 *cur_byte = NULL;
	struct join_bss_param *bss_param;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		result = -EFAULT;
		goto error;
	}

	if (hif_drv->usr_scan_req.scan_result) {
		result = wilc_enqueue_work(msg);
		if (result)
			goto error;

		usleep_range(2 * 1000, 2 * 1000);
		return;
	}

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. P2P_IFC is in state [%d]\n",
			 hif_drv_p2p->hif_state);
			 result = -EFAULT;
			goto error;
		}
	}
	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Don't scan. WLAN_IFC is in state [%d]\n",
			 hif_drv_wlan->hif_state);
			result = -EFAULT;
			goto error;
		}
	}

	PRINT_D(vif->ndev, HOSTINF_DBG,
		"Saving connection parameters in global structure\n");
	bss_param = conn_attr->params;
	if (!bss_param) {
		PRINT_ER(vif->ndev, "Required BSSID not found\n");
		result = -ENOENT;
		goto error;
	}

	if (conn_attr->bssid) {
		hif_drv->usr_conn_req.bssid = kmemdup(conn_attr->bssid, 6,
						      GFP_KERNEL);
		if (!hif_drv->usr_conn_req.bssid) {
			result = -ENOMEM;
			goto error;
		}
	}

	hif_drv->usr_conn_req.ssid_len = conn_attr->ssid_len;
	if (conn_attr->ssid) {
		hif_drv->usr_conn_req.ssid = kmalloc(conn_attr->ssid_len + 1,
						     GFP_KERNEL);
		if (!hif_drv->usr_conn_req.ssid) {
			result = -ENOMEM;
			goto error;
		}
		memcpy(hif_drv->usr_conn_req.ssid,
		       conn_attr->ssid,
		       conn_attr->ssid_len);
		hif_drv->usr_conn_req.ssid[conn_attr->ssid_len] = '\0';
	}

	hif_drv->usr_conn_req.ies_len = conn_attr->ies_len;
	if (conn_attr->ies) {
		hif_drv->usr_conn_req.ies = kmemdup(conn_attr->ies,
						    conn_attr->ies_len,
						    GFP_KERNEL);
		if (!hif_drv->usr_conn_req.ies) {
			result = -ENOMEM;
			goto error;
		}
	}

	hif_drv->usr_conn_req.security = conn_attr->security;
	hif_drv->usr_conn_req.auth_type = conn_attr->auth_type;
	hif_drv->usr_conn_req.conn_result = conn_attr->result;
	hif_drv->usr_conn_req.arg = conn_attr->arg;

	wid_list[wid_cnt].id = WID_SUCCESS_FRAME_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)(&(dummyval));
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RECEIVED_FRAGMENT_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)(&(dummyval));
	wid_cnt++;

	wid_list[wid_cnt].id = WID_FAILED_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)(&(dummyval));
	wid_cnt++;

	wid_list[wid_cnt].id = WID_INFO_ELEMENT_ASSOCIATE;
	wid_list[wid_cnt].type = WID_BIN_DATA;
	wid_list[wid_cnt].val = hif_drv->usr_conn_req.ies;
	wid_list[wid_cnt].size = hif_drv->usr_conn_req.ies_len;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_11I_MODE;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&hif_drv->usr_conn_req.security;
	wid_cnt++;

	PRINT_D(vif->ndev, HOSTINF_DBG, "Encrypt Mode = %x\n",
		hif_drv->usr_conn_req.security);
	wid_list[wid_cnt].id = WID_AUTH_TYPE;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&hif_drv->usr_conn_req.auth_type;
	wid_cnt++;

	PRINT_D(vif->ndev, HOSTINF_DBG, "Authentication Type = %x\n",
		hif_drv->usr_conn_req.auth_type);
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Connecting to network of SSID %s on channel %d\n",
		 hif_drv->usr_conn_req.ssid, conn_attr->ch);

	wid_list[wid_cnt].id = WID_JOIN_REQ_EXTENDED;
	wid_list[wid_cnt].type = WID_STR;
	wid_list[wid_cnt].size = 112;
	wid_list[wid_cnt].val = kmalloc(wid_list[wid_cnt].size, GFP_KERNEL);

	if (!wid_list[wid_cnt].val) {
		result = -EFAULT;
		goto error;
	}

	cur_byte = wid_list[wid_cnt].val;

	if (conn_attr->ssid) {
		memcpy(cur_byte, conn_attr->ssid, conn_attr->ssid_len);
		cur_byte[conn_attr->ssid_len] = '\0';
	}
	cur_byte += MAX_SSID_LEN;
	*(cur_byte++) = INFRASTRUCTURE;

	if (conn_attr->ch >= 1 && conn_attr->ch <= 14) {
		*(cur_byte++) = conn_attr->ch;
	} else {
		PRINT_ER(vif->ndev, "Channel out of range\n");
		*(cur_byte++) = 0xFF;
	}
	*(cur_byte++)  = (bss_param->cap_info) & 0xFF;
	*(cur_byte++)  = ((bss_param->cap_info) >> 8) & 0xFF;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* Cap Info %0x*\n",
		   (*(cur_byte - 2) | ((*(cur_byte - 1)) << 8)));

	if (conn_attr->bssid)
		memcpy(cur_byte, conn_attr->bssid, 6);
	cur_byte += 6;

	if (conn_attr->bssid)
		memcpy(cur_byte, conn_attr->bssid, 6);
	cur_byte += 6;

	*(cur_byte++)  = (bss_param->beacon_period) & 0xFF;
	*(cur_byte++)  = ((bss_param->beacon_period) >> 8) & 0xFF;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* Beacon Period %d*\n",
		   *(cur_byte - 2) | ((*(cur_byte - 1)) << 8));
	*(cur_byte++)  =  bss_param->dtim_period;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* DTIM Period %d*\n",
		   *(cur_byte - 1));

	memcpy(cur_byte, bss_param->supp_rates, MAX_RATES_SUPPORTED + 1);
	cur_byte += (MAX_RATES_SUPPORTED + 1);

	*(cur_byte++)  =  bss_param->wmm_cap;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* wmm cap%d*\n", *(cur_byte - 1));
	*(cur_byte++)  = bss_param->uapsd_cap;

	*(cur_byte++)  = bss_param->ht_capable;
	hif_drv->usr_conn_req.ht_capable = bss_param->ht_capable;

	*(cur_byte++)  =  bss_param->rsn_found;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* rsn found %d*\n",
		   *(cur_byte - 1));
	*(cur_byte++)  =  bss_param->rsn_grp_policy;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* rsn group policy %0x*\n",
		   *(cur_byte - 1));
	*(cur_byte++) =  bss_param->mode_802_11i;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "* mode_802_11i %d*\n",
		   *(cur_byte - 1));
	memcpy(cur_byte, bss_param->rsn_pcip_policy,
	       sizeof(bss_param->rsn_pcip_policy));
	cur_byte += sizeof(bss_param->rsn_pcip_policy);

	memcpy(cur_byte, bss_param->rsn_auth_policy,
	       sizeof(bss_param->rsn_auth_policy));
	cur_byte += sizeof(bss_param->rsn_auth_policy);

	memcpy(cur_byte, bss_param->rsn_cap, sizeof(bss_param->rsn_cap));
	cur_byte += sizeof(bss_param->rsn_cap);

	*(cur_byte++) = REAL_JOIN_REQ;
	*(cur_byte++) = bss_param->noa_enabled;

	if (bss_param->noa_enabled) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "NOA present\n");
		*(cur_byte++) = (bss_param->tsf) & 0xFF;
		*(cur_byte++) = ((bss_param->tsf) >> 8) & 0xFF;
		*(cur_byte++) = ((bss_param->tsf) >> 16) & 0xFF;
		*(cur_byte++) = ((bss_param->tsf) >> 24) & 0xFF;

		*(cur_byte++) = bss_param->idx;
		*(cur_byte++) = bss_param->opp_enabled;

		if (bss_param->opp_enabled)
			*(cur_byte++) = bss_param->ct_window;

		*(cur_byte++) = bss_param->cnt;

		memcpy(cur_byte, bss_param->duration,
		       sizeof(bss_param->duration));
		cur_byte += sizeof(bss_param->duration);

		memcpy(cur_byte, bss_param->interval,
		       sizeof(bss_param->interval));
		cur_byte += sizeof(bss_param->interval);

		memcpy(cur_byte, bss_param->start_time,
		       sizeof(bss_param->start_time));
		cur_byte += sizeof(bss_param->start_time);
	} else {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "NOA not present\n");
	}

	cur_byte = wid_list[wid_cnt].val;
	wid_cnt++;

	PRINT_INFO(vif->ndev, GENERIC_DBG, "send HOST_IF_WAITING_CONN_RESP\n");

	result = wilc_send_config_pkt(vif, SET_CFG, wid_list,
				      wid_cnt,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "failed to send config packet\n");
		result = -EFAULT;
		goto error;
	} else {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "set HOST_IF_WAITING_CONN_RESP\n");
		hif_drv->hif_state = HOST_IF_WAITING_CONN_RESP;
	}

error:
	if (result) {
		struct connect_info conn_info;

		del_timer(&hif_drv->connect_timer);

		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "could not start connecting to the required network\n");
		memset(&conn_info, 0, sizeof(struct connect_info));

		if (conn_attr->result) {
			if (conn_attr->bssid)
				memcpy(conn_info.bssid, conn_attr->bssid, 6);

			if (conn_attr->ies) {
				conn_info.req_ies_len = conn_attr->ies_len;
				conn_info.req_ies = kmalloc(conn_attr->ies_len,
							    GFP_KERNEL);
				memcpy(conn_info.req_ies,
				       conn_attr->ies,
				       conn_attr->ies_len);
			}

			conn_attr->result(EVENT_CONN_RESP,
					  &conn_info, MAC_STATUS_DISCONNECTED,
					  NULL, conn_attr->arg);
			hif_drv->hif_state = HOST_IF_IDLE;
			kfree(conn_info.req_ies);
			conn_info.req_ies = NULL;

		} else {
			PRINT_ER(vif->ndev, "conn_result is NULL\n");
		}
	}

	kfree(conn_attr->bssid);
	conn_attr->bssid = NULL;

	kfree(conn_attr->ssid);
	conn_attr->ssid = NULL;

	kfree(conn_attr->ies);
	conn_attr->ies = NULL;

	kfree(cur_byte);
	kfree(msg);
}

static void handle_connect_timeout(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	int result;
	struct connect_info info;
	struct wid wid;
	u16 dummy_reason_code = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		goto out;
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	memset(&info, 0, sizeof(struct connect_info));

	if (hif_drv->usr_conn_req.conn_result) {
		if (hif_drv->usr_conn_req.bssid) {
			memcpy(info.bssid,
			       hif_drv->usr_conn_req.bssid, 6);
		}

		if (hif_drv->usr_conn_req.ies) {
			info.req_ies_len = hif_drv->usr_conn_req.ies_len;
			info.req_ies = kmemdup(hif_drv->usr_conn_req.ies,
					       hif_drv->usr_conn_req.ies_len,
					       GFP_KERNEL);
			if (!info.req_ies)
				goto out;
		}

		hif_drv->usr_conn_req.conn_result(EVENT_CONN_RESP,
						  &info,
						  MAC_STATUS_DISCONNECTED,
						  NULL,
						  hif_drv->usr_conn_req.arg);

		kfree(info.req_ies);
		info.req_ies = NULL;
	} else {
		PRINT_ER(vif->ndev, "conn_result is NULL\n");
	}

	wid.id = WID_DISCONNECT;
	wid.type = WID_CHAR;
	wid.val = (s8 *)&dummy_reason_code;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending disconnect request\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send disconect\n");

	hif_drv->usr_conn_req.ssid_len = 0;
	kfree(hif_drv->usr_conn_req.ssid);
	hif_drv->usr_conn_req.ssid = NULL;
	kfree(hif_drv->usr_conn_req.bssid);
	hif_drv->usr_conn_req.bssid = NULL;
	hif_drv->usr_conn_req.ies_len = 0;
	kfree(hif_drv->usr_conn_req.ies);
	hif_drv->usr_conn_req.ies = NULL;

out:
	kfree(msg);
}

static void host_int_fill_join_bss_param(struct join_bss_param *param, u8 *ies,
					 u16 *out_index, u8 *pcipher_tc,
					 u8 *auth_total_cnt, u32 tsf_lo,
					 u8 *rates_no)
{
	u8 ext_rates_no;
	u16 offset;
	u8 pcipher_cnt;
	u8 auth_cnt;
	u8 i, j;
	u16 index = *out_index;

	if (ies[index] == WLAN_EID_SUPP_RATES) {
		*rates_no = ies[index + 1];
		param->supp_rates[0] = *rates_no;
		index += 2;

		for (i = 0; i < *rates_no; i++)
			param->supp_rates[i + 1] = ies[index + i];

		index += *rates_no;
	} else if (ies[index] == WLAN_EID_EXT_SUPP_RATES) {
		ext_rates_no = ies[index + 1];
		if (ext_rates_no > (MAX_RATES_SUPPORTED - *rates_no))
			param->supp_rates[0] = MAX_RATES_SUPPORTED;
		else
			param->supp_rates[0] += ext_rates_no;
		index += 2;
		for (i = 0; i < (param->supp_rates[0] - *rates_no); i++)
			param->supp_rates[*rates_no + i + 1] = ies[index + i];

		index += ext_rates_no;
	} else if (ies[index] == WLAN_EID_HT_CAPABILITY) {
		param->ht_capable = true;
		index += ies[index + 1] + 2;
	} else if ((ies[index] == WLAN_EID_VENDOR_SPECIFIC) &&
		   (ies[index + 2] == 0x00) && (ies[index + 3] == 0x50) &&
		   (ies[index + 4] == 0xF2) && (ies[index + 5] == 0x02) &&
		   ((ies[index + 6] == 0x00) || (ies[index + 6] == 0x01)) &&
		   (ies[index + 7] == 0x01)) {
		param->wmm_cap = true;

		if (ies[index + 8] & BIT(7))
			param->uapsd_cap = true;
		index += ies[index + 1] + 2;
	} else if ((ies[index] == WLAN_EID_VENDOR_SPECIFIC) &&
		 (ies[index + 2] == 0x50) && (ies[index + 3] == 0x6f) &&
		 (ies[index + 4] == 0x9a) &&
		 (ies[index + 5] == 0x09) && (ies[index + 6] == 0x0c)) {
		u16 p2p_cnt;

		param->tsf = tsf_lo;
		param->noa_enabled = 1;
		param->idx = ies[index + 9];

		if (ies[index + 10] & BIT(7)) {
			param->opp_enabled = 1;
			param->ct_window = ies[index + 10];
		} else {
			param->opp_enabled = 0;
		}

		param->cnt = ies[index + 11];
		p2p_cnt = index + 12;

		memcpy(param->duration, ies + p2p_cnt, 4);
		p2p_cnt += 4;

		memcpy(param->interval, ies + p2p_cnt, 4);
		p2p_cnt += 4;

		memcpy(param->start_time, ies + p2p_cnt, 4);

		index += ies[index + 1] + 2;
	} else if ((ies[index] == WLAN_EID_RSN) ||
		 ((ies[index] == WLAN_EID_VENDOR_SPECIFIC) &&
		  (ies[index + 2] == 0x00) &&
		  (ies[index + 3] == 0x50) && (ies[index + 4] == 0xF2) &&
		  (ies[index + 5] == 0x01))) {
		u16 rsn_idx = index;

		if (ies[rsn_idx] == WLAN_EID_RSN) {
			param->mode_802_11i = 2;
		} else {
			if (param->mode_802_11i == 0)
				param->mode_802_11i = 1;
			rsn_idx += 4;
		}

		rsn_idx += 7;
		param->rsn_grp_policy = ies[rsn_idx];
		rsn_idx++;
		offset = ies[rsn_idx] * 4;
		pcipher_cnt = (ies[rsn_idx] > 3) ? 3 : ies[rsn_idx];
		rsn_idx += 2;

		i = *pcipher_tc;
		j = 0;
		for (; i < (pcipher_cnt + *pcipher_tc) && i < 3; i++, j++) {
			u8 *policy =  &param->rsn_pcip_policy[i];

			*policy = ies[rsn_idx + ((j + 1) * 4) - 1];
		}

		*pcipher_tc += pcipher_cnt;
		rsn_idx += offset;

		offset = ies[rsn_idx] * 4;

		auth_cnt = (ies[rsn_idx] > 3) ? 3 : ies[rsn_idx];
		rsn_idx += 2;
		i = *auth_total_cnt;
		j = 0;
		for (; i < (*auth_total_cnt + auth_cnt); i++, j++) {
			u8 *policy =  &param->rsn_auth_policy[i];

			*policy = ies[rsn_idx + ((j + 1) * 4) - 1];
		}

		*auth_total_cnt += auth_cnt;
		rsn_idx += offset;

		if (ies[index] == WLAN_EID_RSN) {
			param->rsn_cap[0] = ies[rsn_idx];
			param->rsn_cap[1] = ies[rsn_idx + 1];
			rsn_idx += 2;
		}
		param->rsn_found = true;
		index += ies[index + 1] + 2;
	} else {
		index += ies[index + 1] + 2;
	}

	*out_index = index;
}

static void *host_int_parse_join_bss_param(struct network_info *info)
{
	struct join_bss_param *param;
	u16 index = 0;
	u8 rates_no = 0;
	u8 pcipher_total_cnt = 0;
	u8 auth_total_cnt = 0;

	param = kzalloc(sizeof(*param), GFP_KERNEL);
	if (!param)
		return NULL;

	param->dtim_period = info->dtim_period;
	param->beacon_period = info->beacon_period;
	param->cap_info = info->cap_info;
	memcpy(param->bssid, info->bssid, 6);
	memcpy((u8 *)param->ssid, info->ssid, info->ssid_len + 1);
	param->ssid_len = info->ssid_len;
	memset(param->rsn_pcip_policy, 0xFF, 3);
	memset(param->rsn_auth_policy, 0xFF, 3);

	while (index < info->ies_len)
		host_int_fill_join_bss_param(param, info->ies, &index,
					     &pcipher_total_cnt,
					     &auth_total_cnt, info->tsf_lo,
					     &rates_no);

	return (void *)param;
}

static inline u8 *get_bssid(struct ieee80211_mgmt *mgmt)
{
	if (ieee80211_has_fromds(mgmt->frame_control))
		return mgmt->sa;
	else if (ieee80211_has_tods(mgmt->frame_control))
		return mgmt->da;
	else
		return mgmt->bssid;
}

static s32 wilc_parse_network_info(struct wilc_vif *vif, u8 *msg_buffer,
				   struct network_info **ret_network_info)
{
	struct network_info *info;
	struct ieee80211_mgmt *mgt;
	u8 *wid_val, *msa, *ies;
	u16 wid_len, rx_len, ies_len;
	u8 msg_type;
	size_t offset;
	const u8 *ch_elm, *tim_elm, *ssid_elm;

	msg_type = msg_buffer[0];
	if ('N' != msg_type)
		return -EFAULT;

	wid_len = get_unaligned_le16(&msg_buffer[6]);
	wid_val = &msg_buffer[8];

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->rssi = wid_val[0];

	msa = &wid_val[1];
	mgt = (struct ieee80211_mgmt *)&wid_val[1];
	rx_len = wid_len - 1;

	if (ieee80211_is_probe_resp(mgt->frame_control)) {
		info->cap_info = le16_to_cpu(mgt->u.probe_resp.capab_info);
		info->beacon_period = le16_to_cpu(mgt->u.probe_resp.beacon_int);
		info->tsf_hi = le64_to_cpu(mgt->u.probe_resp.timestamp);
		info->tsf_lo = (u32)info->tsf_hi;
		offset = offsetof(struct ieee80211_mgmt, u.probe_resp.variable);
	} else if (ieee80211_is_beacon(mgt->frame_control)) {
		info->cap_info = le16_to_cpu(mgt->u.beacon.capab_info);
		info->beacon_period = le16_to_cpu(mgt->u.beacon.beacon_int);
		info->tsf_hi = le64_to_cpu(mgt->u.beacon.timestamp);
		info->tsf_lo = (u32)info->tsf_hi;
		offset = offsetof(struct ieee80211_mgmt, u.beacon.variable);
	} else {
		/* only process probe response and beacon frame */
		kfree(info);
		return -EIO;
	}

	PRINT_INFO(vif->ndev, CORECONFIG_DBG, "TSF :%x\n", info->tsf_lo);

	ether_addr_copy(info->bssid, get_bssid(mgt));

	ies = mgt->u.beacon.variable;
	ies_len = rx_len - offset;
	if (ies_len <= 0) {
		kfree(info);
		return -EIO;
	}

	info->ies = kmemdup(ies, ies_len, GFP_KERNEL);
	if (!info->ies) {
		kfree(info);
		return -ENOMEM;
	}

	info->ies_len = ies_len;

	ssid_elm = cfg80211_find_ie(WLAN_EID_SSID, ies, ies_len);
	if (ssid_elm) {
		info->ssid_len = ssid_elm[1];
		if (info->ssid_len <= IEEE80211_MAX_SSID_LEN)
			memcpy(info->ssid, ssid_elm + 2, info->ssid_len);
		else
			info->ssid_len = 0;
	}

	ch_elm = cfg80211_find_ie(WLAN_EID_DS_PARAMS, ies, ies_len);
	if (ch_elm && ch_elm[1] > 0)
		info->ch = ch_elm[2];

	tim_elm = cfg80211_find_ie(WLAN_EID_TIM, ies, ies_len);
	if (tim_elm && tim_elm[1] >= 2)
		info->dtim_period = tim_elm[3];

	*ret_network_info = info;

	return 0;
}

static void handle_rcvd_ntwrk_info(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct rcvd_net_info *rcvd_info = &msg->body.net_info;
	u32 i;
	bool found;
	struct network_info *info = NULL;
	void *params;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct user_scan_req *scan_req = &hif_drv->usr_scan_req;
	int ret;

	found = true;
	PRINT_D(vif->ndev, HOSTINF_DBG, "Handling received network info\n");

	if (!scan_req->scan_result)
		goto done;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "State: Scanning, parsing network information received\n");
	ret = wilc_parse_network_info(vif, rcvd_info->buffer, &info);
	if (ret || !info || !scan_req->scan_result) {
		PRINT_ER(vif->ndev, "info or scan result NULL\n");
		goto done;
	}

	for (i = 0; i < scan_req->ch_cnt; i++) {
		if (memcmp(scan_req->net_info[i].bssid, info->bssid, 6) == 0) {
			if (info->rssi <= scan_req->net_info[i].rssi) {
				PRINT_INFO(vif->ndev, HOSTINF_DBG,
					   "Network previously discovered\n");
				goto done;
			} else {
				scan_req->net_info[i].rssi = info->rssi;
				found = false;
				break;
			}
		}
	}

	if (found) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "New network found\n");
		if (scan_req->ch_cnt < MAX_NUM_SCANNED_NETWORKS) {
			scan_req->net_info[scan_req->ch_cnt].rssi = info->rssi;

			memcpy(scan_req->net_info[scan_req->ch_cnt].bssid,
			       info->bssid, 6);

			scan_req->ch_cnt++;

			info->new_network = true;
			params = host_int_parse_join_bss_param(info);

			scan_req->scan_result(SCAN_EVENT_NETWORK_FOUND, info,
					       scan_req->arg, params);
		} else {
			PRINT_WRN(vif->ndev, HOSTINF_DBG,
				   "Discovered networks exceeded max. limit\n");
		}
	} else {
		info->new_network = false;
		scan_req->scan_result(SCAN_EVENT_NETWORK_FOUND, info,
				      scan_req->arg, NULL);
	}

done:
	kfree(rcvd_info->buffer);
	rcvd_info->buffer = NULL;

	if (info) {
		kfree(info->ies);
		kfree(info);
	}

	kfree(msg);
}

static void host_int_get_assoc_res_info(struct wilc_vif *vif,
				       u8 *assoc_resp_info,
				       u32 max_assoc_resp_info_len,
				       u32 *rcvd_assoc_resp_info_len)
{
	int result;
	struct wid wid;

	wid.id = WID_ASSOC_RES_INFO;
	wid.type = WID_STR;
	wid.val = assoc_resp_info;
	wid.size = max_assoc_resp_info_len;

	result = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		*rcvd_assoc_resp_info_len = 0;
		PRINT_ER(vif->ndev, "Failed to send association response\n");
		return;
	}

	*rcvd_assoc_resp_info_len = wid.size;
}

static inline void host_int_free_user_conn_req(struct host_if_drv *hif_drv)
{
	hif_drv->usr_conn_req.ssid_len = 0;
	kfree(hif_drv->usr_conn_req.ssid);
	hif_drv->usr_conn_req.ssid = NULL;
	kfree(hif_drv->usr_conn_req.bssid);
	hif_drv->usr_conn_req.bssid = NULL;
	hif_drv->usr_conn_req.ies_len = 0;
	kfree(hif_drv->usr_conn_req.ies);
	hif_drv->usr_conn_req.ies = NULL;
}

static s32 wilc_parse_assoc_resp_info(u8 *buffer, u32 buffer_len,
				      struct connect_info *ret_conn_info)
{
	u8 *ies;
	u16 ies_len;
	struct assoc_resp *res = (struct assoc_resp *)buffer;

	ret_conn_info->status = le16_to_cpu(res->status_code);
	if (ret_conn_info->status == WLAN_STATUS_SUCCESS) {
		ies = &buffer[sizeof(*res)];
		ies_len = buffer_len - sizeof(*res);

		ret_conn_info->resp_ies = kmemdup(ies, ies_len, GFP_KERNEL);
		if (!ret_conn_info->resp_ies)
			return -ENOMEM;

		ret_conn_info->resp_ies_len = ies_len;
	}

	return 0;
}
static inline void host_int_parse_assoc_resp_info(struct wilc_vif *vif,
						  u8 mac_status)
{
	struct connect_info conn_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	memset(&conn_info, 0, sizeof(struct connect_info));

	if (mac_status == MAC_STATUS_CONNECTED) {
		u32 assoc_resp_info_len;

		memset(hif_drv->assoc_resp, 0, MAX_ASSOC_RESP_FRAME_SIZE);

		host_int_get_assoc_res_info(vif, hif_drv->assoc_resp,
					    MAX_ASSOC_RESP_FRAME_SIZE,
					    &assoc_resp_info_len);

		PRINT_D(vif->ndev, HOSTINF_DBG,
			"Received association response = %d\n",
			assoc_resp_info_len);
		if (assoc_resp_info_len != 0) {
			s32 err = 0;

			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Parsing association response\n");
			err = wilc_parse_assoc_resp_info(hif_drv->assoc_resp,
							 assoc_resp_info_len,
							 &conn_info);
			if (err)
				PRINT_ER(vif->ndev,
					 "wilc_parse_assoc_resp_info() returned error %d\n",
					 err);
		}
	}

	if (hif_drv->usr_conn_req.bssid) {
		memcpy(conn_info.bssid, hif_drv->usr_conn_req.bssid, 6);

		if (mac_status == MAC_STATUS_CONNECTED &&
		    conn_info.status == WLAN_STATUS_SUCCESS) {
			memcpy(hif_drv->assoc_bssid,
			       hif_drv->usr_conn_req.bssid, ETH_ALEN);
		}
	}

	if (hif_drv->usr_conn_req.ies) {
		conn_info.req_ies = kmemdup(hif_drv->usr_conn_req.ies,
					    hif_drv->usr_conn_req.ies_len,
					    GFP_KERNEL);
		if (conn_info.req_ies)
			conn_info.req_ies_len = hif_drv->usr_conn_req.ies_len;
	}

	del_timer(&hif_drv->connect_timer);
	hif_drv->usr_conn_req.conn_result(EVENT_CONN_RESP,
					  &conn_info, mac_status, NULL,
					  hif_drv->usr_conn_req.arg);

	if (mac_status == MAC_STATUS_CONNECTED &&
	    conn_info.status == WLAN_STATUS_SUCCESS) {

		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "MAC status : CONNECTED and Connect Status : Successful\n");
		hif_drv->hif_state = HOST_IF_CONNECTED;

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
		handle_pwrsave_for_IP(vif, IP_STATE_OBTAINING);
#endif
	} else {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "MAC status : %d and Connect Status : %d\n",
			   mac_status, conn_info.status);
		hif_drv->hif_state = HOST_IF_IDLE;
	}

	kfree(conn_info.resp_ies);
	conn_info.resp_ies = NULL;

	kfree(conn_info.req_ies);
	conn_info.req_ies = NULL;
	host_int_free_user_conn_req(hif_drv);
}

static inline void host_int_handle_disconnect(struct wilc_vif *vif)
{
	struct disconnect_info disconn_info;
	struct host_if_drv *hif_drv = vif->hif_drv;
	wilc_connect_result conn_result = hif_drv->usr_conn_req.conn_result;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Received MAC_STATUS_DISCONNECTED from the FW\n");
	memset(&disconn_info, 0, sizeof(struct disconnect_info));

	if (hif_drv->usr_scan_req.scan_result) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "\n\n<< Abort the running OBSS Scan >>\n\n");
		del_timer(&hif_drv->scan_timer);
		handle_scan_done(vif, SCAN_EVENT_ABORTED);
	}

	disconn_info.reason = 0;
	disconn_info.ie = NULL;
	disconn_info.ie_len = 0;

	if (conn_result) {
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
		handle_pwrsave_for_IP(vif, IP_STATE_DEFAULT);
#endif

		conn_result(EVENT_DISCONN_NOTIF,
			    NULL, 0, &disconn_info, hif_drv->usr_conn_req.arg);
	} else {
		PRINT_ER(vif->ndev, "Connect result NULL\n");
	}

	eth_zero_addr(hif_drv->assoc_bssid);

	host_int_free_user_conn_req(hif_drv);
	hif_drv->hif_state = HOST_IF_IDLE;
}

static void handle_rcvd_gnrl_async_info(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct rcvd_async_info *rcvd_info = &msg->body.async_info;
	u8 msg_type;
	u8 mac_status;
	u8 mac_status_reason_code;
	u8 mac_status_additional_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!rcvd_info->buffer) {
		netdev_err(vif->ndev, "buffer is NULL\n");
		goto free_msg;
	}

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		goto free_rcvd_info;
	}
	PRINT_INFO(vif->ndev, GENERIC_DBG,
		   "Current State = %d,Received state = %d\n",
		   hif_drv->hif_state,
		   rcvd_info->buffer[7]);

	if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP ||
	    hif_drv->hif_state == HOST_IF_CONNECTED ||
	    hif_drv->usr_scan_req.scan_result) {
		if (!hif_drv->usr_conn_req.conn_result) {
			PRINT_ER(vif->ndev, "conn_result is NULL\n");
			goto free_rcvd_info;
		}

		msg_type = rcvd_info->buffer[0];

		if ('I' != msg_type) {
			PRINT_ER(vif->ndev, "Received Message incorrect.\n");
			goto free_rcvd_info;
		}

		mac_status  = rcvd_info->buffer[7];
		mac_status_reason_code = rcvd_info->buffer[8];
		mac_status_additional_info = rcvd_info->buffer[9];
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "Recieved MAC status= %d Reason= %d Info = %d\n",
			   mac_status, mac_status_reason_code,
			   mac_status_additional_info);
		if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP) {
			host_int_parse_assoc_resp_info(vif, mac_status);
		} else if ((mac_status == MAC_STATUS_DISCONNECTED) &&
			   (hif_drv->hif_state == HOST_IF_CONNECTED)) {
			host_int_handle_disconnect(vif);
		} else if ((mac_status == MAC_STATUS_DISCONNECTED) &&
			   (hif_drv->usr_scan_req.scan_result)) {
			PRINT_WRN(vif->ndev, HOSTINF_DBG,
				  "Received MAC_STATUS_DISCONNECTED. Abort the running Scan");
			del_timer(&hif_drv->scan_timer);
			if (hif_drv->usr_scan_req.scan_result)
				handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}

free_rcvd_info:
	kfree(rcvd_info->buffer);
	rcvd_info->buffer = NULL;

free_msg:
	kfree(msg);
}

static int wilc_pmksa_key_copy(struct wilc_vif *vif, struct key_attr *hif_key)
{
	int i;
	int ret;
	struct wid wid;
	u8 *key_buf;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling PMKSA key\n");
	key_buf = kmalloc((hif_key->attr.pmkid.numpmkid * PMKSA_KEY_LEN) + 1,
			  GFP_KERNEL);
	if (!key_buf) {
		PRINT_ER(vif->ndev, "No buffer to send PMKSA Key\n");
		return -ENOMEM;
	}

	key_buf[0] = hif_key->attr.pmkid.numpmkid;

	for (i = 0; i < hif_key->attr.pmkid.numpmkid; i++) {
		memcpy(key_buf + ((PMKSA_KEY_LEN * i) + 1),
		       hif_key->attr.pmkid.pmkidlist[i].bssid, ETH_ALEN);
		memcpy(key_buf + ((PMKSA_KEY_LEN * i) + ETH_ALEN + 1),
		       hif_key->attr.pmkid.pmkidlist[i].pmkid, PMKID_LEN);
	}

	wid.id = WID_PMKID_INFO;
	wid.type = WID_STR;
	wid.val = (s8 *)key_buf;
	wid.size = (hif_key->attr.pmkid.numpmkid * PMKSA_KEY_LEN) + 1;

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	kfree(key_buf);

	return ret;
}

static void handle_key(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct key_attr *hif_key = &msg->body.key_info;
	int result = 0;
	struct wid wid;
	struct wid wid_list[5];
	u8 *key_buf;
	struct host_if_drv *hif_drv = vif->hif_drv;

	switch (hif_key->type) {
	case WEP:

		if (hif_key->action & ADDKEY_AP) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Handling WEP key index: %d\n",
				   hif_key->attr.wep.index);
			wid_list[0].id = WID_11I_MODE;
			wid_list[0].type = WID_CHAR;
			wid_list[0].size = sizeof(char);
			wid_list[0].val = (s8 *)&hif_key->attr.wep.mode;

			wid_list[1].id = WID_AUTH_TYPE;
			wid_list[1].type = WID_CHAR;
			wid_list[1].size = sizeof(char);
			wid_list[1].val = (s8 *)&hif_key->attr.wep.auth_type;

			key_buf = kmalloc(hif_key->attr.wep.key_len + 2,
					  GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send Key\n");
				result = -ENOMEM;
				goto out_wep;
			}

			key_buf[0] = hif_key->attr.wep.index;
			key_buf[1] = hif_key->attr.wep.key_len;

			memcpy(&key_buf[2], hif_key->attr.wep.key,
			       hif_key->attr.wep.key_len);

			wid_list[2].id = WID_WEP_KEY_VALUE;
			wid_list[2].type = WID_STR;
			wid_list[2].size = hif_key->attr.wep.key_len + 2;
			wid_list[2].val = (s8 *)key_buf;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      wid_list, 3,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
		} else if (hif_key->action & ADDKEY) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Handling WEP key\n");
			key_buf = kmalloc(hif_key->attr.wep.key_len + 2,
					  GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev, "No buffer to send Key\n");
				result = -ENOMEM;
				goto out_wep;
			}
			key_buf[0] = hif_key->attr.wep.index;
			memcpy(key_buf + 1, &hif_key->attr.wep.key_len, 1);
			memcpy(key_buf + 2, hif_key->attr.wep.key,
			       hif_key->attr.wep.key_len);

			wid.id = WID_ADD_WEP_KEY;
			wid.type = WID_STR;
			wid.val = (s8 *)key_buf;
			wid.size = hif_key->attr.wep.key_len + 2;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
		} else if (hif_key->action & REMOVEKEY) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG, "Removing key\n");
			wid.id = WID_REMOVE_WEP_KEY;
			wid.type = WID_STR;

			wid.val = (s8 *)&hif_key->attr.wep.index;
			wid.size = 1;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
		} else if (hif_key->action & DEFAULTKEY) {
			wid.id = WID_KEY_ID;
			wid.type = WID_CHAR;
			wid.val = (s8 *)&hif_key->attr.wep.index;
			wid.size = sizeof(char);

			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Setting default key index\n");
			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
		}
out_wep:
		complete(&msg->work_comp);
		break;

	case WPA_RX_GTK:
		if (hif_key->action & ADDKEY_AP) {
			key_buf = kzalloc(RX_MIC_KEY_MSG_LEN, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev,
					 "No buffer to send RxGTK Key\n");
				result = -ENOMEM;
				goto out_wpa_rx_gtk;
			}

			if (hif_key->attr.wpa.seq)
				memcpy(key_buf + 6, hif_key->attr.wpa.seq, 8);

			memcpy(key_buf + 14, &hif_key->attr.wpa.index, 1);
			memcpy(key_buf + 15, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 16, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid_list[0].id = WID_11I_MODE;
			wid_list[0].type = WID_CHAR;
			wid_list[0].size = sizeof(char);
			wid_list[0].val = (s8 *)&hif_key->attr.wpa.mode;

			wid_list[1].id = WID_ADD_RX_GTK;
			wid_list[1].type = WID_STR;
			wid_list[1].val = (s8 *)key_buf;
			wid_list[1].size = RX_MIC_KEY_MSG_LEN;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      wid_list, 2,
						      wilc_get_vif_idx(vif));

			kfree(key_buf);
		} else if (hif_key->action & ADDKEY) {
			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "Handling group key(Rx) function\n");
			key_buf = kzalloc(RX_MIC_KEY_MSG_LEN, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev,
					 "No buffer to send RxGTK Key\n");
				result = -ENOMEM;
				goto out_wpa_rx_gtk;
			}

			if (hif_drv->hif_state == HOST_IF_CONNECTED)
				memcpy(key_buf, hif_drv->assoc_bssid, ETH_ALEN);
			else
				PRINT_ER(vif->ndev, "Couldn't handle\n");

			memcpy(key_buf + 6, hif_key->attr.wpa.seq, 8);
			memcpy(key_buf + 14, &hif_key->attr.wpa.index, 1);
			memcpy(key_buf + 15, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 16, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid.id = WID_ADD_RX_GTK;
			wid.type = WID_STR;
			wid.val = (s8 *)key_buf;
			wid.size = RX_MIC_KEY_MSG_LEN;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));

			kfree(key_buf);
		}
out_wpa_rx_gtk:
		complete(&msg->work_comp);
		break;

	case WPA_PTK:
		if (hif_key->action & ADDKEY_AP) {
			key_buf = kmalloc(PTK_KEY_MSG_LEN + 1, GFP_KERNEL);
			if (!key_buf) {
				PRINT_ER(vif->ndev,
					 "No buffer to send PTK Key\n");
				result = -ENOMEM;
				goto out_wpa_ptk;
			}

			memcpy(key_buf, hif_key->attr.wpa.mac_addr, 6);
			memcpy(key_buf + 6, &hif_key->attr.wpa.index, 1);
			memcpy(key_buf + 7, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 8, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid_list[0].id = WID_11I_MODE;
			wid_list[0].type = WID_CHAR;
			wid_list[0].size = sizeof(char);
			wid_list[0].val = (s8 *)&hif_key->attr.wpa.mode;

			wid_list[1].id = WID_ADD_PTK;
			wid_list[1].type = WID_STR;
			wid_list[1].val = (s8 *)key_buf;
			wid_list[1].size = PTK_KEY_MSG_LEN + 1;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      wid_list, 2,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
		} else if (hif_key->action & ADDKEY) {
			key_buf = kmalloc(PTK_KEY_MSG_LEN, GFP_KERNEL);
			if (!key_buf) {
				result = -ENOMEM;
				goto out_wpa_ptk;
			}

			memcpy(key_buf, hif_key->attr.wpa.mac_addr, 6);
			memcpy(key_buf + 6, &hif_key->attr.wpa.key_len, 1);
			memcpy(key_buf + 7, hif_key->attr.wpa.key,
			       hif_key->attr.wpa.key_len);

			wid.id = WID_ADD_PTK;
			wid.type = WID_STR;
			wid.val = (s8 *)key_buf;
			wid.size = PTK_KEY_MSG_LEN;

			result = wilc_send_config_pkt(vif, SET_CFG,
						      &wid, 1,
						      wilc_get_vif_idx(vif));
			kfree(key_buf);
		}

out_wpa_ptk:
		complete(&msg->work_comp);
		break;

	case PMKSA:
		result = wilc_pmksa_key_copy(vif, hif_key);
		/*free 'msg', this case it not a sync call*/
		kfree(msg);
		break;
	}

	if (result)
		PRINT_ER(vif->ndev, "Failed to send key config packet\n");

	/* free 'msg' data in caller sync call */
}

static void handle_disconnect(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct disconnect_info disconn_info;
	struct user_scan_req *scan_req;
	struct user_conn_req *conn_req;
	int result;
	u16 dummy_reason_code = 0;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		goto out;
	}

	if (hif_drv_wlan != NULL)	{
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Abort Scan. WLAN_IFC is in state [%d]\n",
				   hif_drv_wlan->hif_state);
			del_timer(&(hif_drv_wlan->scan_timer));
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}
	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "Abort Scan. P2P_IFC is in state [%d]\n",
				   hif_drv_p2p->hif_state);
			del_timer(&(hif_drv_p2p->scan_timer));
			handle_scan_done(vif, SCAN_EVENT_ABORTED);
		}
	}
	wid.id = WID_DISCONNECT;
	wid.type = WID_CHAR;
	wid.val = (s8 *)&dummy_reason_code;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Sending disconnect request\n");

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	handle_pwrsave_for_IP(vif, IP_STATE_DEFAULT);
#endif

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));

	if (result) {
		PRINT_ER(vif->ndev, "Failed to send dissconect\n");
		goto out;
	}

	memset(&disconn_info, 0, sizeof(struct disconnect_info));

	disconn_info.reason = 0;
	disconn_info.ie = NULL;
	disconn_info.ie_len = 0;
	scan_req = &hif_drv->usr_scan_req;
	conn_req = &hif_drv->usr_conn_req;

	if (scan_req->scan_result) {
		del_timer(&hif_drv->scan_timer);
		scan_req->scan_result(SCAN_EVENT_ABORTED, NULL, scan_req->arg,
				      NULL);
		scan_req->scan_result = NULL;
	}

	if (conn_req->conn_result) {
		if (hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP) {
			struct connect_info connect;

			PRINT_INFO(vif->ndev, HOSTINF_DBG,
				   "supplicant requested disconnection\n");
			memset(&connect, 0, sizeof(struct connect_info));
			del_timer(&hif_drv->connect_timer);
			if (conn_req->bssid != NULL)
				memcpy(connect.bssid, conn_req->bssid, 6);
			if (conn_req->ies != NULL) {
				connect.req_ies_len = conn_req->ies_len;
				connect.req_ies = kmalloc(conn_req->ies_len,
							  GFP_ATOMIC);
				memcpy(connect.req_ies,
				       conn_req->ies,
				       conn_req->ies_len);
			}
			conn_req->conn_result(EVENT_CONN_RESP,
					      &connect,
					      MAC_STATUS_DISCONNECTED, NULL,
					      conn_req->arg);

			if (connect.req_ies != NULL) {
				kfree(connect.req_ies);
				connect.req_ies = NULL;
			}

		} else if (hif_drv->hif_state == HOST_IF_CONNECTED) {
			conn_req->conn_result(EVENT_DISCONN_NOTIF,
					      NULL, 0, &disconn_info,
					      conn_req->arg);
		}
	} else {
		PRINT_ER(vif->ndev, "conn_result = NULL\n");
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	eth_zero_addr(hif_drv->assoc_bssid);

	conn_req->ssid_len = 0;
	kfree(conn_req->ssid);
	conn_req->ssid = NULL;
	kfree(conn_req->bssid);
	conn_req->bssid = NULL;
	conn_req->ies_len = 0;
	kfree(conn_req->ies);
	conn_req->ies = NULL;

out:

	complete(&msg->work_comp);
	/* free 'msg' in caller after receiving completion */
}

void wilc_resolve_disconnect_aberration(struct wilc_vif *vif)
{
	if (!vif->hif_drv)
		return;
	if (vif->hif_drv->hif_state == HOST_IF_WAITING_CONN_RESP ||
	    vif->hif_drv->hif_state == HOST_IF_CONNECTING) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG,
			   "\n\n<< correcting Supplicant state machine >>\n\n");
		wilc_disconnect(vif, 1);
	}
}

static void handle_get_rssi(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	int result;
	struct wid wid;

	wid.id = WID_RSSI;
	wid.type = WID_CHAR;
	wid.val = msg->body.data;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Getting RSSI value\n");
	result = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "Failed to get RSSI value\n");
		*msg->body.data = INVALID_RSSI;
	}

	complete(&msg->work_comp);
	/* free 'msg' data in caller */
}

static void handle_get_statistics(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct wid wid_list[5];
	u32 wid_cnt = 0, result;
	struct rf_info *stats = (struct rf_info *)msg->body.data;

	wid_list[wid_cnt].id = WID_LINKSPEED;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&stats->link_speed;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RSSI;
	wid_list[wid_cnt].type = WID_CHAR;
	wid_list[wid_cnt].size = sizeof(char);
	wid_list[wid_cnt].val = (s8 *)&stats->rssi;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_SUCCESS_FRAME_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->tx_cnt;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_RECEIVED_FRAGMENT_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->rx_cnt;
	wid_cnt++;

	wid_list[wid_cnt].id = WID_FAILED_COUNT;
	wid_list[wid_cnt].type = WID_INT;
	wid_list[wid_cnt].size = sizeof(u32);
	wid_list[wid_cnt].val = (s8 *)&stats->tx_fail_cnt;
	wid_cnt++;

	result = wilc_send_config_pkt(vif, GET_CFG, wid_list,
				      wid_cnt,
				      wilc_get_vif_idx(vif));

	if (result)
		PRINT_ER(vif->ndev, "Failed to send scan parameters\n");

	if (stats->link_speed > TCP_ACK_FILTER_LINK_SPEED_THRESH &&
	    stats->link_speed != DEFAULT_LINK_SPEED) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Enable TCP filter\n");
		wilc_enable_tcp_ack_filter(vif, true);
	} else if (stats->link_speed != DEFAULT_LINK_SPEED) {
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Disable TCP filter %d\n",
			   stats->link_speed);
		wilc_enable_tcp_ack_filter(vif, false);
	}

	/* free 'msg' for async command, for sync caller will free it */
	if (msg->is_sync)
		complete(&msg->work_comp);
	else
		kfree(msg);
}

static void handle_get_inactive_time(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct sta_inactive_t *hif_sta_inactive = &msg->body.mac_info;
	int result;
	struct wid wid;

	wid.id = WID_SET_STA_MAC_INACTIVE_TIME;
	wid.type = WID_STR;
	wid.size = ETH_ALEN;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto out;

	ether_addr_copy(wid.val, hif_sta_inactive->mac);

	PRINT_INFO(vif->ndev, CFG80211_DBG, "SETING STA inactive time\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);

	if (result) {
		PRINT_ER(vif->ndev, "Failed to SET inactive time\n");
		goto out;
	}

	wid.id = WID_GET_INACTIVE_TIME;
	wid.type = WID_INT;
	wid.val = (s8 *)&hif_sta_inactive->inactive_time;
	wid.size = sizeof(u32);

	result = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));

	if (result)
		PRINT_ER(vif->ndev, "Failed to get inactive time\n");

	PRINT_INFO(vif->ndev, CFG80211_DBG, "Getting inactive time : %d\n",
		   hif_sta_inactive->inactive_time);
out:
	/* free 'msg' data in caller */
	complete(&msg->work_comp);
}

static void handle_add_beacon(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct beacon_attr *param = &msg->body.beacon_info;
	int result;
	struct wid wid;
	u8 *cur_byte;

	wid.id = WID_ADD_BEACON;
	wid.type = WID_BIN;
	wid.size = param->head_len + param->tail_len + 16;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	*cur_byte++ = (param->interval & 0xFF);
	*cur_byte++ = ((param->interval >> 8) & 0xFF);
	*cur_byte++ = ((param->interval >> 16) & 0xFF);
	*cur_byte++ = ((param->interval >> 24) & 0xFF);

	*cur_byte++ = (param->dtim_period & 0xFF);
	*cur_byte++ = ((param->dtim_period >> 8) & 0xFF);
	*cur_byte++ = ((param->dtim_period >> 16) & 0xFF);
	*cur_byte++ = ((param->dtim_period >> 24) & 0xFF);

	*cur_byte++ = (param->head_len & 0xFF);
	*cur_byte++ = ((param->head_len >> 8) & 0xFF);
	*cur_byte++ = ((param->head_len >> 16) & 0xFF);
	*cur_byte++ = ((param->head_len >> 24) & 0xFF);

	memcpy(cur_byte, param->head, param->head_len);
	cur_byte += param->head_len;

	*cur_byte++ = (param->tail_len & 0xFF);
	*cur_byte++ = ((param->tail_len >> 8) & 0xFF);
	*cur_byte++ = ((param->tail_len >> 16) & 0xFF);
	*cur_byte++ = ((param->tail_len >> 24) & 0xFF);

	if (param->tail)
		memcpy(cur_byte, param->tail, param->tail_len);
	cur_byte += param->tail_len;

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add beacon\n");

error:
	kfree(wid.val);
	kfree(param->head);
	kfree(param->tail);
	kfree(msg);
}

static void handle_del_beacon(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	int result;
	struct wid wid;
	u8 del_beacon = 0;

	wid.id = WID_DEL_BEACON;
	wid.type = WID_CHAR;
	wid.size = sizeof(char);
	wid.val = &del_beacon;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Deleting BEACON\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send delete beacon\n");
	kfree(msg);
}

static u32 wilc_hif_pack_sta_param(struct wilc_vif *vif, u8 *buff,
				    struct add_sta_param *param)
{
	u8 *cur_byte;

	cur_byte = buff;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Packing STA params\n");
	memcpy(cur_byte, param->bssid, ETH_ALEN);
	cur_byte +=  ETH_ALEN;

	*cur_byte++ = param->aid & 0xFF;
	*cur_byte++ = (param->aid >> 8) & 0xFF;

	*cur_byte++ = param->rates_len;
	if (param->rates_len > 0)
		memcpy(cur_byte, param->rates, param->rates_len);
	cur_byte += param->rates_len;

	*cur_byte++ = param->ht_supported;
	memcpy(cur_byte, &param->ht_capa, sizeof(struct ieee80211_ht_cap));
	cur_byte += sizeof(struct ieee80211_ht_cap);

	*cur_byte++ = param->flags_mask & 0xFF;
	*cur_byte++ = (param->flags_mask >> 8) & 0xFF;

	*cur_byte++ = param->flags_set & 0xFF;
	*cur_byte++ = (param->flags_set >> 8) & 0xFF;

	return cur_byte - buff;
}

static void handle_add_station(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct add_sta_param *param = &msg->body.add_sta_info;
	int result;
	struct wid wid;
	u8 *cur_byte;

	wid.id = WID_ADD_STA;
	wid.type = WID_BIN;
	wid.size = WILC_ADD_STA_LENGTH + param->rates_len;

	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	cur_byte += wilc_hif_pack_sta_param(vif, cur_byte, param);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result != 0)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

error:
	kfree(param->rates);
	kfree(wid.val);
	kfree(msg);
}

static void handle_del_all_sta(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct del_all_sta *param = &msg->body.del_all_sta_info;
	int result;
	struct wid wid;
	u8 *curr_byte;
	u8 i;
	u8 zero_buff[6] = {0};

	wid.id = WID_DEL_ALL_STA;
	wid.type = WID_STR;
	wid.size = (param->assoc_sta * ETH_ALEN) + 1;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling delete station\n");
	wid.val = kmalloc((param->assoc_sta * ETH_ALEN) + 1, GFP_KERNEL);
	if (!wid.val)
		goto error;

	curr_byte = wid.val;

	*(curr_byte++) = param->assoc_sta;

	for (i = 0; i < MAX_NUM_STA; i++) {
		if (memcmp(param->del_all_sta[i], zero_buff, ETH_ALEN))
			memcpy(curr_byte, param->del_all_sta[i], ETH_ALEN);
		else
			continue;

		curr_byte += ETH_ALEN;
	}

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

error:
	kfree(wid.val);

	/* free 'msg' data in caller */
	complete(&msg->work_comp);
}

static void handle_del_station(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct del_sta *param = &msg->body.del_sta_info;
	int result;
	struct wid wid;

	wid.id = WID_REMOVE_STA;
	wid.type = WID_BIN;
	wid.size = ETH_ALEN;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling delete station\n");
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	ether_addr_copy(wid.val, param->mac_addr);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send add station\n");

error:
	kfree(wid.val);
	kfree(msg);
}

static void handle_edit_station(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct add_sta_param *param = &msg->body.edit_sta_info;
	int result;
	struct wid wid;
	u8 *cur_byte;

	wid.id = WID_EDIT_STA;
	wid.type = WID_BIN;
	wid.size = WILC_ADD_STA_LENGTH + param->rates_len;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling edit station\n");
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	cur_byte += wilc_hif_pack_sta_param(vif, cur_byte, param);

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send edit station\n");

error:
	kfree(param->rates);
	kfree(wid.val);
	kfree(msg);
}

static int handle_remain_on_chan(struct wilc_vif *vif,
				 struct remain_ch *hif_remain_ch)
{
	int result;
	u8 remain_on_chan_flag;
	struct wid wid;
	struct host_if_drv *hif_drv = vif->hif_drv;
	struct host_if_drv *hif_drv_p2p = get_drv_hndl_by_ifc(vif->wilc,
							      P2P_IFC);
	struct host_if_drv *hif_drv_wlan = get_drv_hndl_by_ifc(vif->wilc,
							       WLAN_IFC);

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "Driver is null\n");
		return -EFAULT;
	}

	if (!hif_drv->remain_on_ch_pending) {
		hif_drv->remain_on_ch.arg = hif_remain_ch->arg;
		hif_drv->remain_on_ch.expired = hif_remain_ch->expired;
		hif_drv->remain_on_ch.ready = hif_remain_ch->ready;
		hif_drv->remain_on_ch.ch = hif_remain_ch->ch;
		hif_drv->remain_on_ch.id = hif_remain_ch->id;
	} else {
		hif_remain_ch->ch = hif_drv->remain_on_ch.ch;
	}

	if (hif_drv_p2p != NULL) {
		if (hif_drv_p2p->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy scanning P2P_IFC state %d\n",
				   hif_drv_p2p->hif_state);
			hif_drv->remain_on_ch_pending = 1;
			result = -EBUSY;
			goto error;
		} else if ((hif_drv_p2p->hif_state != HOST_IF_IDLE) &&
		(hif_drv_p2p->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy connecting. P2P_IFC state %d\n",
				   hif_drv_p2p->hif_state);
			result = -EBUSY;
			goto error;
		}
	}
	if (hif_drv_wlan != NULL) {
		if (hif_drv_wlan->hif_state == HOST_IF_SCANNING) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy scanning. WLAN_IFC state %d\n",
				   hif_drv_wlan->hif_state);
			hif_drv->remain_on_ch_pending = 1;
			result = -EBUSY;
			goto error;
		} else if ((hif_drv_wlan->hif_state != HOST_IF_IDLE) &&
		(hif_drv_wlan->hif_state != HOST_IF_CONNECTED)) {
			PRINT_INFO(vif->ndev, GENERIC_DBG,
				   "IFC busy connecting. WLAN_IFC %d\n",
				   hif_drv_wlan->hif_state);
			result = -EBUSY;
			goto error;
		}
	}

	if (vif->connecting) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't do scan in (CONNECTING) state\n");
		result = -EBUSY;
		goto error;
	}
#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	if (vif->obtaining_ip) {
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't obss scan until IP adresss is obtained\n");
		result = -EBUSY;
		goto error;
	}
#endif

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setting channel :%d\n",
		   hif_remain_ch->ch);
	remain_on_chan_flag = true;
	wid.id = WID_REMAIN_ON_CHAN;
	wid.type = WID_STR;
	wid.size = 2;
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val) {
		result = -ENOMEM;
		goto error;
	}

	wid.val[0] = remain_on_chan_flag;
	wid.val[1] = (s8)hif_remain_ch->ch;

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);
	if (result != 0)
		PRINT_ER(vif->ndev, "Failed to set remain on channel\n");

	hif_drv->hif_state = HOST_IF_P2P_LISTEN;
error:

	hif_drv->remain_on_ch_timer_vif = vif;
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
	hif_drv->remain_on_ch_timer.data = (unsigned long)hif_drv;
#endif
	mod_timer(&hif_drv->remain_on_ch_timer,
			  jiffies +
			  msecs_to_jiffies(hif_remain_ch->duration));

	if (hif_drv->remain_on_ch.ready)
		hif_drv->remain_on_ch.ready(hif_drv->remain_on_ch.arg);

	if (hif_drv->remain_on_ch_pending)
		hif_drv->remain_on_ch_pending = 0;

	return result;
}

static void handle_register_frame(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct reg_frame *hif_reg_frame = &msg->body.reg_frame;
	int result;
	struct wid wid;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Handling frame register Flag : %d FrameType: %d\n",
		   hif_reg_frame->reg, hif_reg_frame->frame_type);
	wid.id = WID_REGISTER_FRAME;
	wid.type = WID_STR;
	wid.val = kmalloc(sizeof(u16) + 2, GFP_KERNEL);
	if (!wid.val)
		goto out;

	cur_byte = wid.val;

	*cur_byte++ = hif_reg_frame->reg;
	*cur_byte++ = hif_reg_frame->reg_id;
	memcpy(cur_byte, &hif_reg_frame->frame_type, sizeof(u16));

	wid.size = sizeof(u16) + 2;

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	kfree(wid.val);
	if (result)
		PRINT_ER(vif->ndev, "Failed to frame register\n");

out:
	kfree(msg);
}

static void handle_listen_state_expired(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct remain_ch *hif_remain_ch = &msg->body.remain_on_ch;
	u8 remain_on_chan_flag;
	struct wid wid;
	int result;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 null_bssid[6] = {0};

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "CANCEL REMAIN ON CHAN\n");

	if (hif_drv->hif_state == HOST_IF_P2P_LISTEN) {
		remain_on_chan_flag = false;
		wid.id = WID_REMAIN_ON_CHAN;
		wid.type = WID_STR;
		wid.size = 2;
		wid.val = kmalloc(wid.size, GFP_KERNEL);

		if (!wid.val) {
			PRINT_ER(vif->ndev, "Failed to allocate memory\n");
			goto free_msg;
		}

		wid.val[0] = remain_on_chan_flag;
		wid.val[1] = FALSE_FRMWR_CHANNEL;

		result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
					      wilc_get_vif_idx(vif));
		kfree(wid.val);
		if (result != 0) {
			PRINT_ER(vif->ndev, "Failed to set remain channel\n");
			goto free_msg;
		}

		if (hif_drv->remain_on_ch.expired)
			hif_drv->remain_on_ch.expired(hif_drv->remain_on_ch.arg,
						      hif_remain_ch->id);

		if (memcmp(hif_drv->assoc_bssid, null_bssid, ETH_ALEN) == 0)
			hif_drv->hif_state = HOST_IF_IDLE;
		else
			hif_drv->hif_state = HOST_IF_CONNECTED;
	} else {
		PRINT_D(vif->ndev, GENERIC_DBG,  "Not in listen state\n");
	}

free_msg:
	kfree(msg);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void listen_timer_cb(struct timer_list *t)
#else
static void listen_timer_cb(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct host_if_drv *hif_drv = from_timer(hif_drv, t,
						      remain_on_ch_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->remain_on_ch_timer_vif;
	int result;
	struct host_if_msg *msg;

	del_timer(&vif->hif_drv->remain_on_ch_timer);

	msg = wilc_alloc_work(vif, handle_listen_state_expired, false);
	if (IS_ERR(msg))
		return;

	msg->body.remain_on_ch.id = vif->hif_drv->remain_on_ch.id;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "wilc_mq_send fail\n");
		kfree(msg);
	}
}

static void handle_power_management(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct power_mgmt_param *pm_param = &msg->body.pwr_mgmt_info;
	int result;
	struct wid wid;
	s8 power_mode;

	wid.id = WID_POWER_MANAGEMENT;

	if (pm_param->enabled)
		power_mode = MIN_FAST_PS;
	else
		power_mode = NO_POWERSAVE;
	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling power mgmt to %d\n",
		   power_mode);
	wid.val = &power_mode;
	wid.size = sizeof(char);

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Handling Power Management\n");
	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result) {
		PRINT_ER(vif->ndev, "Failed to send power management\n");
		goto out;
	}
	store_power_save_current_state(vif, power_mode);
out:
	kfree(msg);
}

static void handle_set_mcast_filter(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	struct set_multicast *hif_set_mc = &msg->body.multicast_info;
	int result;
	struct wid wid;
	u8 *cur_byte;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "Setup Multicast Filter\n");

	wid.id = WID_SETUP_MULTICAST_FILTER;
	wid.type = WID_BIN;
	wid.size = sizeof(struct set_multicast) + (hif_set_mc->cnt * ETH_ALEN);
	wid.val = kmalloc(wid.size, GFP_KERNEL);
	if (!wid.val)
		goto error;

	cur_byte = wid.val;
	*cur_byte++ = (hif_set_mc->enabled & 0xFF);
	*cur_byte++ = 0;
	*cur_byte++ = 0;
	*cur_byte++ = 0;

	*cur_byte++ = (hif_set_mc->cnt & 0xFF);
	*cur_byte++ = ((hif_set_mc->cnt >> 8) & 0xFF);
	*cur_byte++ = ((hif_set_mc->cnt >> 16) & 0xFF);
	*cur_byte++ = ((hif_set_mc->cnt >> 24) & 0xFF);

	if (hif_set_mc->cnt > 0 && hif_set_mc->mc_list)
		memcpy(cur_byte, hif_set_mc->mc_list,
		       ((hif_set_mc->cnt) * ETH_ALEN));

	result = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				      wilc_get_vif_idx(vif));
	if (result)
		PRINT_ER(vif->ndev, "Failed to send setup multicast\n");

error:
	kfree(hif_set_mc->mc_list);
	kfree(wid.val);
	kfree(msg);
}

static void handle_set_wowlan_trigger(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	int ret;
	struct wid wid;

	wid.id = WID_WOWLAN_TRIGGER;
	wid.type = WID_CHAR;
	wid.val = &msg->body.wow_trigger.wowlan_trigger;
	wid.size = sizeof(s8);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));

	if (ret)
		PRINT_ER(vif->ndev,
			 "Failed to send wowlan trigger config packet\n");
	kfree(msg);
}

static void handle_set_tx_pwr(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	u8 tx_pwr = msg->body.tx_power.tx_pwr;
	int ret;
	struct wid wid;

	wid.id = WID_TX_POWER;
	wid.type = WID_CHAR;
	wid.val = &tx_pwr;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if (ret)
		PRINT_ER(vif->ndev, "Failed to set TX PWR\n");
	kfree(msg);
}

/* Note: 'msg' will be free after using data */
static void handle_get_tx_pwr(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	u8 *tx_pwr = &msg->body.tx_power.tx_pwr;
	int ret;
	struct wid wid;

	wid.id = WID_TX_POWER;
	wid.type = WID_CHAR;
	wid.val = (s8 *)tx_pwr;
	wid.size = sizeof(char);

	ret = wilc_send_config_pkt(vif, GET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if (ret)
		PRINT_ER(vif->ndev, "Failed to get TX PWR\n");

	complete(&msg->work_comp);
}

static void handle_scan_timer(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	int ret;

	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "handling scan timer\n");
	ret = handle_scan_done(msg->vif, SCAN_EVENT_ABORTED);
	if (ret)
		PRINT_ER(msg->vif->ndev, "Failed to handle scan done\n");
	kfree(msg);
}

static void handle_remain_on_chan_work(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	int ret;

	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "handling remain on channel\n");
	ret = handle_remain_on_chan(msg->vif, &msg->body.remain_on_ch);
	if (ret)
		PRINT_ER(msg->vif->ndev,
			 "Failed to handle remain on channel\n");
	kfree(msg);
}

static void handle_scan_complete(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);

	del_timer(&msg->vif->hif_drv->scan_timer);
	PRINT_INFO(msg->vif->ndev, HOSTINF_DBG, "scan completed\n");

	handle_scan_done(msg->vif, SCAN_EVENT_DONE);

	if (msg->vif->hif_drv->remain_on_ch_pending)
		handle_remain_on_chan(msg->vif, &msg->body.remain_on_ch);
	kfree(msg);
}

static void handle_set_antenna_mode(struct work_struct *work)
{
	struct host_if_msg *msg = container_of(work, struct host_if_msg, work);
	struct wilc_vif *vif = msg->vif;
	int ret;
	struct wid wid;
	struct sysfs_attr_group *attr_syfs_p = &vif->attr_sysfs;
	struct host_if_set_ant *set_ant = &msg->body.set_ant;

	wid.id = WID_ANTENNA_SELECTION;
	wid.type = WID_BIN;
	wid.val = (u8 *)set_ant;
	wid.size = sizeof(struct host_if_set_ant);

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_SNGL_GPIO_CTRL)
		PRINT_INFO(vif->ndev, CFG80211_DBG,
			   "set antenna %d on GPIO %d\n", set_ant->mode,
			   set_ant->antenna1);
	else if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_DUAL_GPIO_CTRL)
		PRINT_INFO(vif->ndev, CFG80211_DBG,
			   "set antenna %d on GPIOs %d and %d\n",
			   set_ant->mode, set_ant->antenna1,
			   set_ant->antenna2);

	ret = wilc_send_config_pkt(vif, SET_CFG, &wid, 1,
				   wilc_get_vif_idx(vif));
	if (ret)
		PRINT_ER(vif->ndev, "Failed to set antenna mode\n");
	kfree(msg);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void timer_scan_cb(struct timer_list *t)
#else
static void timer_scan_cb(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct host_if_drv *hif_drv = from_timer(hif_drv, t, scan_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->scan_timer_vif;
	struct host_if_msg *msg;
	int result;

	msg = wilc_alloc_work(vif, handle_scan_timer, false);
	if (IS_ERR(msg))
		return;

	result = wilc_enqueue_work(msg);
	if (result)
		kfree(msg);
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void timer_connect_cb(struct timer_list *t)
#else
static void timer_connect_cb(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct host_if_drv *hif_drv = from_timer(hif_drv, t, connect_timer);
#else
	struct host_if_drv *hif_drv = (struct host_if_drv *)arg;
#endif
	struct wilc_vif *vif = hif_drv->connect_timer_vif;
	struct host_if_msg *msg;
	int result;

	msg = wilc_alloc_work(vif, handle_connect_timeout, false);
	if (IS_ERR(msg))
		return;

	result = wilc_enqueue_work(msg);
	if (result)
		kfree(msg);
}

signed int wilc_send_buffered_eap(struct wilc_vif *vif,
				  wilc_frmw_to_linux_t frmw_to_linux,
				  free_eap_buf_param eap_buf_param,
				  u8 *buff, unsigned int size,
				  unsigned int pkt_offset,
				  void *user_arg)
{
	int result;
	struct host_if_msg *msg;

	if (!vif || !frmw_to_linux || !eap_buf_param)
		return -EFAULT;

	msg = wilc_alloc_work(vif, handle_send_buffered_eap, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);
	msg->body.send_buff_eap.frmw_to_linux = frmw_to_linux;
	msg->body.send_buff_eap.eap_buf_param = eap_buf_param;
	msg->body.send_buff_eap.size = size;
	msg->body.send_buff_eap.pkt_offset = pkt_offset;
	msg->body.send_buff_eap.buff = kmalloc(size + pkt_offset,
						  GFP_ATOMIC);
	memcpy(msg->body.send_buff_eap.buff, buff, size + pkt_offset);
	msg->body.send_buff_eap.user_arg = user_arg;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg->body.send_buff_eap.buff);
		kfree(msg);
	}
	return result;
}

int wilc_remove_wep_key(struct wilc_vif *vif, u8 index)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		result = -EFAULT;
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return result;
	}

	msg = wilc_alloc_work(vif, handle_key, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.key_info.type = WEP;
	msg->body.key_info.action = REMOVEKEY;
	msg->body.key_info.attr.wep.index = index;

	result = wilc_enqueue_work(msg);
	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	else
		wait_for_completion(&msg->work_comp);

	kfree(msg);
	return result;
}

int wilc_set_wep_default_keyid(struct wilc_vif *vif, u8 index)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		result = -EFAULT;
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return result;
	}

	msg = wilc_alloc_work(vif, handle_key, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.key_info.type = WEP;
	msg->body.key_info.action = DEFAULTKEY;
	msg->body.key_info.attr.wep.index = index;

	result = wilc_enqueue_work(msg);
	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	else
		wait_for_completion(&msg->work_comp);

	kfree(msg);
	return result;
}

int wilc_add_wep_key_bss_sta(struct wilc_vif *vif, const u8 *key, u8 len,
			     u8 index)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_key, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.key_info.type = WEP;
	msg->body.key_info.action = ADDKEY;
	msg->body.key_info.attr.wep.key = kmemdup(key, len, GFP_KERNEL);
	if (!msg->body.key_info.attr.wep.key) {
		result = -ENOMEM;
		goto free_msg;
	}

	msg->body.key_info.attr.wep.key_len = len;
	msg->body.key_info.attr.wep.index = index;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		goto free_key;
	}

	wait_for_completion(&msg->work_comp);

free_key:
	/*free key only here when work is completed or error in posting work*/
	kfree(msg->body.key_info.attr.wep.key);

free_msg:
	kfree(msg);
	return result;
}

int wilc_add_wep_key_bss_ap(struct wilc_vif *vif, const u8 *key, u8 len,
			    u8 index, u8 mode, enum authtype auth_type)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_key, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.key_info.type = WEP;
	msg->body.key_info.action = ADDKEY_AP;
	msg->body.key_info.attr.wep.key = kmemdup(key, len, GFP_KERNEL);
	if (!msg->body.key_info.attr.wep.key) {
		result = -ENOMEM;
		goto free_msg;
	}

	msg->body.key_info.attr.wep.key_len = len;
	msg->body.key_info.attr.wep.index = index;
	msg->body.key_info.attr.wep.mode = mode;
	msg->body.key_info.attr.wep.auth_type = auth_type;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		goto free_key;
	}

	wait_for_completion(&msg->work_comp);

free_key:
	kfree(msg->body.key_info.attr.wep.key);

free_msg:
	kfree(msg);
	return result;
}

int wilc_add_ptk(struct wilc_vif *vif, const u8 *ptk, u8 ptk_key_len,
		 const u8 *mac_addr, const u8 *rx_mic, const u8 *tx_mic,
		 u8 mode, u8 cipher_mode, u8 index)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 key_len = ptk_key_len;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	if (rx_mic)
		key_len += RX_MIC_KEY_LEN;

	if (tx_mic)
		key_len += TX_MIC_KEY_LEN;

	msg = wilc_alloc_work(vif, handle_key, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.key_info.type = WPA_PTK;
	if (mode == AP_MODE) {
		msg->body.key_info.action = ADDKEY_AP;
		msg->body.key_info.attr.wpa.index = index;
	}
	if (mode == STATION_MODE)
		msg->body.key_info.action = ADDKEY;

	msg->body.key_info.attr.wpa.key = kmemdup(ptk, ptk_key_len, GFP_KERNEL);
	if (!msg->body.key_info.attr.wpa.key) {
		result = -ENOMEM;
		goto free_msg;
	}

	if (rx_mic)
		memcpy(msg->body.key_info.attr.wpa.key + 16, rx_mic,
		       RX_MIC_KEY_LEN);

	if (tx_mic)
		memcpy(msg->body.key_info.attr.wpa.key + 24, tx_mic,
		       TX_MIC_KEY_LEN);

	msg->body.key_info.attr.wpa.key_len = key_len;
	msg->body.key_info.attr.wpa.mac_addr = mac_addr;
	msg->body.key_info.attr.wpa.mode = cipher_mode;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		goto free_key;
	}

	wait_for_completion(&msg->work_comp);

free_key:
	kfree(msg->body.key_info.attr.wpa.key);

free_msg:
	kfree(msg);
	return result;
}

int wilc_add_rx_gtk(struct wilc_vif *vif, const u8 *rx_gtk, u8 gtk_key_len,
		    u8 index, u32 key_rsc_len, const u8 *key_rsc,
		    const u8 *rx_mic, const u8 *tx_mic, u8 mode,
		    u8 cipher_mode)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;
	u8 key_len = gtk_key_len;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_key, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	if (rx_mic)
		key_len += RX_MIC_KEY_LEN;

	if (tx_mic)
		key_len += TX_MIC_KEY_LEN;

	if (key_rsc) {
		msg->body.key_info.attr.wpa.seq = kmemdup(key_rsc,
							  key_rsc_len,
							  GFP_KERNEL);
		if (!msg->body.key_info.attr.wpa.seq) {
			result = -ENOMEM;
			goto free_msg;
		}
	}

	msg->body.key_info.type = WPA_RX_GTK;

	if (mode == AP_MODE) {
		msg->body.key_info.action = ADDKEY_AP;
		msg->body.key_info.attr.wpa.mode = cipher_mode;
	}
	if (mode == STATION_MODE)
		msg->body.key_info.action = ADDKEY;

	msg->body.key_info.attr.wpa.key = kmemdup(rx_gtk, key_len, GFP_KERNEL);
	if (!msg->body.key_info.attr.wpa.key) {
		result = -ENOMEM;
		goto free_seq;
	}

	if (rx_mic)
		memcpy(msg->body.key_info.attr.wpa.key + 16, rx_mic,
		       RX_MIC_KEY_LEN);

	if (tx_mic)
		memcpy(msg->body.key_info.attr.wpa.key + 24, tx_mic,
		       TX_MIC_KEY_LEN);

	msg->body.key_info.attr.wpa.index = index;
	msg->body.key_info.attr.wpa.key_len = key_len;
	msg->body.key_info.attr.wpa.seq_len = key_rsc_len;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		goto free_key;
	}

	wait_for_completion(&msg->work_comp);

free_key:
	kfree(msg->body.key_info.attr.wpa.key);

free_seq:
	kfree(msg->body.key_info.attr.wpa.seq);

free_msg:
	kfree(msg);
	return result;
}

int wilc_set_pmkid_info(struct wilc_vif *vif,
			struct host_if_pmkid_attr *pmkid)
{
	int result;
	struct host_if_msg *msg;
	int i;

	msg = wilc_alloc_work(vif, handle_key, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.key_info.type = PMKSA;
	msg->body.key_info.action = ADDKEY;

	for (i = 0; i < pmkid->numpmkid; i++) {
		memcpy(msg->body.key_info.attr.pmkid.pmkidlist[i].bssid,
		       &pmkid->pmkidlist[i].bssid, ETH_ALEN);
		memcpy(msg->body.key_info.attr.pmkid.pmkidlist[i].pmkid,
		       &pmkid->pmkidlist[i].pmkid, PMKID_LEN);
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

int wilc_get_mac_address(struct wilc_vif *vif, u8 *mac_addr)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_get_mac_address, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.dev_mac_info.mac_addr = mac_addr;

	result = wilc_enqueue_work(msg);
	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	else
		wait_for_completion(&msg->work_comp);

	kfree(msg);

	return result;
}

int wilc_set_mac_address(struct wilc_vif *vif, u8 *mac_addr)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_set_mac_address, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.dev_mac_info.mac_addr = mac_addr;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		result = -EFAULT;
	} else {
		wait_for_completion(&msg->work_comp);
	}
	kfree(msg);
	return result;
}

int wilc_set_join_req(struct wilc_vif *vif, u8 *bssid, const u8 *ssid,
		      size_t ssid_len, const u8 *ies, size_t ies_len,
		      wilc_connect_result connect_result, void *user_arg,
		      u8 security, enum authtype auth_type,
		      u8 channel, void *join_params)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv || !connect_result) {
		PRINT_ER(vif->ndev, "hif driver or connect result is NULL\n");
		return -EFAULT;
	}

	if (!join_params) {
		PRINT_ER(vif->ndev, "joinparams is NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_connect, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.con_info.security = security;
	msg->body.con_info.auth_type = auth_type;
	msg->body.con_info.ch = channel;
	msg->body.con_info.result = connect_result;
	msg->body.con_info.arg = user_arg;
	msg->body.con_info.params = join_params;

	if (bssid) {
		msg->body.con_info.bssid = kmemdup(bssid, 6, GFP_KERNEL);
		if (!msg->body.con_info.bssid) {
			result = -ENOMEM;
			goto free_msg;
		}
	}

	if (ssid) {
		msg->body.con_info.ssid_len = ssid_len;
		msg->body.con_info.ssid = kmemdup(ssid, ssid_len, GFP_KERNEL);
		if (!msg->body.con_info.ssid) {
			result = -ENOMEM;
			goto free_bssid;
		}
	}

	if (ies) {
		msg->body.con_info.ies_len = ies_len;
		msg->body.con_info.ies = kmemdup(ies, ies_len, GFP_KERNEL);
		if (!msg->body.con_info.ies) {
			result = -ENOMEM;
			goto free_ssid;
		}
	}
	if (hif_drv->hif_state < HOST_IF_CONNECTING)
		hif_drv->hif_state = HOST_IF_CONNECTING;
	else
		PRINT_INFO(vif->ndev, GENERIC_DBG,
			   "Don't set state to 'connecting' as state is %d\n",
			   hif_drv->hif_state);

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		goto free_ies;
	}
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
	hif_drv->connect_timer.data = (unsigned long)hif_drv;
#endif
	hif_drv->connect_timer_vif = vif;
	mod_timer(&hif_drv->connect_timer,
		  jiffies + msecs_to_jiffies(HOST_IF_CONNECT_TIMEOUT));

	return 0;

free_ies:
	kfree(msg->body.con_info.ies);

free_ssid:
	kfree(msg->body.con_info.ssid);

free_bssid:
	kfree(msg->body.con_info.bssid);

free_msg:
	kfree(msg);
	return result;
}

int wilc_disconnect(struct wilc_vif *vif, u16 reason_code)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_disconnect, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	result = wilc_enqueue_work(msg);
	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	else
		wait_for_completion(&msg->work_comp);

	kfree(msg);
	return result;
}

int wilc_set_mac_chnl_num(struct wilc_vif *vif, u8 channel)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_set_channel, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.channel_info.set_ch = channel;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

int wilc_set_wfi_drv_handler(struct wilc_vif *vif, int index, u8 mode,
			     u8 ifc_id, bool is_sync)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_set_wfi_drv_handler, is_sync);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.drv.handler = index;
	msg->body.drv.mode = mode;
	msg->body.drv.ifc_id = ifc_id;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
		return result;
	}

	if (is_sync)
		wait_for_completion(&msg->work_comp);

	return result;
}

int wilc_set_operation_mode(struct wilc_vif *vif, u32 mode)
{
	int result;
	struct host_if_msg *msg;

	msg  = wilc_alloc_work(vif, handle_set_operation_mode, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.mode.mode = mode;
	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

s32 wilc_get_inactive_time(struct wilc_vif *vif, const u8 *mac,
			   u32 *out_val)
{
	s32 result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_get_inactive_time, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	memcpy(msg->body.mac_info.mac, mac, ETH_ALEN);

	result = wilc_enqueue_work(msg);
	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	else
		wait_for_completion(&msg->work_comp);

	*out_val = msg->body.mac_info.inactive_time;
	kfree(msg);

	return result;
}

int wilc_get_rssi(struct wilc_vif *vif, s8 *rssi_level)
{
	int result;
	struct host_if_msg *msg;

	if (!rssi_level) {
		PRINT_ER(vif->ndev, "RSS pointer value is null\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_get_rssi, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.data = kzalloc(sizeof(s8), GFP_KERNEL);
	if (!msg->body.data) {
		kfree(msg);
		return -ENOMEM;
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	} else {
		wait_for_completion(&msg->work_comp);

		if (*msg->body.data == INVALID_RSSI)
			result = -EFAULT;
		else
			*rssi_level = *msg->body.data;
	}

	kfree(msg->body.data);
	kfree(msg);

	return result;
}

int
wilc_get_statistics(struct wilc_vif *vif, struct rf_info *stats, bool is_sync)
{
	int result;
	struct host_if_msg *msg;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, " getting statistics\n");
	msg = wilc_alloc_work(vif, handle_get_statistics, is_sync);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.data = (char *)stats;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
		return result;
	}

	if (is_sync) {
		wait_for_completion(&msg->work_comp);
		kfree(msg);
	}

	return result;
}

int wilc_scan(struct wilc_vif *vif, u8 scan_source, u8 scan_type,
	      u8 *ch_freq_list, u8 ch_list_len, const u8 *ies,
	      size_t ies_len, wilc_scan_result scan_result, void *user_arg,
	      struct hidden_network *hidden_network)
{
	int result;
	struct host_if_msg *msg;
	struct scan_attr *scan_info;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv || !scan_result) {
		PRINT_ER(vif->ndev, "hif_drv or scan_result = NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_scan, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	scan_info = &msg->body.scan_info;

	if (hidden_network) {
		scan_info->hidden_network.net_info = hidden_network->net_info;
		scan_info->hidden_network.n_ssids = hidden_network->n_ssids;
	} else {
		PRINT_WRN(vif->ndev, HOSTINF_DBG,
			  "hidden_network IS EQUAL TO NULL\n");
	}

	scan_info->src = scan_source;
	scan_info->type = scan_type;
	scan_info->result = scan_result;
	scan_info->arg = user_arg;

	scan_info->ch_list_len = ch_list_len;
	scan_info->ch_freq_list = kmemdup(ch_freq_list,
					  ch_list_len,
					  GFP_KERNEL);
	if (!scan_info->ch_freq_list) {
		result = -ENOMEM;
		goto free_msg;
	}

	scan_info->ies_len = ies_len;
	scan_info->ies = kmemdup(ies, ies_len, GFP_KERNEL);
	if (!scan_info->ies) {
		result = -ENOMEM;
		goto free_freq_list;
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		goto free_ies;
	}

	PRINT_INFO(vif->ndev, HOSTINF_DBG, ">> Starting the SCAN timer\n");
#if KERNEL_VERSION(4, 15, 0) > LINUX_VERSION_CODE
	hif_drv->scan_timer.data = (unsigned long)hif_drv;
#endif
	hif_drv->scan_timer_vif = vif;
	mod_timer(&hif_drv->scan_timer,
		  jiffies + msecs_to_jiffies(HOST_IF_SCAN_TIMEOUT));

	return 0;

free_ies:
	kfree(scan_info->ies);

free_freq_list:
	kfree(scan_info->ch_freq_list);

free_msg:
	kfree(msg);
	return result;
}

int wilc_hif_set_cfg(struct wilc_vif *vif,
		     struct cfg_param_attr *cfg_param)
{
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;
	int result;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif_drv NULL\n");
		return -EFAULT;
	}

	msg = wilc_alloc_work(vif, handle_cfg_param, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.cfg_info = *cfg_param;
	result = wilc_enqueue_work(msg);
	if (result)
		kfree(msg);

	return result;
}

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
static void get_periodic_rssi(struct timer_list *t)
#else
static void get_periodic_rssi(unsigned long arg)
#endif
{
#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	struct wilc_vif *vif = from_timer(vif, t, periodic_rssi);
#else
	struct wilc_vif *vif = (struct wilc_vif *)arg;
#endif

	if (!vif->hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return;
	}

	if (vif->hif_drv->hif_state == HOST_IF_CONNECTED)
		wilc_get_statistics(vif, &vif->periodic_stats, false);

	mod_timer(&vif->periodic_rssi, jiffies + msecs_to_jiffies(5000));
}

int wilc_init(struct net_device *dev, struct host_if_drv **hif_drv_handler)
{
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif = netdev_priv(dev);
	struct wilc *wilc = vif->wilc;
	int i;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Initializing host interface for client %d\n",
		   wilc->clients_count + 1);

	hif_drv  = kzalloc(sizeof(*hif_drv), GFP_KERNEL);
	if (!hif_drv) {
		PRINT_ER(dev, "hif driver is NULL\n");
		return -ENOMEM;
	}
	*hif_drv_handler = hif_drv;
	for (i = 0; i <= wilc->vif_num; i++)
		if (dev == wilc->vif[i]->ndev) {
			wilc->vif[i]->hif_drv = hif_drv;
			hif_drv->driver_handler_id = i + 1;
			break;
		}

#ifdef DISABLE_PWRSAVE_AND_SCAN_DURING_IP
	vif->obtaining_ip = false;
#endif

	if (wilc->clients_count == 0)
		mutex_init(&hif_deinit_lock);

	#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
		timer_setup(&vif->periodic_rssi, get_periodic_rssi, 0);
	#else
		setup_timer(&vif->periodic_rssi, get_periodic_rssi,
			    (unsigned long)vif);
	#endif
		mod_timer(&vif->periodic_rssi,
			  jiffies + msecs_to_jiffies(5000));

#if KERNEL_VERSION(4, 15, 0) <= LINUX_VERSION_CODE
	timer_setup(&hif_drv->scan_timer, timer_scan_cb, 0);
	timer_setup(&hif_drv->connect_timer, timer_connect_cb, 0);
	timer_setup(&hif_drv->remain_on_ch_timer, listen_timer_cb, 0);
#else
	setup_timer(&hif_drv->scan_timer, timer_scan_cb, 0);
	setup_timer(&hif_drv->connect_timer, timer_connect_cb, 0);
	setup_timer(&hif_drv->remain_on_ch_timer, listen_timer_cb, 0);
#endif

	hif_drv->hif_state = HOST_IF_IDLE;

	hif_drv->p2p_timeout = 0;

	wilc->clients_count++;

	return 0;
}

int wilc_deinit(struct wilc_vif *vif)
{
	int result = 0;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	mutex_lock(&hif_deinit_lock);

	terminated_handle = hif_drv;
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "De-initializing host interface for client %d\n",
		   vif->wilc->clients_count);

	del_timer_sync(&hif_drv->scan_timer);
	del_timer_sync(&hif_drv->connect_timer);
	del_timer_sync(&vif->periodic_rssi);
	del_timer_sync(&hif_drv->remain_on_ch_timer);

	wilc_set_wfi_drv_handler(vif, 0, 0, 0, true);

	if (hif_drv->usr_scan_req.scan_result) {
		hif_drv->usr_scan_req.scan_result(SCAN_EVENT_ABORTED, NULL,
						  hif_drv->usr_scan_req.arg,
						  NULL);
		hif_drv->usr_scan_req.scan_result = NULL;
	}

	hif_drv->hif_state = HOST_IF_IDLE;

	kfree(hif_drv);

	vif->wilc->clients_count--;
	terminated_handle = NULL;
	mutex_unlock(&hif_deinit_lock);
	return result;
}

void wilc_network_info_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	int result;
	struct host_if_msg *msg;
	int id;
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;

	id = buffer[length - 4];
	id |= (buffer[length - 3] << 8);
	id |= (buffer[length - 2] << 16);
	id |= (buffer[length - 1] << 24);
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif)
		return;
	hif_drv = vif->hif_drv;

	if (!hif_drv || hif_drv == terminated_handle) {
		PRINT_ER(vif->ndev, "driver not init[%p]\n", hif_drv);
		return;
	}

	msg = wilc_alloc_work(vif, handle_rcvd_ntwrk_info, false);
	if (IS_ERR(msg))
		return;

	msg->body.net_info.len = length;
	msg->body.net_info.buffer = kmemdup(buffer, length, GFP_KERNEL);
	if (!msg->body.net_info.buffer) {
		kfree(msg);
		return;
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "message parameters (%d)\n", result);
		kfree(msg->body.net_info.buffer);
		kfree(msg);
	}
}

void wilc_gnrl_async_info_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	int result;
	struct host_if_msg *msg;
	int id;
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;

	mutex_lock(&hif_deinit_lock);

	id = buffer[length - 4];
	id |= (buffer[length - 3] << 8);
	id |= (buffer[length - 2] << 16);
	id |= (buffer[length - 1] << 24);
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif) {
		mutex_unlock(&hif_deinit_lock);
		return;
	}
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "General asynchronous info packet received\n");

	hif_drv = vif->hif_drv;

	if (!hif_drv || hif_drv == terminated_handle) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	if (!hif_drv->usr_conn_req.conn_result) {
		PRINT_ER(vif->ndev, "there is no current Connect Request\n");
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	msg = wilc_alloc_work(vif, handle_rcvd_gnrl_async_info, false);
	if (IS_ERR(msg)) {
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	msg->body.async_info.len = length;
	msg->body.async_info.buffer = kmemdup(buffer, length, GFP_KERNEL);
	if (!msg->body.async_info.buffer) {
		kfree(msg);
		mutex_unlock(&hif_deinit_lock);
		return;
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg->body.async_info.buffer);
		kfree(msg);
	}

	mutex_unlock(&hif_deinit_lock);
}

void wilc_scan_complete_received(struct wilc *wilc, u8 *buffer, u32 length)
{
	int result;
	int id;
	struct host_if_drv *hif_drv;
	struct wilc_vif *vif;

	id = buffer[length - 4];
	id |= buffer[length - 3] << 8;
	id |= buffer[length - 2] << 16;
	id |= buffer[length - 1] << 24;
	vif = wilc_get_vif_from_idx(wilc, id);
	if (!vif)
		return;
	hif_drv = vif->hif_drv;
	PRINT_INFO(vif->ndev, GENERIC_DBG, "Scan notification received\n");

	if (!hif_drv || hif_drv == terminated_handle) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return;
	}

	if (hif_drv->usr_scan_req.scan_result) {
		struct host_if_msg *msg;

		msg = wilc_alloc_work(vif, handle_scan_complete, false);
		if (IS_ERR(msg))
			return;

		result = wilc_enqueue_work(msg);
		if (result) {
			PRINT_ER(vif->ndev, "enqueue work failed\n");
			kfree(msg);
		}
	}
}

int wilc_remain_on_channel(struct wilc_vif *vif, u32 session_id,
			   u32 duration, u16 chan,
			   wilc_remain_on_chan_expired expired,
			   wilc_remain_on_chan_ready ready,
			   void *user_arg)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_remain_on_chan_work, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.remain_on_ch.ch = chan;
	msg->body.remain_on_ch.expired = expired;
	msg->body.remain_on_ch.ready = ready;
	msg->body.remain_on_ch.arg = user_arg;
	msg->body.remain_on_ch.duration = duration;
	msg->body.remain_on_ch.id = session_id;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

int wilc_listen_state_expired(struct wilc_vif *vif, u32 session_id)
{
	int result;
	struct host_if_msg *msg;
	struct host_if_drv *hif_drv = vif->hif_drv;

	if (!hif_drv) {
		PRINT_ER(vif->ndev, "hif driver is NULL\n");
		return -EFAULT;
	}

	del_timer(&hif_drv->remain_on_ch_timer);

	msg = wilc_alloc_work(vif, handle_listen_state_expired, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.remain_on_ch.id = session_id;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

void wilc_frame_register(struct wilc_vif *vif, u16 frame_type, bool reg)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_register_frame, false);
	if (IS_ERR(msg))
		return;

	switch (frame_type) {
	case ACTION:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "ACTION\n");
		msg->body.reg_frame.reg_id = ACTION_FRM_IDX;
		break;

	case PROBE_REQ:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "PROBE REQ\n");
		msg->body.reg_frame.reg_id = PROBE_REQ_IDX;
		break;

	default:
		PRINT_INFO(vif->ndev, HOSTINF_DBG, "Not valid frame type\n");
		break;
	}
	msg->body.reg_frame.frame_type = frame_type;
	msg->body.reg_frame.reg = reg;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
}

int wilc_add_beacon(struct wilc_vif *vif, u32 interval, u32 dtim_period,
		    u32 head_len, u8 *head, u32 tail_len, u8 *tail)
{
	int result;
	struct host_if_msg *msg;
	struct beacon_attr *beacon_info;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting adding beacon message queue params\n");
	msg = wilc_alloc_work(vif, handle_add_beacon, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	beacon_info = &msg->body.beacon_info;
	beacon_info->interval = interval;
	beacon_info->dtim_period = dtim_period;
	beacon_info->head_len = head_len;
	beacon_info->head = kmemdup(head, head_len, GFP_KERNEL);
	if (!beacon_info->head) {
		result = -ENOMEM;
		goto error;
	}
	beacon_info->tail_len = tail_len;

	if (tail_len > 0) {
		beacon_info->tail = kmemdup(tail, tail_len, GFP_KERNEL);
		if (!beacon_info->tail) {
			result = -ENOMEM;
			goto error;
		}
	} else {
		beacon_info->tail = NULL;
	}

	result = wilc_enqueue_work(msg);
	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");

error:
	if (result) {
		kfree(beacon_info->head);

		kfree(beacon_info->tail);
		kfree(msg);
	}

	return result;
}

int wilc_del_beacon(struct wilc_vif *vif)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_del_beacon, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deleting beacon message queue params\n");

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return result;
}

int wilc_add_station(struct wilc_vif *vif, struct add_sta_param *sta_param)
{
	int result;
	struct host_if_msg *msg;
	struct add_sta_param *add_sta_info;

	msg = wilc_alloc_work(vif, handle_add_station, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	add_sta_info = &msg->body.add_sta_info;
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting adding station message queue params\n");

	memcpy(add_sta_info, sta_param, sizeof(struct add_sta_param));
	if (add_sta_info->rates_len > 0) {
		add_sta_info->rates = kmemdup(sta_param->rates,
					      add_sta_info->rates_len,
					      GFP_KERNEL);
		if (!add_sta_info->rates) {
			kfree(msg);
			return -ENOMEM;
		}
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(add_sta_info->rates);
		kfree(msg);
	}
	return result;
}

int wilc_del_station(struct wilc_vif *vif, const u8 *mac_addr)
{
	int result;
	struct host_if_msg *msg;
	struct del_sta *del_sta_info;

	msg = wilc_alloc_work(vif, handle_del_station, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	del_sta_info = &msg->body.del_sta_info;
	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deleting station message queue params\n");

	if (!mac_addr)
		eth_broadcast_addr(del_sta_info->mac_addr);
	else
		memcpy(del_sta_info->mac_addr, mac_addr, ETH_ALEN);

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
	return result;
}

int wilc_del_allstation(struct wilc_vif *vif, u8 mac_addr[][ETH_ALEN])
{
	int result;
	struct host_if_msg *msg;
	struct del_all_sta *del_all_sta_info;
	u8 zero_addr[ETH_ALEN] = {0};
	int i;
	u8 assoc_sta = 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting deauthenticating station message queue params\n");
	msg = wilc_alloc_work(vif, handle_del_all_sta, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	del_all_sta_info = &msg->body.del_all_sta_info;

	for (i = 0; i < MAX_NUM_STA; i++) {
		if (memcmp(mac_addr[i], zero_addr, ETH_ALEN)) {
			memcpy(del_all_sta_info->del_all_sta[i], mac_addr[i],
			       ETH_ALEN);
			PRINT_INFO(vif->ndev,
				   CFG80211_DBG, "BSSID = %x%x%x%x%x%x\n",
				   del_all_sta_info->del_all_sta[i][0],
				   del_all_sta_info->del_all_sta[i][1],
				   del_all_sta_info->del_all_sta[i][2],
				   del_all_sta_info->del_all_sta[i][3],
				   del_all_sta_info->del_all_sta[i][4],
				   del_all_sta_info->del_all_sta[i][5]);
			assoc_sta++;
		}
	}
	if (!assoc_sta) {
		PRINT_INFO(vif->ndev, CFG80211_DBG, "NO ASSOCIATED STAS\n");
		kfree(msg);
		return 0;
	}

	del_all_sta_info->assoc_sta = assoc_sta;
	result = wilc_enqueue_work(msg);

	if (result)
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	else
		wait_for_completion(&msg->work_comp);

	kfree(msg);

	return result;
}

int wilc_edit_station(struct wilc_vif *vif,
		      struct add_sta_param *sta_param)
{
	int result;
	struct host_if_msg *msg;
	struct add_sta_param *add_sta_info;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting editing station message queue params\n");
	msg = wilc_alloc_work(vif, handle_edit_station, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	add_sta_info = &msg->body.add_sta_info;
	memcpy(add_sta_info, sta_param, sizeof(*add_sta_info));
	if (add_sta_info->rates_len > 0) {
		add_sta_info->rates = kmemdup(sta_param->rates,
					      add_sta_info->rates_len,
					      GFP_KERNEL);
		if (!add_sta_info->rates) {
			kfree(msg);
			return -ENOMEM;
		}
	}

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(add_sta_info->rates);
		kfree(msg);
	}

	return result;
}

int wilc_set_power_mgmt(struct wilc_vif *vif, bool enabled, u32 timeout)
{
	int result;
	struct host_if_msg *msg;

	if (wilc_wlan_get_num_conn_ifcs(vif->wilc) == 2 && enabled)
		return 0;

	PRINT_INFO(vif->ndev, HOSTINF_DBG, "\n\n>> Setting PS to %d <<\n\n",
		   enabled);
	msg = wilc_alloc_work(vif, handle_power_management, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.pwr_mgmt_info.enabled = enabled;
	msg->body.pwr_mgmt_info.timeout = timeout;

	if (!vif->wilc->hif_workqueue)
		return 0;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
	return result;
}

int wilc_setup_multicast_filter(struct wilc_vif *vif, bool enabled,
				u32 count, u8 *mc_list)
{
	int result;
	struct host_if_msg *msg;

	PRINT_INFO(vif->ndev, HOSTINF_DBG,
		   "Setting Multicast Filter params\n");
	msg = wilc_alloc_work(vif, handle_set_mcast_filter, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.multicast_info.enabled = enabled;
	msg->body.multicast_info.cnt = count;
	msg->body.multicast_info.mc_list = mc_list;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
	return result;
}

int wilc_set_tx_power(struct wilc_vif *vif, u8 tx_power)
{
	int ret;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_set_tx_pwr, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.tx_power.tx_pwr = tx_power;

	ret = wilc_enqueue_work(msg);
	if (ret) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}

	return ret;
}

int wilc_get_tx_power(struct wilc_vif *vif, u8 *tx_power)
{
	int ret;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_get_tx_pwr, true);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	ret = wilc_enqueue_work(msg);
	if (ret) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
	} else {
		wait_for_completion(&msg->work_comp);
		*tx_power = msg->body.tx_power.tx_pwr;
	}

	/* free 'msg' after copying data */
	kfree(msg);
	return ret;
}

bool is_valid_gpio(struct wilc_vif *vif, u8 gpio)
{
	switch (vif->wilc->chip) {
	case WILC_1000:
		if (gpio == 0 || gpio == 1 || gpio == 4 || gpio == 6)
			return true;
		else
			return false;
	case WILC_3000:
		if (gpio == 0 || gpio == 3 || gpio == 4 ||
			(gpio >= 17 && gpio <= 20))
			return true;
		else
			return false;
	default:
		return false;
	}
}

int wilc_set_antenna(struct wilc_vif *vif, u8 mode)
{
	int ret;
	struct host_if_msg *msg;
	struct sysfs_attr_group *attr_syfs_p;

	msg = wilc_alloc_work(vif, handle_set_antenna_mode, false);
	if (IS_ERR(msg))
		return PTR_ERR(msg);

	msg->body.set_ant.mode = mode;
	attr_syfs_p = &vif->attr_sysfs;

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_INVALID_GPIO_CTRL) {
		PRINT_ER(vif->ndev, "Ant switch GPIO mode is invalid.\n");
		PRINT_ER(vif->ndev, "Set it using /sys/wilc/ant_swtch_mode\n");
		return WILC_FAIL;
	}

	if (is_valid_gpio(vif, attr_syfs_p->antenna1)) {
		msg->body.set_ant.antenna1 = attr_syfs_p->antenna1;
	} else {
		PRINT_ER(vif->ndev, "Invalid GPIO%d\n", attr_syfs_p->antenna1);
		return WILC_FAIL;
	}

	if (attr_syfs_p->ant_swtch_mode == ANT_SWTCH_DUAL_GPIO_CTRL) {
		if ((attr_syfs_p->antenna2 != attr_syfs_p->antenna1) &&
			is_valid_gpio(vif, attr_syfs_p->antenna2)) {
			msg->body.set_ant.antenna2 = attr_syfs_p->antenna2;
		} else {
			PRINT_ER(vif->ndev, "Invalid GPIO %d\n",
				 attr_syfs_p->antenna2);
			return WILC_FAIL;
		}
	}

	msg->body.set_ant.gpio_mode = attr_syfs_p->ant_swtch_mode;
	ret = wilc_enqueue_work(msg);
	if (ret) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
		return -EINVAL;
	}
	return ret;
}

int host_int_set_wowlan_trigger(struct wilc_vif *vif, u8 wowlan_trigger)
{
	int result;
	struct host_if_msg *msg;

	msg = wilc_alloc_work(vif, handle_set_wowlan_trigger, false);
	msg->body.wow_trigger.wowlan_trigger = wowlan_trigger;

	result = wilc_enqueue_work(msg);
	if (result) {
		PRINT_ER(vif->ndev, "enqueue work failed\n");
		kfree(msg);
	}
	return result;
}