
/* #include "system/app_core.h" */
#include "includes.h"
/* #include "server/server_core.h" */
#include "hid_app_config.h"
#include "app_action.h"
/* #include "os/os_api.h" */
#include "btcontroller_config.h"
#include "bt_controller_include/btctrler_task.h"
/* #include "config/config_transport.h" */
#include "bt_include/avctp_user.h"
#include "bt_include/btstack_task.h"
/* #include "bt_common.h" */
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_app_bss")
#pragma data_seg(".ble_app_data")
#pragma const_seg(".ble_app_text_const")
#pragma code_seg(".ble_app_text")
#endif
#include "user_cfg.h"
#if (TRANS_DATA_HID_EN)
#if (CONFIG_APP_KEYBOARD)
#include "edr_hid_user.h"
/* #include "code_switch.h" */
/* #include "omsensor/OMSensor_manage.h" */
/* #include "le_common.h" */
#include <stdlib.h>
#include "standard_hid.h"
/* #include "app_charge.h" */
/* #include "app_power_manage.h" */
/* #include "app_chargestore.h" */
#include "app_comm_bt.h"
#include "vble_complete.h"
#include "sys_timer.h"
#include "msg.h"
#include "rc_key.h"
#include "rc_led.h"
#include "app_reg_include.h"
#include "encoder_mge.h"

#if RCSP_BTMATE_EN
#include "rcsp_bluetooth.h"
#endif

#define LOG_TAG_CONST       HID_KEY
#define LOG_TAG             "[HID_KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
/* #include "debug.h" */
#include "log.h"

//2.4G模式： 0---ble ,非0 2.4G配对码
#define CFG_RF_24G_CODE_ID  (0)

static const u8 remoter_name1[] = "Remote G10";

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_BLE = BIT(1),
    HID_MODE_BLE_24G = BIT(2),
    HID_MODE_INIT = 0xff
} bt_mode_e;
bt_mode_e bt_hid_mode = HID_MODE_NULL;

u8 ble_bonding_info=0;
u8 cur_bt_idx;

void ble_module_enable(u8);
void modify_ble_pair_name(void);

//----------------------------------
static const u8 hidkey_report_map[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, KEYBOARD_REPORT_ID,//   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, COUSTOM_CONTROL_REPORT_ID,//   Report ID (3)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x8C, 0x02,  //   Logical Maximum (652)
    0x19, 0x00,        //   Usage Minimum (Unassigned)
    0x2A, 0x8C, 0x02,  //   Usage Maximum (AC Send)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    //
//     0x05, 0x01,        // Usage Page (HID_USAGE_PAGE_GENERIC)
//     0x09, 0x80,        // Usage (HID_USAGE_GENERIC_SYSTEM_CTL)
//     0xA1, 0x01,        // Collection (Application)
//     0x85, SYS_CONTROL_REPORT_ID,//   Report ID (4)
// //    0x05, 0x01,        // Usage Page (HID_USAGE_PAGE_GENERIC)
//     0x75, 0x01,        //   Report Size (8)
//     0x95, 0x03,        //   Report Count (3)
//     0x15, 0x00,        //   Logical Minimum (0)
//     0x25, 0x01,  //   Logical Maximum (1)
//     0x19, 0x81,        //   Usage Minimum (Unassigned)
//     0x29, 0x83,  //   Usage Maximum (AC Send)
//     0x81, 0x02,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
// //    0x75, 0x05,        //   Report Size (5)
//     0x95, 0x05,        //   Report Count (5)
//     0x81, 0x01,        //   input(constant)
//     0xC0,              // End Collection

    // Dummy mouse collection starts here
    //
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)     0
    0x09, 0x02,                         // USAGE (Mouse)                    2
    0xa1, 0x01,                         // COLLECTION (Application)         4
    0x85, MOUSE_POINT_REPORT_ID,               //   REPORT_ID (Mouse)              6
    0x09, 0x01,                         //   USAGE (Pointer)                8
    0xa1, 0x00,                         //   COLLECTION (Physical)          10
    0x05, 0x09,                         //     USAGE_PAGE (Button)          12
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)     14
    0x29, 0x02,                         //     USAGE_MAXIMUM (Button 2)     16
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          18
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          20
    0x75, 0x01,                         //     REPORT_SIZE (1)              22
    0x95, 0x02,                         //     REPORT_COUNT (2)             24
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         26
    0x95, 0x06,                         //     REPORT_COUNT (6)             28
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         30
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop) 32
    0x09, 0x30,                         //     USAGE (X)                    34
    0x09, 0x31,                         //     USAGE (Y)                    36
	0x09, 0x38, 	   					//	  Usage (roll)
    0x15, 0x81,                         //     LOGICAL_MINIMUM (-127)       38
    0x25, 0x7f,                         //     LOGICAL_MAXIMUM (127)        40
    0x75, 0x08,                         //     REPORT_SIZE (8)              42
    0x95, 0x03,                         //     REPORT_COUNT (2)             44
    0x81, 0x06,                         //     INPUT (Data,Var,Rel)         46
    0xc0,                               //   END_COLLECTION                 48
    0xc0,                                // END_COLLECTION                   49/50
    // vendor define for audio transit
    //
    0x06, 0x00 ,0xFF, 		//Usage Page (Vendor-defined 0xFF00) 
    0x09, 0x00,				//Usage (Vendor-defined 0x0000)   
    0xa1, 0x01,				//Collection (Application) 
    0xa1, 0x02,				// Collection (Logical)  
    0x85, 0x5a,	//SLAVE_RESPONSE_REPORT_ID
    0x09, 0x00,				//Usage (Vendor-defined 0x0000)
    0x15, 0x80,				//Logical minimum (-128)
    0x25, 0x7f,				//Logical maximum (127)  
    0x75, 0x08,				//Report Size (8)   
    0x95, 0xff,				//Report Count (255) 
    0x81, 0x22,				//Input (Data,Value,Absolute,Bit Field) 
    0xc0,                               //   END_COLLECTION                 48
    0xc0                                // END_COLLECTION   
};

//----------------------------------
static const ble_init_cfg_t hidkey_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_GENERIC_REMOTE_CONTROL,
    .report_map = hidkey_report_map,
    .report_map_size = sizeof(hidkey_report_map),
};

static void kb_24g_mode_set(u32 code_id)
{
#if TCFG_USER_BLE_ENABLE
    log_info("%s:%02x", __FUNCTION__, code_id);
    rf_set_24g_hackable_coded(code_id);
#endif
}

static u16 int_advOpen_timer = 0;
extern void ble_key_adv_open(void);
extern void hogp_wakeUpAdv_config_set(void);
static void ble_closeAdv_handle(void)
{
	ble_module_enable(0);
	if(int_advOpen_timer)
	{
		sys_timeout_del(int_advOpen_timer);
		int_advOpen_timer = 0;
	}
	ble_key_adv_open();
	ble_bonding_info = 0;
	syscfg_read(CFG_USER_BLE_BONDING,&ble_bonding_info,1);

	if(ble_bonding_info^0xa3)
		ble_module_enable(0);
	else ble_module_enable(1); 
}
void ble_advOpen_handle(void)
{
	ble_module_enable(0);
    bt_led_mode = BT_PAIR_LED_DISCON;
	hogp_wakeUpAdv_config_set();
	ble_module_enable(1);
	if(!int_advOpen_timer)
		int_advOpen_timer = sys_timeout_add(NULL,ble_closeAdv_handle,5000);//5s timeout
}
u8 get_cur_bt_idx(void)
{
    return cur_bt_idx;
}
static void ble_reset_addr(u8 *new_addr)
{
    u8 ble_addr[6] = {0};

    ble_module_enable(0);
 	memcpy(ble_addr,new_addr,sizeof(ble_addr));

    log_info("new ble address:");
    put_buf(new_addr, 6);
    le_controller_set_mac((void *)new_addr);
    if(fbt_donglePair)
		fbt_donglePair = 0;
 	modify_ble_pair_name();//重填广播信息

	ble_module_enable(1);
}

void user_key_deal(u8 user_key, u8 timeout)
{
    int ret = 0;
    u8 tmp_ble_addr[6];
    u8 vm_hid_mode = HID_MODE_NULL;

    log_info("user_event:0x%x\n", user_key);

    log_info(">>>bt_hid_mode:%d\n", bt_hid_mode);
    if (bt_led_mode == BT_PAIR_LED_CONNECT)
		return ;
    ble_module_enable(0);
    kb_24g_mode_set(0);
    bt_led_mode = BT_PAIR_LED_PAIRING;

    cur_bt_idx = user_key;
    syscfg_write(CFG_CUR_BT_IDX, &cur_bt_idx, 1);              //��¼��ǰʹ����������

    if (timeout) {                              //����ǳ�����ʱֱ�Ӵ�3.0 �� 4.0 �����VM��Ϣ
        log_info("timeout\n");
        get_random_number(tmp_ble_addr, 6);               //��������BLE��ַ
        syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
        ble_bonding_info = 0;
        syscfg_write(CFG_USER_BLE_BONDING,&ble_bonding_info,1);
        ble_reset_addr(tmp_ble_addr);
        return;
    }

#if TCFG_USER_BLE_ENABLE

	if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {		 //read current index ble address error
		get_random_number(tmp_ble_addr, 6);
		syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
	}
	ble_bonding_info = 0;
	syscfg_write(CFG_USER_BLE_BONDING,&ble_bonding_info,1);
	ble_reset_addr(tmp_ble_addr);


#endif

}

/*************************************************************************************************/
/*!
 *  \brief      app 入口
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
extern void le_hogp_set_reconnect_adv_cfg(u8 adv_type, u32 adv_timeout);
extern void le_hogp_set_output_callback(void *cb);
static int hid_keyboard_recv_callback(u8 *buffer, u16 size);
static void hidkey_app_start()
{
    log_info("=======================================");
    log_info("-------------HID DEMO-----------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

#if TCFG_USER_BLE_ENABLE
    /* SFR(JL_CLOCK->PRP_CON1,  0, 3, 1); */
    /* SFR(JL_CLOCK->PRP_CON1, 26, 1, 1); */
    SFR(JL_CLOCK->PRP_CON2, 0, 2, 1);   // wl_and_clk 1:std_48m
    SFR(JL_CLOCK->PRP_CON2, 2, 2, 2);   // wl2adc_clk 1:std_48m 2:pll_96m
    SFR(JL_CLOCK->PRP_CON2, 4, 2, 2);   // wl2dac_clk 1:std_48m 2:pll_96m
    SFR(JL_CLOCK->PRP_CON2, 8, 1, 1);   // wl_aud_rst
    asm("csync");

    bt_pll_para(48000000, 48000000, 0, 0);

    btstack_ble_start_before_init(&hidkey_ble_config, 0);
    /* le_hogp_set_reconnect_adv_cfg(ADV_IND, 5000); */
    le_hogp_set_reconnect_adv_cfg(ADV_DIRECT_IND_LOW, 5000);
    //注册hogp数据接收回调函数
    le_hogp_set_output_callback(hid_keyboard_recv_callback);

    extern void cfg_file_parse(u8 idx);
    cfg_file_parse(0);

    btstack_init();

    //btstack task 使用软中断执行
    /* extern void btstack_task(void); */
    /* extern const int IRQ_BTSTACK_MSG_IP; */
    /* request_irq(IRQ_SOFT3_IDX, IRQ_BTSTACK_MSG_IP, btstack_task, 0); */
#endif
}

void ble_slave_init(void)
{
    if (syscfg_read(CFG_CUR_BT_IDX, &cur_bt_idx, 1) != 1) {
        cur_bt_idx = 0;
        syscfg_write(CFG_CUR_BT_IDX, &cur_bt_idx, 1);
    }
    if(cur_bt_idx ^ 0x3)
		cur_bt_idx =0;
    log_info("use cur_ble_idx:%d\n", cur_bt_idx);
    bt_led_mode =BT_PAIR_LED_DISCON;

    btstack_ble_start_before_init(&hidkey_ble_config, 0);
    //le_hogp_set_reconnect_adv_cfg(ADV_DIRECT_IND, 5000);
    //le_hogp_set_reconnect_adv_cfg(ADV_DIRECT_IND_LOW, 5000);
    //注册hogp数据接收回调函数
    le_hogp_set_output_callback(hid_keyboard_recv_callback);

    extern void cfg_file_parse(u8 idx);
    cfg_file_parse(0);

	bt_set_local_name(remoter_name1,sizeof(remoter_name1));

    u8 tmp_ble_addr[6];

    if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read err
        get_random_number(tmp_ble_addr, 6);
        syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
    }
    le_controller_set_mac((void *)tmp_ble_addr);

    log_info("\n-----change ble 's address-----");
    printf_buf((void *)tmp_ble_addr, 6);

    btstack_init();
}

void bt_init_api(void)
{
    hidkey_app_start();
}

/*************************************************************************************************/
/*!
 *  \brief      hid 数据接收
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hid_keyboard_recv_callback(u8 *buffer, u16 size)
{
    log_info("ble_hid_data_receive:size=%d", size);
    put_buf(buffer, size);

	if((buffer[0] == 0x85)&&(size==4))
	 {
	 	#if 0
		if (buffer[1] & BIT(1)) {	//CAP灯的状态
			r_printf("enter bble callback......\n");
			gpio_direction_output(CAP_LED_PIN, CAP_LED_ON_VALUE);
		} else {
			gpio_direction_output(CAP_LED_PIN, !CAP_LED_ON_VALUE);
		}
		#endif
        if(buffer[3] == 0xaa)
        {
			fbt_donglePair = 1;
            fsys_isAtv = 0;
        }
#if 1
        if (buffer[2] == 1){
            log_info("mic_on......\n");
		    audio_mic_switch(1);
        }
        else{
            log_info("mic_off......\n");
		    audio_mic_switch(0);
        }
#endif
	 }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      app  状态处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hidkey_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_HID_MAIN:
            hidkey_app_start();
            break;
        }
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        log_info("APP_STA_DESTROY\n");
        break;
    }

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙HCI事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hidkey_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙连接状态事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hidkey_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        //根据模式执行对应蓝牙的初始化
#if TCFG_USER_BLE_ENABLE
        btstack_ble_start_after_init(0);
#endif
        fsys_initok = 1;
        if(fkey_wakeUp)
		{
            fhogp_open = 1;
			ble_advOpen_handle();
			break ;
		}

        ble_bonding_info = 0;
        syscfg_read(CFG_USER_BLE_BONDING,&ble_bonding_info,1);

        if(fcc_pair)
        {
            fcc_pair = 0;
            if(ble_bonding_info ^ 0xa3)
            {
                ble_set_fix_pwr(0);
                user_key_deal(0,0);
                break;
            }
        }
        
        if(ble_bonding_info ^ 0xa3)
            ble_module_enable(0);
        else ble_module_enable(1);

        // user_key_deal(0,0);
        break;

    default:

#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif
        break;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙ble hogp状态消息回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void hidkey_hogp_ble_status_callback(ble_state_e status, u8 reason)
{
    u8 array[6];
    log_info("hidkey_hogp_ble_status_callback============== %02x   reason:0x%x\n", status, reason);

    vble_slave_status_update(status, reason);
    switch (status) {
    case BLE_ST_CONNECT:
        log_info("BLE_ST_CONNECT\n");
        ble_set_fix_pwr(7);
        post_msg(1, MSG_BLE_CONNECT_COMPLETE);
        if (cur_bt_idx == 0x3) {
            log_info("write 24g mode\n");
            bt_hid_mode = HID_MODE_BLE_24G;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);

        } else {
            log_info("write ble mode\n");
            bt_hid_mode = HID_MODE_BLE;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);         //保存当前HID模式
        }
        break;

    case BLE_PRIV_MSG_PAIR_CONFIRM:
        log_info("BLE_PRIV_MSG_PAIR_CONFIRM\n");
        break;

    case BLE_PRIV_PAIR_ENCRYPTION_CHANGE:
        log_info("BLE_PRIV_PAIR_ENCRYPTION_CHANGE\n");
        post_msg(1, MSG_BLE_CONNECT_COMPLETE);
        break;

    case BLE_ST_SEND_DISCONN:
        log_info("BLE_ST_SEND_DISCONN\n");
        post_msg(1, MSG_BLE_DISCONNECT_COMPLETE);
        break;

    case BLE_ST_DISCONN:
        log_info("BLE_ST_DISCONN BY LOCAL\n");
        fbt_donglePair = 0;
        bt_led_mode =BT_PAIR_LED_DISCON;
        post_msg(1, MSG_BLE_DISCONNECT_COMPLETE);
        break;

    case BLE_ST_NOTIFY_IDICATE:
        log_info("BLE_ST_NOTIFY_IDICATE\n");
        array[0] = AUDIO_GOOGLE_CMD_CAP;
		extern struct bmts control_t;
		array[1] = (control_t.atv_ctr.version_gcr & 0xff00)>>8;
		array[2] = control_t.atv_ctr.version_gcr & 0x00ff;
        array[5] = control_t.atv_ctr.gcr.interaction_model;
        atv_remote_protocol_deal(array , 6);

        if(ble_bonding_info ^ 0xa3)
		{
			ble_bonding_info =0xa3;
			syscfg_write(CFG_USER_BLE_BONDING,&ble_bonding_info,1);
		}

        bt_led_mode = BT_PAIR_LED_CONNECT;
        post_msg(1, MSG_BLE_CONNECT_COMPLETE);
        fhogp_open = 0;
        break;

    default:
        break;
    }
}
/*************************************************************************************************/
/*!
 *  \brief      蓝牙公共消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hidkey_bt_common_event_handler(struct bt_event *bt)
{
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

    switch (bt->event) {
    case COMMON_EVENT_BLE_REMOTE_TYPE:
        log_info(" COMMON_EVENT_BLE_REMOTE_TYPE,%d \n", bt->value);
        break;

    case COMMON_EVENT_SHUTDOWN_DISABLE:
        /* hidkey_auto_shutdown_disable(); */
        break;

    default:
        break;

    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      app 线程事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hidkey_event_handler(struct application *app, struct sys_event *event)
{

    switch (event->type) {
    case SYS_BT_EVENT:
#if TCFG_USER_BLE_ENABLE
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            hidkey_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            hidkey_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            hidkey_hogp_ble_status_callback(event->u.bt.event, event->u.bt.value);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return hidkey_bt_common_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return 0;
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
static const struct application_operation app_hidkey_ops = {
    .state_machine  = hidkey_state_machine,
    .event_handler 	= hidkey_event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_hidkey) = {
    .name 	= "hid_key",
    .action	= ACTION_HID_MAIN,
    .ops 	= &app_hidkey_ops,
    .state  = APP_STA_DESTROY,
};


#endif
#endif

