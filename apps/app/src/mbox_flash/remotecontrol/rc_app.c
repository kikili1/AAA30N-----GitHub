#pragma bss_seg(".app_rc.data.bss")
#pragma data_seg(".app_rc.data")
#pragma const_seg(".app_rc.text.const")
#pragma code_seg(".app_rc.text")
#pragma str_literal_override(".app_rc.text.const")

#include "audio_rf_mge.h"
#include "audio2rf_send.h"
#include "trans_packet.h"
#include "rf2audio_recv.h"
#include "rc_app.h"
#include "app_modules.h"
#include "app.h"

#include "cpu.h"
#include "config.h"
#include "typedef.h"
#include "vfs.h"
#include "hot_msg.h"
#include "msg.h"
#include "crc16.h"
#include "circular_buf.h"

#include "decoder_api.h"
#include "decoder_mge.h"
#include "decoder_msg_tab.h"
#include "encoder_mge.h"
#include "sound_mge.h"
#include "audio.h"
#include "audio_dac_api.h"
#include "audio_adc_api.h"
#include "audio_dac.h"
#include "audio_adc.h"
#include "ir_encoder.h"
#include "gpio.h"
#include "clock.h"
#include "bt_ble.h"
#include "vble_complete.h"
#include "usb/device/hid.h"
#include "hot_msg.h"
#include "power_api.h"
#include "tick_timer_driver.h"
#include "update.h"
#include "encoder_stream.h"
#include "rc_key.h"
#include "rc_led.h"
#include "rc_ir.h"
#include "app_reg_include.h"
#include "rc_gyro.h"
#include "app_power_mg.h"
#include "rc_cswitch.h"
#if ENCODER_UMP3_EN
#include "ump3_encoder.h"
#endif

#if DECODER_UMP3_EN
#include "ump3_encoder.h"
#endif

#if ENCODER_IMA_EN
#include "ima_encoder.h"
#endif

#if DECODER_IMA_EN
#include "ima_api.h"
#endif

#if ENCODER_SBC_EN
#include "sbc_encoder.h"
#endif

#if ENCODER_SPEEX_EN
#include "speex_encoder.h"
#endif

#if DECODER_SBC_EN
#include "sbc_api.h"
#endif

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[rf_controller]"
#include "log.h"

#if (RF_REMOTECONTROL_MODE_EN & TRANS_DATA_HID_EN)

/* 外部变量/函数声明 */
extern void bt_init_api(void);
extern void ble_module_enable(u8 en);
// extern u32 rf_enc_output(void *priv, u8 *data, u16 len);

// static int remote_audio_send_api(u8 *data, u16 len)
// {
//     if(fsys_isAtv || fbt_donglePair)
//         return vble_slave_send_api(ATT_SLV2MSTR_ATV_IDX, data, len);
//     else return vble_slave_send_api(ATT_SLV2MSTR_AOSP_IDX, data, len);
// }
// static int rc_check_status_api(void)
// {
//     int ble_status;
//     vble_ioctl(VBLE_IOCTL_GET_SLAVE_STATUS, (int)&ble_status);
//     if (ble_status == BLE_ST_NOTIFY_IDICATE) {
//         return 1;
//     } else {
//         return 0;
//     }
// }
// static int rc_get_valid_len_api(void)
// {
//     return get_buffer_vaild_len(0);
// }
// const audio2rf_send_mge_ops remote_ops = {
//     .send = remote_audio_send_api,
//     .check_status = rc_check_status_api,
//     .get_valid_len = rc_get_valid_len_api,
// };

static u8 rc_ctrl_adc_buff[1280] AT(.rec_data);
void rec_cbuf_init(void *cbuf_t)
{
    cbuf_init(cbuf_t, &rc_ctrl_adc_buff[0], sizeof(rc_ctrl_adc_buff));
}

void rf_controller_uninit()
{
    if (fmic_vld) {
        audio2rf_encoder_stop(ENC_NO_WAIT);
        fmic_vld =0;
        log_info("SPEAKER_STOP\n");
    }

#if BLE_EN
    ble_module_enable(0);
#endif
    // audio2rf_send_register(NULL);
    audio_adc_off_api();

}

void rf_rc_app()
{
    int msg[2];
    u32 err;
    // clk_set("sys", IDLE_SYS_CLK);
    // delay_10ms(30);
    audio_rf_clr_buf();
    u32 ble_status = 0;

    /* ir_encoder_init(IO_PORTB_03, 38000, 5000); */
    /* 蓝牙BLE初始化 */
#if BLE_EN
    /* bt_init_api(); */
    vble_slave_init_api();
    // audio2rf_send_register(&remote_ops);
#endif
    log_info("rf_controller\n");

    while (1) {
        sys_power_led_pmu();
        if (!fmic_vld && !fir_star) {
            /* putchar('s'); */
            /* u32 sr = dac_sr_read(); */
            audio_off();
            sys_power_down(2000000);
            audio_init();
            /* dac_power_on(sr); */
        }

        err = get_msg(2, &msg[0]);
        switch (msg[0]) {
        case MSG_MATRIX_KEY:
        case MSG_IO_MOUSE:
            /* send_ir_key(msg[0], 0); */
            // log_info("msg[0]:%d\n",msg[0]);
            rf_send_hid_key();
            //send_hid_data
            break;
        case MSG_SPEAKER://audio_data
            audio_msg_handle();
            break;
        case MSG_CODE_SWITCH:
            #if TCFG_CODE_SWITCH
            code_switch_deal(cswitch_report_value);
            cswitch_report_value = 0;
            #endif
            break;
        case MSG_POWER_OFF:
            flow_pow = 1;
            app_power_led_handler();
            break;
        case MSG_BLE_APP_UPDATE_START:
        /* 手机APP升级 */
        case MSG_BLE_TESTBOX_UPDATE_START:
        /* 测试盒蓝牙升级 */
        case MSG_UART_TESTBOX_UPDATE_START:
            /* 测试盒串口升级 */
            app_update_handle(msg[0]);
            break;
        // case MSG_FCC_SING:
        //     ble_module(0,0);
        //     break;
        case MSG_CHANGE_WORK_MODE:
            goto __remotecontrol_exit;
        case MSG_500MS:
            app_power_scan();
            /* log_info("ble status 0x%x\n", ble_status); */
            wdt_clear();
        default:
            /*     ap_handle_hotkey(msg[0]); */
            break;

        }
    }
__remotecontrol_exit:
    rf_controller_uninit();
}

#endif


