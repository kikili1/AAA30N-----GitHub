#include "encoder_mge.h"
#include "audio_adc_api.h"
#include "cpu.h"
#include "config.h"
#include "typedef.h"
#include "hwi.h"
/* #include "dev_manage.h" */
/* #include "fs_io.h" */
/* #include "vfs.h" */
#include "circular_buf.h"
/* #include "a_encoder.h" */
/* #include "mp3_encoder.h" */
#include "clock.h"
// #include "ble_user.h"
#include "user_cfg.h"
#include "trans_packet.h"
#include "rf_send_queue.h"
#include "audio2rf_send.h"
#include "msg.h"
#include "app_reg_include.h"
#include "rc_led.h"
#if ENCODER_IMA_EN
#include "ima_encoder.h"
#endif
#if ENCODER_SBC_EN
#include "sbc_encoder.h"
#endif
#include "app_gyro_lib.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[audio_enc]"
#include "log.h"

static u16 gg_timerout1_id = 0;
static u16 gg_timerout2_id = 0;
// u8 enc_buffer[240];
static u16 frame_no = 0;
static u8 interaction_model_pre = 0;

typedef struct _self_apdcm_conext_ {
    int index;
    int valpred;
} self_apdcm_conext;
static self_apdcm_conext enc_objt;

struct bmts control_t = {
    .mic_busy_flag = 0,
    .aosp_interaction_model = HTT_MODE,
    .atv_ctr = {
        .frame_buf = {0},
        .stream_id = 0,
        .frame_num = 0,
        .version_gcr = 0,
        // .gc = {0},
        .gcr = {
            .gcr_version = GOOGLE_VOICE_VERSION_1P0,
            .bytes_frame = VOICE_V0P4_ADPCM_PACKET_LEN,
            .bytes_characteristic = VOICE_V0P4_ADPCM_FRAME_LEN,
            .codec_supported = CODEC_SUPPORTED_16K,
            .interaction_model = AOSP_MODE,
            .extra_config = 0x00,
            .reserved_byte = 0x00,
        }

    }
};
#define AUDIO_TR_BUF_LEN  2048//10480
#define AUDIO_TR_MTU      240
#define AUDIO_TR_HEAD      6
#define LOOP_TIMER  50
static u8 audio_tr_cbuf[AUDIO_TR_BUF_LEN] __attribute__((aligned(4)));//����ʹ��,�������󼸱�
static u8 audio_tr_rbuf[AUDIO_TR_MTU*4] __attribute__((aligned(4)));//�̶߳�ȡ�뷢��
static u8 audio_tr_tbuf[AUDIO_TR_MTU+AUDIO_TR_HEAD] __attribute__((aligned(4)));//�̶߳�ȡ�뷢��
///cbuf�����������жϽ�������
static cbuffer_t audio_tr_cbuft;

#define cbuf_get_space(a) (a)->total_len
typedef struct {
    u8   audio_id;
    u32  rx_len;
}__attribute__((packed)) audio_tr_head_t;

static void audio_tr_cbuf_init(void)
{
    cbuf_init(&audio_tr_cbuft, audio_tr_cbuf, sizeof(audio_tr_cbuf));
}
static void audio_tr_cbuf_exit(void)
{
    cbuf_clear(&audio_tr_cbuf);
}

static int audio_tr_cbuf_is_write_able(u32 len)
{
    u32 wlen, buf_space;
    /* OS_ENTER_CRITICAL(); */
    /* cbuf_is_write_able() */
    buf_space = cbuf_get_space(&audio_tr_cbuft) - cbuf_get_data_size(&audio_tr_cbuft);
    if (buf_space < len) {
        wlen = 0;
    }else {
        wlen = len;
    }
    /* OS_EXIT_CRITICAL(); */
   log_info("%s[%d %d %d]", __func__, len, buf_space, wlen);
    return wlen;
}
static int audio_tr_ibuf_to_cbuf(u8 *buf, u32 len)
{
    int ret = 0;
    u16 wlen = 0;
    u16 head_size = sizeof(audio_tr_head_t);
    audio_tr_head_t rx_head;
    rx_head.audio_id = 1; //use uart1
    OS_ENTER_CRITICAL();
    if (audio_tr_cbuf_is_write_able(len)) {
        rx_head.rx_len = len;
        // wlen = cbuf_write(&audio_tr_cbuft, &rx_head, head_size);
        wlen += cbuf_write(&audio_tr_cbuft, buf, rx_head.rx_len);
        if (wlen != (rx_head.rx_len)) {
            ret = -1;
        } else {
            ret = wlen;
        }
    }else{
        ret = -2;
    }
    OS_EXIT_CRITICAL();
    if(ret < 0)
    log_info("cbuf_full!!!,ret:%d",ret);
    return ret;
}
static int audio_tr_cbuf_read(u8 *buf, u32 len)
{
    int ret = 0;
    u16 rlen = 0;
    u16 head_size = sizeof(audio_tr_head_t);
    audio_tr_head_t rx_head;
    OS_ENTER_CRITICAL();
    if (cbuf_get_data_size(&audio_tr_cbuft) < len) {
        /* OS_EXIT_CRITICAL(); */
        ret =  -1;
    }else{

        rlen = cbuf_read(&audio_tr_cbuft, buf, len);
        if(rlen != len){
            ret =  -3;
        }
    }
    OS_EXIT_CRITICAL();
//    gg_debug("%s[%d 0x%x ]", __func__, ret, len);
    return ret;
}

void enc_wait_stop_stream(enc_obj *obj)
{
    log_info("stop encode C\n");
    u32 i = 0x10000;
    while (0 != cbuf_get_data_size(obj->p_obuf) && (0 != i)) {
        if (obj->enable & B_ENC_FULL) {
            break;
        }
        kick_rf_queue_isr();
        delay(100);
        i--;
    }
    /* HWI_Uninstall(IRQ_SOFT2_IDX); */
}
/*----------------------------------------------------------------------------*/
/**@brief   发送编码信息并启动编码器
   @param   enc_fun     :编码器选择，可传入ump2或者opus编码器初始化接口
            enc_type    :编码对应的解码类型选择
   @return
   @author
   @note    初始化编码器后会将编码信息发送给远端，再将编码链路挂载到AUDIO_ADC中断
            调用该函数前需确保AUDIO_ADC已工作
*/
/*----------------------------------------------------------------------------*/
enc_obj *audio2rf_encoder_io(u32(*enc_fun)(void *, void *, void *),  AUDIO_FORMAT enc_type)
{

    /* enc_hdl = (void *)enc_fun(NULL, enc_input, rf_enc_output); */
    enc_obj *obj = encoder_io(enc_fun, enc_input, rf_enc_output, NULL);
    if (NULL == obj) {
        return NULL;
    }
    // obj->wait_output_empty = enc_wait_stop_stream;
    /* #if RF_SENDER_USE_QUEUE */
    // rf_send_soft_isr_init(obj->p_obuf);
    /* #endif */
    /* audio2rf_start_cmd(obj->info.sr, obj->info.br, enc_type); */
    return obj;
}
u32 audio2rf_encoder_start(enc_obj *obj)
{
    if (NULL != obj) {
        obj->enable = B_ENC_ENABLE;
        start_encode();//adc_enable();
    }
    return 0;
}

void audio2rf_encoder_stop(ENC_STOP_WAIT wait)
{
    stop_encode_phy(wait);
    HWI_Uninstall(IRQ_SOFT2_IDX);
}

void audio_mic_switch(u8 mode)
{
	if(bt_led_mode==BT_PAIR_LED_CONNECT)
	{
		if(mode)
		{
			if (!fmic_vld) {
				// clk_set("sys", SEND_AUDIO_SYS_CLK);
				// delay_10ms(30);
                control_t.mic_busy_flag = 1;
                memset(&enc_objt,0,sizeof(enc_objt));
				log_info("SPEAKER_START\n");
				fmic_vld =1;
				audio_adc_init_api(16000, AUDIO_ADC_MIC, 0);
                audio_tr_cbuf_init();
                if(fsys_isAtv || fbt_donglePair)
                    audio2rf_encoder_start(audio2rf_encoder_io(ima_encode_api, FORMAT_IMA));
				else audio2rf_encoder_start(audio2rf_encoder_io(sbc_encode_api, FORMAT_SBC)); 
			} 
			else return ;
		}else {
			if(fmic_vld)
			{
				//send_audio_data
				audio2rf_encoder_stop(ENC_NEED_WAIT);
				audio_adc_off_api();
                audio_tr_cbuf_exit();
				// clk_set("sys", IDLE_SYS_CLK);
				// delay_10ms(30);
                control_t.mic_busy_flag = 0;
                fmic_send_busy = 0;
				fmic_vld = 0;
                if(!fkey_mic)
                {
                    switch(interaction_model_pre)
                    {
                        case PTT_MODE:
                        case HTT_MODE:
                            control_t.atv_ctr.gcr.interaction_model = interaction_model_pre;
                            break;
                        default:
                            break;
                    }
                }
                fkey_mic = 0;
				log_info("SPEAKER_STOP\n");
			}
			else return ;
		}
	}
    else if(!mode)
    {
        if(fmic_vld)
        {
            //send_audio_data
            audio2rf_encoder_stop(ENC_NEED_WAIT);
            audio_adc_off_api();
            audio_tr_cbuf_exit();
            // clk_set("sys", IDLE_SYS_CLK);
            // delay_10ms(30);
            control_t.mic_busy_flag = 0;
            fmic_send_busy = 0;
            fmic_vld =0;
            log_info("SPEAKER_STOP\n");
        }
        else return ;
    }
}
//�豸�¼���Ӧdemo
extern u8 exchange_att_mtu_size,sbc_att_mtu_size,sbc_tail_ct;

void audio_msg_handle(void)
{
    u32 rlen = 0;
    int ret = 0;
    u16 cur_byte_ct = 0;
    u8 resend_ct = 0;
    u8 val_index[3];

    while(control_t.mic_busy_flag){
        extern void clr_wdt(void);
        clr_wdt();
        fmic_send_busy = 1;
        if(fbt_donglePair)
        {
            memset(audio_tr_tbuf,0x00,sizeof(audio_tr_tbuf));
            ret = audio_tr_cbuf_read(&audio_tr_tbuf, 128+3);//ջ����ȡֵ���ܻ�����
            if(ret < 0)
            {
                fmic_send_busy = 0;
                return;
            }
            ret = ATVV_CHAR_AUDIO(&audio_tr_tbuf[3],128);
            while(ret!= 0)
            {
                extern void clr_wdt(void);
                clr_wdt();
                ret = ATVV_CHAR_AUDIO(&audio_tr_tbuf[3],128);
                if(++resend_ct>3)
                {
                    resend_ct = 0;
                    break;
                }
            }
        }
        else
        {
            if(fsys_isAtv)
            {
                memset(audio_tr_tbuf,0x00,sizeof(audio_tr_tbuf));
                ret = audio_tr_cbuf_read(&audio_tr_tbuf[3], 128+3);//ջ����ȡֵ���ܻ�����
                // log_info("ret:%d\n",ret);
                if(ret < 0)
                {
                    fmic_send_busy = 0;
                    return;
                }
                audio_tr_tbuf[0] = frame_no >> 8;
                audio_tr_tbuf[1] = frame_no & 0xff;
                audio_tr_tbuf[2] = 0;
                frame_no ++;
                // val_index[0] = audio_tr_tbuf[3];
                // val_index[1] = audio_tr_tbuf[4];
                // val_index[2] = audio_tr_tbuf[5];
                // audio_tr_tbuf[3] = valpred_value >> 8;;
                // audio_tr_tbuf[4] = valpred_value & 0xff;
                // audio_tr_tbuf[5] = enc_objt.index;
                // log_info("valpred:%x,index:%x\n",enc_objt.valpred,enc_objt.index);
                // log_info("control_t.atv_ctr.version_gcr:%x\n",control_t.atv_ctr.version_gcr);
                // adpcm_ima_coder_noheader(audio_tr_rbuf, &audio_tr_tbuf[AUDIO_TR_HEAD], &enc_objt.index, &enc_objt.valpred, 128*2);
                switch(control_t.atv_ctr.version_gcr)
                {
                    case GOOGLE_VOICE_VERSION_0P4:
                        ret = ATVV_CHAR_AUDIO(audio_tr_tbuf,(128+AUDIO_TR_HEAD));
                        while(ret!= 0)
                        {
                            extern void clr_wdt(void);
                            clr_wdt();
                            ret = ATVV_CHAR_AUDIO(audio_tr_tbuf,(128+AUDIO_TR_HEAD));
                            if(++resend_ct>3)
                            {
                                resend_ct = 0;
                                break;
                            }
                        }
                        // enc_objt.valpred = 0;
                        // enc_objt.valpred |= val_index[0]<<8;
                        // enc_objt.valpred |= val_index[1];
                        // enc_objt.index = val_index[2];
                        break;
                    case GOOGLE_VOICE_VERSION_1P0:
                        ret = ATVV_CHAR_AUDIO(&audio_tr_tbuf[AUDIO_TR_HEAD],128);
                        while(ret!= 0)
                        {
                            extern void clr_wdt(void);
                            clr_wdt();
                            ret = ATVV_CHAR_AUDIO(&audio_tr_tbuf[AUDIO_TR_HEAD],128);
                            if(++resend_ct>3)
                            {
                                resend_ct = 0;
                                break;
                            }
                        }
                        break;

                    default:
                        break;
                }

            }
            else
            {
                // log_info("msbc_send_len:%d\n",msbc_att_mtu_size);
                ret = audio_tr_cbuf_read(audio_tr_tbuf, sbc_att_mtu_size);//??????????
                // log_info("msbc_send_len:%d\n",ret);
                if(ret < 0)
                {
                    fmic_send_busy = 0;
                    return;
                }
                switch(control_t.aosp_interaction_model)
                {
                    case ON_REQUEST_MODE:
                        aosp_onquest_hanlde(audio_tr_tbuf,audio_tr_tbuf,sbc_att_mtu_size);
                        break;
                    case HTT_MODE:
                        aosp_htt_handle(audio_tr_tbuf,sbc_att_mtu_size);
                        break;

                    default:
                        break;
                }            
                ret = AOSP_CHAR_AUDIO(audio_tr_tbuf, sbc_att_mtu_size);
                while(ret!= 0)
                {
                    extern void clr_wdt(void);
                    clr_wdt();
                    putchar('F');
                    ret = AOSP_CHAR_AUDIO(audio_tr_tbuf,sbc_att_mtu_size);
                    if(++resend_ct>3)
                    {
                        putchar('D');
                        resend_ct = 0;
                        break;
                    }
                }
            }
        }
    }
}
static u16 swap16_HL(u16 swap_data)
{
    u16 temp_H = 0,temp_L = 0;

    temp_H = swap_data & 0xff00;
    temp_L = swap_data & 0x00ff;
    swap_data = 0;
    swap_data |= temp_L << 8;
    swap_data |= temp_H >> 8;
    return swap_data;
}
static u8 swap8_HL(u8 swap_data)
{
    u8 temp_H = 0,temp_L = 0;

    temp_H = swap_data & 0xf0;
    temp_L = swap_data & 0x0f;
    swap_data = 0;
    swap_data |= temp_L << 4;
    swap_data |= temp_H >> 4;
    return swap_data;
}

static u16 atv_remote_send_data_insert_str(void *priv ,u16 len ,u16 offest)
{
    u8 *data = (u8 *)priv;
    memcpy(control_t.atv_ctr.frame_buf + offest ,data ,len);
    return len;
}
static u16 atv_remote_send_data_insert_co(u16 c ,u16 len ,u16 offest)
{
    memcpy(control_t.atv_ctr.frame_buf + offest ,&c ,len);
    // control_t.atv_ctr.frame_buf[offest] = c;
    return len;
}
#define ATV_SEND_DATA_INSERT_S(a ,b ,c)   atv_remote_send_data_insert_str(a ,b ,c)
#define ATV_SEND_DATA_INSERT_C(a ,b ,c)   atv_remote_send_data_insert_co(a ,b ,c)

static u8 rsp_cap_data[] = {0x00, 0x04, 0x00, 0x02, 0x00, 0x86, 0x00, 0x86};
void gg_timerout2(void *priv)
{
    u16 offest = 0;
    gg_timerout1_id = 0;
	gg_timerout2_id = 0;
    offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_AUDIO_STOP,1,offest);
    offest += ATV_SEND_DATA_INSERT_C(GOOGLE_AUDIO_STOP_TIMEROUT,1,offest);
    remote_respod_atv_event(ATV_MIC_CHAR_CTL_AUDIO_STOP,control_t.atv_ctr.frame_buf ,offest);
    audio_mic_switch(0);
}
void atv_remote_protocol_deal(void *priv ,u16 len)
{
    if(!priv)
    {
        return;
    }
    u8 *buf = (u8 *)priv;
    u16 offest = 0;
    
    memset(control_t.atv_ctr.frame_buf ,0 ,VOICE_V0P4_ADPCM_FRAME_LEN);

    log_info("%s\n",__func__);
    // put_buf(priv,len);
    switch (buf[0])
    {
    case AUDIO_GOOGLE_CMD_CAP:
        log_info("AUDIO_GOOGLE_CMD_CAP\n");
        offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_CAPS_RESP,1 ,offest);
        switch((buf[1]<<8) | buf[2])
        {
            // case 0://?remote caps????grc,??version 0.4
            case GOOGLE_VOICE_VERSION_0P4:
                offest += ATV_SEND_DATA_INSERT_S(rsp_cap_data ,8 ,offest);
                control_t.atv_ctr.version_gcr = GOOGLE_VOICE_VERSION_0P4;
                // fsys_isAtv = 1;
                break;
            case GOOGLE_VOICE_VERSION_1P0:
                offest += ATV_SEND_DATA_INSERT_C(swap16_HL(control_t.atv_ctr.gcr.gcr_version),2 ,offest);
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.gcr.codec_supported,1 ,offest);
                switch(buf[5])
                {
                    case ONLY_ONREQUEST:
                        control_t.atv_ctr.gcr.interaction_model = ON_REQUEST_MODE;
                        break;
                    case PTT_ON_REQUEST:
                        control_t.atv_ctr.gcr.interaction_model = PTT_MODE;
                        break;
                    case HTT_PTT_ON_REQUEST:
                        control_t.atv_ctr.gcr.interaction_model = HTT_MODE;
                        break;
                    default:
                        break;
                }
                log_info("interaction_model:%x\n",control_t.atv_ctr.gcr.interaction_model);
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.gcr.interaction_model,1 ,offest);
                offest += ATV_SEND_DATA_INSERT_C(swap16_HL(control_t.atv_ctr.gcr.bytes_frame),2 ,offest);
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.gcr.extra_config,1 ,offest);
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.gcr.reserved_byte,1 ,offest);
                control_t.atv_ctr.version_gcr = GOOGLE_VOICE_VERSION_1P0;
                // fsys_isAtv = 1;
                break;

            default: 
                offest += ATV_SEND_DATA_INSERT_S(rsp_cap_data ,8 ,offest);
                control_t.atv_ctr.version_gcr = GOOGLE_VOICE_VERSION_0P4;
                break ;

        }
        // put_buf(control_t.atv_ctr.frame_buf ,offest);
        ATVV_CHAR_CTL(control_t.atv_ctr.frame_buf ,offest);
        /* code */
        break;

    case AUDIO_GOOGLE_CMD_OPEN:
        log_info("AUDIO_GOOGLE_CMD_OPEN\n");
        fsys_isAtv = 1;
        if(gg_timerout1_id){
            sys_timeout_del(gg_timerout1_id);
            gg_timerout1_id = 0;
        }
        offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_AUDIO_START,1 ,offest);
        switch(control_t.atv_ctr.version_gcr)
        {
            case GOOGLE_VOICE_VERSION_0P4:               
                break;
            case GOOGLE_VOICE_VERSION_1P0:
                switch(control_t.atv_ctr.gcr.interaction_model)
                {
                    case PTT_MODE:
                    case HTT_MODE:
                        if(!fkey_mic)
                        {
                            interaction_model_pre = control_t.atv_ctr.gcr.interaction_model;
                            control_t.atv_ctr.gcr.interaction_model = ON_REQUEST_MODE;
                        }
                        break;
                    default:
                        break;
                }
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.gcr.interaction_model,1 ,offest);
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.gcr.codec_supported,1 ,offest);
                if(control_t.atv_ctr.gcr.interaction_model)
                {
                    if(++control_t.atv_ctr.stream_id > 0x80)
                        control_t.atv_ctr.stream_id = 1;
                }
                else control_t.atv_ctr.stream_id = 0x00;
                offest += ATV_SEND_DATA_INSERT_C(control_t.atv_ctr.stream_id,1 ,offest);
                break;
            default:
                break;
        }
        ATVV_CHAR_CTL(control_t.atv_ctr.frame_buf ,offest);
		frame_no =0;
        audio_mic_switch(1);

        if(control_t.atv_ctr.gcr.interaction_model ^ HTT_MODE)
        {
            if(!gg_timerout2_id){
                gg_timerout2_id = sys_timeout_add(NULL, gg_timerout2, GG_TIMEROUT2);
            }else{
                sys_timer_modify(gg_timerout2_id,GG_TIMEROUT2);
            }
        }
	
        break;


    case AUDIO_GOOGLE_CMD_CLOSE:
        log_info("AUDIO_GOOGLE_CMD_CLOSE\n");
        offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_AUDIO_STOP,1 ,offest);
        if(control_t.atv_ctr.gcr.interaction_model ^ HTT_MODE)
            offest += ATV_SEND_DATA_INSERT_C(0x00,1 ,offest);
        else offest += ATV_SEND_DATA_INSERT_C(0x02,1 ,offest);
        ATVV_CHAR_CTL(control_t.atv_ctr.frame_buf ,offest);
        audio_mic_switch(0);

        if(gg_timerout1_id){
            sys_timeout_del(gg_timerout1_id);
            gg_timerout1_id = 0;
        }

        if(gg_timerout2_id){
            sys_timeout_del(gg_timerout2_id);
            gg_timerout2_id = 0;
        }
        
        break;
	case AUDIO_GOOGLE_CMD_EXTEND:
        log_info("AUDIO_GOOGLE_CMD_EXTEND\n");
        if(gg_timerout2_id)
            sys_timer_modify(gg_timerout2_id,GG_TIMEROUT2);
        else gg_timerout2_id = sys_timeout_add(NULL, gg_timerout2, GG_TIMEROUT2);
        break;
    // case AUDIO_GOOGLE_CMD_TIMEOUT:
    //     gg_debug("AUDIO_GOOGLE_CMD_TIMEOUT\n");
    //     offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_AUDIO_STOP,1 ,offest);
    //     offest += ATV_SEND_DATA_INSERT_C(GOOGLE_AUDIO_STOP_TIMEROUT,1 ,offest);
    //     ATVV_CHAR_CTL(control_t.atv_ctr.frame_buf ,offest);
    //     extern void audio_adc_demo_close(void);
    //     audio_adc_demo_close();
    //     sync_cnt = 0;
    //     if(gg_timerout1_id){
    //         sys_timeout_del(gg_timerout1_id);
    //         gg_timerout1_id = 0;
    //     }

    //     if(gg_timerout2_id){
    //         sys_timeout_del(gg_timerout2_id);
    //         gg_timerout2_id = 0;
    //     }
    //     break;
    default:
        break;
    }
}
static u16 aosp_timerout_id = 0;
void aosp_timeout_handle(void)
{
    log_info("AOSP_onquest_timeout\n");
    if(aosp_timerout_id)
    {
        sys_timeout_del(aosp_timerout_id);
        aosp_timerout_id = 0;
    }
    audio_mic_switch(0);
}
void aosp_remote_protocol_deal(void *priv ,u16 len)
{
    if(!priv)
    {
        return;
    }
    u8 *buf = (u8 *)priv;
    u16 offest = 0;
    memset(control_t.atv_ctr.frame_buf ,0 ,VOICE_V0P4_ADPCM_FRAME_LEN);

    log_info("%s\n",__func__);
    // put_buf(priv,len);
    switch (buf[0])
    {
    case AOSP_CMD_OPEN:
        log_info("AOSP_CMD_OPEN\n");
        fsys_isAtv = 0;
        audio_mic_switch(1);
        if(!control_t.aosp_interaction_model)
        {
            if(!aosp_timerout_id)
                aosp_timerout_id = sys_timeout_add(NULL,aosp_timeout_handle,AOSP_TIMEROUT);
            else sys_timer_modify(aosp_timerout_id,AOSP_TIMEROUT);
        }
        break;


    case AOSP_CMD_CLOSE:
        log_info("AOSP_CMD_CLOSE\n");
        fsys_isAtv = 0;
        audio_mic_switch(0);
        if(aosp_timerout_id)
        {
            sys_timeout_del(aosp_timerout_id);
            aosp_timerout_id = 0;
        }
        break;
    default:
        break;
    }
}

void gg_timerout1(void *priv)
{
	printf("gg_timerout1\n");
    u16 offest = 0;
    gg_timerout1_id = 0;
    // audio_mic_switch(0);
    offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_MIC_OPEN_ERROR,1,offest);
    offest += ATV_SEND_DATA_INSERT_C(swap16_HL(GOOGLE_MIC_OPEN_ERROR_RESERVED),2,offest);
    remote_respod_atv_event(ATV_MIC_CHAR_CTL_MIC_OPEN_ERROR,control_t.atv_ctr.frame_buf ,offest);
}

int remote_respod_atv_event(u8 event ,void *priv ,u16 len)
{
    u16 offest = 0;
    u8 frame_buf[2];

    switch (event)
    {
    case ATV_MIC_CHAR_CTL_SYNC:
        putchar('s');
        frame_buf[0] = frame_no >> 8;
        frame_buf[1] = frame_no & 0xff;
        offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_SYNC,1,offest);
        offest += ATV_SEND_DATA_INSERT_S(frame_buf,sizeof(frame_buf),offest);

        break;
    case ATV_MIC_CHAR_CTL_SEARCH:
        // audio_mic_switch(1);
        offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_SEARCH,1,offest);
        if(!gg_timerout1_id){
            gg_timerout1_id = sys_timeout_add(NULL, gg_timerout1, GG_TIMEROUT1);
        }else{
            sys_timer_modify(gg_timerout1_id,GG_TIMEROUT1);
        }
        break;
    case ATV_MIC_CHAR_CTL_MIC_OPEN_ERROR:
        // offest += ATV_SEND_DATA_INSERT_C(ATV_MIC_CHAR_CTL_MIC_OPEN_ERROR,1,offest);
        offest = len;
        break;
    case ATV_MIC_CHAR_CTL_AUDIO_STOP:
        offest = len;
    default:
        break;
    }
    ATVV_CHAR_CTL(control_t.atv_ctr.frame_buf ,offest);
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief   编码器输出数据并进行无线传输
   @param   priv:私有结构体，此处为编码句柄
            data:输出数据buff
            len :输出数据长度
   @return  告知编码器本次输出的实际长度
   @author
   @note    无线传输模块出错导致无法正常发送时停止编码，发送繁忙时等待下次编码输出
*/
/*----------------------------------------------------------------------------*/
u32 rf_enc_output(void *priv, u8 *data, u16 len)
{
    static u16 cur_cbuf_len = 0;
    u8 sbc_str[60];
    static u8 i = 0;
    u8 j;
    int ret;

    enc_obj *obj = priv;
    if (NULL == obj) {
        return 0;
    }

    if (obj->enable & B_ENC_INIT) {
        /* 忽略部分编码格式输出的头信息 */
        return len;
    }
    if(fbt_donglePair)
    {
        audio_tr_ibuf_to_cbuf(data,len);
        goto report_msg_speaker;
    }
    if(fsys_isAtv)
    {
        for(j = 3;j < len;j++)
            data[j] = swap8_HL(data[j]);//编码高低位互换
        audio_tr_ibuf_to_cbuf(data,len);
        goto report_msg_speaker;
    }
    if(!fbt_donglePair && !fsys_isAtv)
    {
        cur_cbuf_len += len + 2;
        i++;
        switch(i)
        {
            case 1:sbc_str[1] = 0x08;break;
            case 2:sbc_str[1] = 0x38;break;
            case 3:sbc_str[1] = 0xc8;break;
            case 4:sbc_str[1] = 0xf8;i = 0;break;

            default:
                break;
        }
        sbc_str[0] = 0x01;
        memcpy(&sbc_str[2], data, len);
        audio_tr_ibuf_to_cbuf(sbc_str,len + 2);
        if(cur_cbuf_len>=sbc_att_mtu_size)
        {
            cur_cbuf_len = 0;
            goto report_msg_speaker;
        }
    }
    // u32 err = audio2rf_send_packet(AUDIO2RF_DATA_PACKET, data, len);

    if (0 != obj->indata_kick_size) {
        /* ibuf数据大于一帧数据 且 蓝牙压包缓存大于一帧数据 */
        if (cbuf_get_data_size(obj->p_ibuf) >= obj->indata_kick_size) {
            kick_encode_isr();
        }
    }
    return len;
report_msg_speaker:
    if(!fmic_send_busy)
    {
        ret = post_msg(1, MSG_SPEAKER);
        if (ret != 0) {
            log_info("error !!! msg pool full \n");
        }
    }
    if (0 != obj->indata_kick_size) {
        /* ibuf数据大于一帧数据 且 蓝牙压包缓存大于一帧数据 */
        if (cbuf_get_data_size(obj->p_ibuf) >= obj->indata_kick_size) {
            kick_encode_isr();
        }
    }
    return len;
}

#include "power_manage.h"
static u8 rc_enc_idle_query(void)
{
   return !control_t.mic_busy_flag;
   // return 1;
}

REGISTER_LP_TARGET(rc_enc_lp_target) = {
   .name = "rc_enc",
   .is_idle = rc_enc_idle_query,
};