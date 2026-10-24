#ifndef __REC_API_H__
#define __REC_API_H__

#include "typedef.h"
#include "audio_enc_api.h"
#include "sound_mge.h"
#include "audio_adc.h"
#include "trans_packet.h"

#define B_ENC_ENABLE  BIT(0)
#define B_ENC_STOP    BIT(1)
#define B_ENC_FULL    BIT(2)
#define B_ENC_INIT    BIT(3)

typedef struct _enc_obj {
    void *p_file;
    void *p_ibuf;
    void *p_obuf;
    void *enc_ops;
    void *p_dbuf;
    ENC_DATA_INFO info;
    volatile u32 enable;
    u32 indata_kick_size;
    void (*wait_output_empty)(void *);
} enc_obj;

typedef enum  {//停止编码时，是否需要将ADC中剩余的样点消耗完
    ENC_NO_WAIT = 0,
    ENC_NEED_WAIT = 1,
} ENC_STOP_WAIT;

extern enc_obj *enc_hdl;
void start_encode(void);


u16 enc_input(void *priv, s16 *buf, u8 channel, u16 len);
// u32 enc_output(void *priv, u8 *data, u16 len);

void stop_encode(void *pfile, u32 dlen);
void stop_encode_phy(ENC_STOP_WAIT wait);
enc_obj *encoder_io(u32(*fun)(void *, void *, void *), void *input_func, void *output_func, void *pfile);
void wfil_soft2_isr_hook(enc_obj *hdl);

void kick_encode_api(void *obj);
void enc_wfile_init(void);
void rec_cbuf_init(void *cbuf_t);
void audio_mic_switch(u8 mode);


#define  kick_encode_isr()   bit_set_swi(1)
#define  kick_wfile_isr()    bit_set_swi(2)


// #if FPGA
#ifdef FPGA
typedef struct _adc_obj {
    void *p_adc_cbuf;
    volatile u32 enable;
} adc_obj;

#define B_DAC_ENABLE  BIT(0)
extern adc_obj adc_hdl;
#define REC_ADC_CBUF  adc_hdl.p_adc_cbuf

#else
extern sound_out_obj rec_sound;
#define REC_ADC_CBUF  rec_sound.p_obuf
#endif


/**************************************************************
 ******************   google voice    *********************
 **************************************************************/
#define AUDIO_GOOGLE_CMD_CAP                0x0A
#define AUDIO_GOOGLE_CMD_OPEN               0x0C
#define AUDIO_GOOGLE_CMD_CLOSE              0x0D
#define AUDIO_GOOGLE_CMD_EXTEND             0x0E

#define AOSP_CMD_OPEN               0x01
#define AOSP_CMD_CLOSE              0x00

#define AUDIO_GOOGLE_CMD_TIMEOUT            0x0F

#define ATV_MIC_CHAR_CTL_CAPS_RESP          0x0B
#define ATV_MIC_CHAR_CTL_AUDIO_START        0x04
#define ATV_MIC_CHAR_CTL_AUDIO_STOP         0x00
#define ATV_MIC_CHAR_CTL_SYNC               0x0A
#define ATV_MIC_CHAR_CTL_MIC_OPEN_ERROR     0x0C
#define ATV_MIC_CHAR_CTL_SEARCH             0x08
#define ATV_MIC_CHAR_CTL_DPAD_SELECT        0x0

#define CODEC_USED_8K                       0x0001
#define CODEC_USED_16K                      0x0002
#define CODEC_USED_OPUS                     0x0004      //not support

#define CODEC_SUPPORTED_8K                  0x01        //ADPCM,8khz/16bit
#define CODEC_SUPPORTED_16K                 0x02        //ADPCM,16khz/16bit(default)
#define CODEC_SUPPORTED_8K16K               0x03        //ADPCM (8khz/16bit & 16khz/16bit)
#define CODEC_SUPPORTED_8KOPUS              0x05        //Opus and ADPCM 8khz/16bit
#define CODEC_SUPPORTED_8K16KOPUS           0x07        //Opus and ADPCM (8khz/16bit & 16khz/16bit)

#define ON_REQUEST_MODE                     0x00
#define PTT_MODE                            0x01
#define HTT_MODE                            0x03
#define AOSP_MODE                           0xff

#define ONLY_ONREQUEST                      0X00
#define PTT_ON_REQUEST                      0x01
#define HTT_PTT_ON_REQUEST                  0x03

#define GOOGLE_VOICE_VERSION_1P0            0x0100
#define GOOGLE_VOICE_VERSION_0P4            0x0004
#define GOOGLE_VOICE_VERSION_DEFAULT_0P4    0

#define ERROR_INVALIED_CODEC                0x0F01
#define ERROR_CODE                          ERROR_INVALIED_CODEC

#define VOICE_V0P4_ADPCM_PACKET_LEN         0x0080   //128 bytes   //20bytes
#define VOICE_V0P4_ADPCM_FRAME_LEN          0x0014    //20bytes //(256 + 6)bytes

#define GET_CAPS_VERSION_MAJOR              1
#define GET_CAPS_VERSION_MINOR              2
#define GET_CAPS_VERSION_CODEC_SUPPORTED    3


#define GOOGLE_MIC_OPEN_ERROR_RESERVED             0X010F

#define GOOGLE_AUDIO_STOP_TIMEROUT                 0X08


/**************************************************************
 ******************   google voice    *********************
 **************************************************************/
#define AUDIO_SYNC_INTERVAL 350
#define GG_TIMEROUT1        3000
#define GG_TIMEROUT2        15000
#define AOSP_TIMEROUT       15000

struct version
{
    u8 major;
    u8 minor;
};

struct get_caps
{
    struct version ver;
    u16 codec_supported;
};

struct get_caps_resp
{
    u8  codec_supported;
    u8  interaction_model;
    u8  extra_config;
    u8  reserved_byte;
    u16 bytes_frame;
    u16 bytes_characteristic;
    u16 gcr_version;
};

struct atv
{
    // struct get_caps gc;
    struct get_caps_resp gcr;
    u16 codec_used;
    u16 frame_num;
    u16 version_gcr;
    u8  stream_id;
    u8 frame_buf[VOICE_V0P4_ADPCM_FRAME_LEN];
}__attribute((aligned (4)));

struct  bmts
{
    u8 mic_busy_flag;
    u8 aosp_interaction_model;
    // struct bmts_buf_ctl *ctl;
    struct atv atv_ctr;
};

extern int google_voice_atvv_char_send(u8 mode ,u8 *buf ,u16 len);
#define ATVV_CHAR_AUDIO(buf ,len)   google_voice_atvv_char_send(0 , buf ,len)//audio2rf_send_packet(AUDIO2RF_DATA_PACKET, buf,len);//google_voice_atvv_char_send(0 , buf ,len)
#define ATVV_CHAR_CTL(buf ,len)   google_voice_atvv_char_send(1 , buf ,len)
#define AOSP_CHAR_AUDIO(buf ,len)   google_voice_atvv_char_send(2 , buf ,len)
    

void atv_remote_protocol_deal(void *priv ,u16 len);
void aosp_remote_protocol_deal(void *priv ,u16 len);

int remote_respod_atv_event(u8 event ,void *priv ,u16 len);
void audio_msg_handle(void);

#endif
