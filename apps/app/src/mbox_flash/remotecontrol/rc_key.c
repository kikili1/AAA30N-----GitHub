#include "typedef.h"
#include "rc_key.h"
#include "msg.h"
#include "app_config.h"
#include "rc_app.h"
#include "config.h"
#include "gpio.h"
#include "vble_complete.h"
#include "bt_ble.h"
#include "rc_ir.h"
#include "rc_led.h"
#include "app_reg_include.h"
#include "encoder_mge.h"
#include "rc_gyro.h"
#include "rc_cswitch.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[RC_KEY]"
#include "log.h"

KEY_STRUCT  _key;
MOUSE_STRUCT  _mouse;
IR_STRUCT  _ir_struct;

_bool_int  _key_bool_bit;
_bool_byte	_rf_bool_bit;
_bool_int	_gy_bool_bit;
_bool_int  _system_bool_bit;

static u8 is_rckey_active=0;
static u8 pair_step = 0;
static u32 bt_pair_id = 0;

u8 RGB_CT = 0;
u8 ATV_STEP = 0;
u8 RF_STEP = 0;

void rc_matrix_col_relase(void)
{
	for (u8 i = 0; i < sizeof(rckey_col); i++) {
		gpio_hw_direction_output(IO_PORT_SPILT(rckey_col[i]),0);
	}
}
void rc_matrix_key_init(void)
{
    // log_info("key init\n");
    u8 i = 0;

	krow_num =0;
    for (i = 0; i < sizeof(rckey_row); i++) { //key 上拉输入
	 	
	 	gpio_hw_set_direction(IO_PORT_SPILT(rckey_row[i]), 1);

		gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_row[i]), 1);

        gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_row[i]), 0);
	 	gpio_hw_set_die(IO_PORT_SPILT(rckey_row[i]), 1);
	 	gpio_hw_set_dieh(IO_PORT_SPILT(rckey_row[i]), 1);
		gpio_set_mode(IO_PORT_SPILT(rckey_row[i]), PORT_INPUT_PULLUP_100K);
	 krow_num |=1<<i;
    }

	kcol_num =0;
    for (i = 0; i < sizeof(rckey_col); i++) { //c 上拉输入

        gpio_hw_set_direction(IO_PORT_SPILT(rckey_col[i]), 1);

        gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_col[i]), 1);
        gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_col[i]), 0);
        gpio_hw_set_die(IO_PORT_SPILT(rckey_col[i]), 1);
        gpio_hw_set_dieh(IO_PORT_SPILT(rckey_col[i]), 1);
		gpio_set_mode(IO_PORT_SPILT(rckey_col[i]), PORT_INPUT_PULLUP_100K);
		kcol_num |=1<<i;
    }
	//rc_matrix_col_relase();
}


void matrix_key_col_status(enum gpio_mode mode)
{
	for (u8 i = 0; i < sizeof(rckey_col); i++)
	{
		switch (mode)
		{
		case PORT_INPUT_PULLUP_100K:
		case PORT_INPUT_PULLUP_10K:
		case PORT_INPUT_PULLUP_1M:
			gpio_hw_set_direction(IO_PORT_SPILT(rckey_col[i]), 1);
			gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_col[i]), 1);
			gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_col[i]), 0);
			break;
		case PORT_INPUT_PULLDOWN_10K:
		case PORT_INPUT_PULLDOWN_100K:
		case PORT_INPUT_PULLDOWN_1M:
			gpio_hw_set_direction(IO_PORT_SPILT(rckey_col[i]), 1);
			gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_col[i]), 0);
			gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_col[i]), 1);
			break;
		case PORT_OUTPUT_LOW:
			gpio_hw_direction_output(IO_PORT_SPILT(rckey_col[i]),0);
			break;
		case PORT_OUTPUT_HIGH:
			gpio_hw_direction_output(IO_PORT_SPILT(rckey_col[i]),1);
			break;
		default:
			break;
		}
		gpio_set_mode(IO_PORT_SPILT(rckey_col[i]), mode);
	}
}
void matrix_key_row_status(enum gpio_mode mode)
{
	for (u8 i = 0; i < sizeof(rckey_row); i++)
	{

		switch (mode)
		{
		case PORT_INPUT_PULLUP_100K:
		case PORT_INPUT_PULLUP_10K:
		case PORT_INPUT_PULLUP_1M:
			gpio_hw_set_direction(IO_PORT_SPILT(rckey_row[i]), 1);
			gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_row[i]), 1);
			gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_row[i]), 0);
			break;
		case PORT_INPUT_PULLDOWN_10K:
		case PORT_INPUT_PULLDOWN_100K:
		case PORT_INPUT_PULLDOWN_1M:
			gpio_hw_set_direction(IO_PORT_SPILT(rckey_row[i]), 1);
			gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_row[i]), 0);
			gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_row[i]), 1);
			break;
		case PORT_OUTPUT_LOW:
			gpio_hw_direction_output(IO_PORT_SPILT(rckey_row[i]),0);
			break;
		case PORT_OUTPUT_HIGH:
			gpio_hw_direction_output(IO_PORT_SPILT(rckey_row[i]),1);
			break;
		default:
			break;
		}
		gpio_set_mode(IO_PORT_SPILT(rckey_row[i]), mode);
	}
}


static void clr_bool_key(void)
{
	flong_key =0;
	fn_pair =0;
	fn_ok =0;
	fir_send =0;
	flong_mic_key = 0;
	_mouse_bool_byte =0;
	// ATV_STEP = 0;
}

const u16 matrix_key_table[COW_MAX+COW_ADK][ROW_MAX];
static unsigned char KEY_P;
static u8  key_len =0;
static u8  PRE_BUFF[COW_MAX+COW_ADK]={0};
u8 key_scan_sub(u8 col,u8 cur_key_value,u8 *key_buf,u8 key_ct)
{
	u8 cur_key_change,cur_key_break,cur_key_new;
	u8 row;
	u8 col_write_value,col_read_value;

	if(col>=COW_ADK)
	{
		col_read_value =0;
		for(row=0;row<sizeof(rckey_col);row++)
			col_read_value |=gpio_read(rckey_col[row])<<row;
		gpio_hw_set_direction(IO_PORT_SPILT(rckey_col[col-COW_ADK]), 1);
		cur_key_value = ~cur_key_value;
	}
	cur_key_value = cur_key_value&krow_num;
	cur_key_change =cur_key_value^PRE_BUFF[col];
	cur_key_break = (cur_key_change | cur_key_value)^cur_key_value;
	cur_key_new =cur_key_change ^cur_key_break;
	if(cur_key_change)
		{
			if(col>=COW_ADK)
			{
			 col_write_value = kcol_num;
			 col_write_value &=~(1 << (col  - COW_ADK));

			 row =cur_key_value-1;
			 if((row & cur_key_value)&krow_num)
				{
					if((col_read_value ^ col_write_value)&kcol_num)
					{
						log_info("col_read_value:%x  col_write_value:%x ", col_read_value, col_write_value);
						return key_ct;
					}
				}
			}

		key_check =1;
		if(key_check_again)
		{
			PRE_BUFF[col] =cur_key_value;
			for (row = 0; row < ROW_MAX; row ++)
			{
				if(cur_key_new&0x01)
					key_len++;
				if(cur_key_break &0x01)
					key_len--;
				if(cur_key_value &0x01)
				{
					log_info("row:%d  col:%d   [SHORT]\n", row, col);
					key_buf[key_ct] =row;
					key_buf[key_ct] |=col<<4;
					if(key_ct<10)
						key_ct++;
				}
				cur_key_new>>=1;
				cur_key_break>>=1;
				cur_key_value>>=1;
			}
		}
	}
	return key_ct;
}

void rc_key_scan(void)
{
    u8 row, col,m;
	u8 tab_buf[10],tab_ct,matrix_event;
	u16 key_value;

	if(key_check)
		key_check_again =1;
	matrix_event =0;
	tab_ct =0;
	KEY_P =0;
// #if TCFG_RGB_ENABLE
// 	MOT_led_set(1);//屏蔽背光灯
// 	delay(50);
// #endif
	for(m=0;m<sizeof(rckey_row);m++)
		p33_io_wakeup_enable(rckey_row[m],0);

	matrix_key_row_status(PORT_INPUT_PULLUP_100K);
	matrix_key_col_status(PORT_INPUT_PULLUP_100K);
	delay(50);
	key_value =0;
	col =0;
	for (row = 0; row < sizeof(rckey_row); row ++)
		key_value |= gpio_read(rckey_row[row])<<row;
	m = (~key_value)&krow_num;
	tab_ct =key_scan_sub(col,m,tab_buf,tab_ct);

	if(!m)
	{
		for (col = COW_ADK; col < COW_MAX+COW_ADK; col ++)
		{
			gpio_hw_direction_output(IO_PORT_SPILT(rckey_col[col-COW_ADK]),0);
			delay(50);
			key_value = 0;
			for (row = 0; row < sizeof(rckey_row); row ++)
				key_value |= gpio_read(rckey_row[row])<<row;
			
			tab_ct =key_scan_sub(col,key_value,tab_buf,tab_ct);
		}
	}
	matrix_key_row_status(PORT_INPUT_PULLUP_100K);
#if TCFG_RGB_ENABLE
	blt_led_set();
	if(fled_r || fled_g || fled_b)
		is_rckey_active =35;
#endif

	if(key_len)
	{
		fkey_doing =1;
		if(fkey_blt_rem)
		{
			fkey_blt=1;
			fkey_blt_rem =0;
		}
		is_rckey_active =35;
		if(key_len - 1)
			fkey_double_pre = 1;
	}
	else {
		fkey_doing =0;
		fkey_double_pre = 0;
		if(is_rckey_active){
			fkey_led=1;
			is_rckey_active--;
		}
			
		if(!is_rckey_active){
			fkey_led=0;
			rc_key_matrix_sleep();
		}
	}
	if(!key_check_again)
	{
		if(key_check)
		{
			key_len =0;
			memset(&PRE_BUFF, 0, sizeof(PRE_BUFF));
		}
	}
	else {
		key_check = 0;
		memset(&_key, 0, sizeof(KEY_STRUCT));
		clr_bool_key();
		// log_info("matrix_event\n");
		// put_buf(tab_buf,tab_ct);
		for(m=0;m<tab_ct;m++)
		{
			key_value =matrix_key_table[tab_buf[m]>>4][tab_buf[m]&0xf];
			key_value &=~0x8000;
			if(key_value==K_OK)
				fn_ok =1;
		}
		void matrix_key_map_deal(u8 );
		for(m=0;m<tab_ct;m++)
			matrix_key_map_deal(tab_buf[m]);

		key_check_again =0;

		matrix_event |=0x1;

	}

    if (matrix_event)//sys_key_event_enable();开按键消息
	{
		int ret = post_msg(1, MSG_MATRIX_KEY);
		if (ret != 0) {
			key_printf("error !!! msg pool full \n");
		}
	}
} 

//key TABLE
const u16 matrix_key_table[COW_MAX + COW_ADK][ROW_MAX]=
{                                        	
//R0		R1			R2			R3			R4			R6
{K_GOOGP	,K_WUp		,K_WApp		,K_MIC		,K_SET 	    ,K_NULL	},//c0
{K_Y		,K_OK		,K_WLeft	,K_Sleep	,K_WRight	,K_NULL},//c1
{K_YOUT		,K_WDown	,K_WBACK	,K_TOUCH	,K_TV		,K_NULL},//c2  
{K_PRIME	,K_WHOME	,K_VOLI		,K_NULL		,KPu	    ,K_B},//c3
{K_NETF		,K_MUTE		,K_VOLD		,K_NULL		,KPd	    ,K_NULL},//C4
};
const u8 matrix_key_ir_table[COW_MAX + COW_ADK][ROW_MAX]=
{                                        
//R0		R1			R2			R3			R4			R6
{0x71		,0x00		,0x1A		,0x22		,0x80	,0xFE},//c0
{0x74		,0x1f		,0x03		,0x0a		,0x02	,0xFE},//c1
{0x72		,0x01		,0x1C		,0x4A		,0x1B	,0xFE},//c2  
{0x73		,0x0c		,0x58		,0xfe		,0x51	,0x75},//c3
{0x86		,0x0D		,0x5d		,0xfe		,0x52	,0xFE},//c4
};

void matrix_key_map_deal(u8 offset)
{
	u8 tab_h,tab_l;
	u16 tab;
	u8 array[2];

	tab_h =offset>>4;
	tab_l =offset&0XF;

	tab =matrix_key_table[tab_h][tab_l];
	IR_DATA =matrix_key_ir_table[tab_h][tab_l];
	if((BT_PAIR_LED_CONNECT ^ bt_led_mode) && !fkey_double_pre)//ir data
	{
		fir_send =1;
		RF_STEP = 1;
	}

	if(ftmode_up)
	{
		if(tab == K_WUp)
		{
			fcc_pair = 1;
			fir_send = 0;
			RF_STEP = 0;
			tab = 0;
		}
	}
	if(fn_ok)
	{
		if(tab == K_VOLD)
		{
			flong_key = 1;
			fkey_pair = 1;
			fir_send = 0;
			pair_step = 0;
			tab = 0;
		}
#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
		else if(tab == KPd)
		{
			flong_key =1;
			firk_clrr =1;
			tab =0;
		}
#endif
#if(TCFG_GYRO_ENABLE)
		if(fgy_inside)
		{
			if(tab == K_TOUCH)
			{
				fgyro_calib = 1;
				tab = 0;
			}
			else if(tab==KPu && (dpi_data<3))
			{
				fdpi_save =1;
				dpi_data++;
				ir_led_times = 8;
				tab = 0;
			}
			else if(tab==KPd && dpi_data)
			{
				fdpi_save =1;
				dpi_data--;
				ir_led_times = 8;
				tab =0;
			}
			if(fdpi_save)
			{
				fdpi_save =0;
				RF_STEP = 6;
				// array[0] =0x39;
				// array[1] =dpi_data;
				// syscfg_write(CFG_USER_dpi_INDEX, array, 2);
			}
		}
#endif
	}


	if(tab ==K_Sleep)
	{
		if(bt_led_mode)
		{
			if(fsys_initok)
			{
				log_info("press sleep\n");
				fkey_wakeUp = 1;
				extern void ble_advOpen_handle(void);
				ble_advOpen_handle();
			}
			else 
			{
				log_info("noinit wake\n");
				fkey_wakeUp = 1;
			}
		}

#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
		if(fir_study)
		{
			RF_STEP = 7;
			// user_ir_learn_timeout();
			return;
		}
		flong_key =1;
		firk_learn =1;
		IR_KEY_NUM =0;
		if(ir_valid_key & (1<<IR_KEY_NUM))
		{
			fir_send = 1;
			RF_STEP=1;
			fir_study_send = 1;
			tab =0;
		}
#endif

	}
	if(fir_send)
		tab = 0;
	
	if(tab==K_MIC)
	{
		flong_mic_key =1;
		fkey_mic =1;
		ATV_STEP = 0;
		tab =0;
	}

	if(!fgy_inside)
	{
		if(fm_open)//������Ĭ�Ϲ� ����Ĭ�Ͽ�
		{
			if(tab == K_WUp)
			{
				tab = 0;
				MOUSE_KEY_U = 1;
			}
			if(tab == K_WLeft)
			{
				tab = 0;
				MOUSE_KEY_L = 1;
			}
			if(tab == K_WRight)
			{
				tab = 0;
				MOUSE_KEY_R = 1;
			}
			if(tab == K_WDown)
			{
				tab = 0;
				MOUSE_KEY_D = 1;
			}
			if(tab == K_OK)
				tab = K_ML;
		}
	}
	else
	{
		if(fm_open)
		{
			if(tab == K_OK)
				tab = K_ML;
		}
	}
   

	tab_h =tab>>8;
	tab_l =tab&0x00ff;

	switch(tab_h)
	{
		case 0x00:
			if(KEY_P>=6)
			KEY_TYPE[0] =KEY_TYPE[1] =KEY_TYPE[2] =KEY_TYPE[3] =KEY_TYPE[4] =KEY_TYPE[5] =1;
			else
			{
				_key.TYPE[KEY_P] =tab_l;
				KEY_P++;
			}
			break;
		case 0x21:
			SYS_KEY =tab_l;
			break;
		case 0x31:
			MOUSE_KKEY |=tab_l;
			if(MOUSE_KKEY&0x1)
				fgy_key_pause =1;
			break;
		case 0x41:
			KEY_MODIF |=tab_l;
			break;
		case 0x50:
			switch(tab_l)
			{
				case K_TOUCH & 0x00ff:
					if(!bt_led_mode)
						fm_open =~fm_open;
					break;
				default:
					break;
			}
			break;

		default:
			if((tab_h&0xf0) == 0x10) //��ý�尴��
			{
				MULITI_KEY1 =tab_l;
				MULITI_KEY2 =tab_h&0x0F;
			}
			break;
	}

}

_bool_byte	_mouse_bool_bit;
u8 	MOUSE_PRE;
void MOUSE_CHECK(void)
{
	static u8 MOUSE_DATA,MOUSE_ct;
	u8 buf_1,buf_2;
	u8 GY_X =0,GY_Y =0;

	if(fm_open)
	{
		buf_1 =_mouse_bool_byte;
		buf_1 = buf_1^MOUSE_PRE;
		buf_1 &=0X0F;
		if(buf_1)
		{
			MOUSE_DATA =2;
			MOUSE_ct =0;
			MOUSE_PRE =_mouse_bool_byte;
		}
		if(_mouse_bool_byte)
		{
			MOUSE_ct++;
			if(MOUSE_ct>=30)
			{
				MOUSE_ct =0;
				MOUSE_DATA =MOUSE_DATA+2;
				if(MOUSE_DATA>30)
					MOUSE_DATA =30;
			}
			buf_1 =MOUSE_DATA;
			buf_2 =buf_1;
			if(MOUSE_KEY_L)
			{
				buf_1 =-buf_1;
				GY_X =buf_1;
			}
			else if(MOUSE_KEY_R)
				GY_X =buf_1;
			if(MOUSE_KEY_U)
			{
				buf_2 =-buf_2;
				GY_Y =buf_2;
			}
			else if(MOUSE_KEY_D)
				GY_Y =buf_2;
		}
	}
	if(GY_X || GY_Y)
	{
		MOUSE_X =GY_X;
		MOUSE_Y =GY_Y;
		post_msg(1,MSG_IO_MOUSE);
	}
}

static u16 key_break_timer;

static void sys_ir_study_sub(void)
{
	fir_study =1;
	fir_send =0;
	if(ir_study_timer)
		sys_timer_modify(ir_study_timer,20);
	else ir_study_timer =sys_timeout_add(NULL,system_ir_rece,20);
}
static void sys_key_break_timeout(void)
{
	int ret = 0;
	key_break_timer =0;
	extern struct bmts control_t;
	if(MULITI_KEY1 == 0x21 && !fbt_donglePair && control_t.aosp_interaction_model)
		fkey_break = 1;
	MULITI_KEY1 =0;
	MULITI_KEY2 =0;
	MOUSE_KKEY =0;
	SYS_KEY =0;
	KEY_TYPE[1]=0;
	fir_send = 0;
	
	ret = post_msg(1, MSG_MATRIX_KEY);
	if (ret != 0) {
		key_printf("error !!! msg pool full \n");
	}
}
void long_key_ir_set(void)
{
	static u16  int_long_key_ct;
	static u8  int_ir_led_fre_ct,int_ir_led_100ms_ct;
	u8 notify,only_notify = 0;
	u8 array[1];
	u16 i;
	int ret = 0;
	extern struct bmts control_t;

	notify =0;
	if(flong_key)
	{
		i=150;
#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
		if(firk_learn || firk_clrr)
			i =500;
#endif
		if(++int_long_key_ct>=i)
		{
			int_long_key_ct =0;
			flong_key =0;

			if(fkey_pair)
			{
				fkey_pair =0;
				RF_STEP = 3;
				only_notify = 1;
				// ble_set_fix_pwr(7);
				// extern void user_key_deal(u8 , u8);
				// user_key_deal(0,0);
			}
#if TCFG_FCC_SINGLE
			else if(fsys_fccSingle)
			{
				fsys_fccSingle = 0;
				fir_send = 0;
				bt_led_mode = BT_PAIR_LED_FCC;
				int ret = post_msg(1, MSG_FCC_SING);
				if (ret != 0) {
					key_printf("error !!! msg pool full \n");
				}
			}
#endif
#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
			else if(firk_learn)
		    {
				printf("firk_learn valid");
				firk_learn =0;
				RF_STEP = 4;
				only_notify = 1;
				// sys_ir_study_sub();
				// int_500ms_wait_study_ct =6;
		    }
			else if(firk_clrr)
			{
				printf("firk_clrr valid");
				firk_clrr =0;
				fir_send = 0;
				RF_STEP = 5;
				only_notify = 1;
				// system_ir_clrr_key();
				ir_led_times =10;
			}
#endif
		}
	}
	else {
		int_long_key_ct =0;
		if(fkey_pair)
			fkey_pair =0;
		else if(fsys_fccSingle)
			fsys_fccSingle = 0;
		else if(firk_learn)
			firk_learn = 0;
		else if(firk_clrr)
			firk_clrr = 0;
	}
#if 1
	if(flong_mic_key)
	{
		switch(ATV_STEP)
		{
			case 0:
				MULITI_KEY1 =0x21;
				MULITI_KEY2 =0x02;
				switch(control_t.atv_ctr.version_gcr)
				{
					case GOOGLE_VOICE_VERSION_0P4:
						fatv_caps = 1;
						RF_STEP = 2;
						// remote_respod_atv_event(ATV_MIC_CHAR_CTL_SEARCH,NULL,0);
						break;
					case GOOGLE_VOICE_VERSION_1P0:
						switch(control_t.atv_ctr.gcr.interaction_model)
						{
							case ON_REQUEST_MODE:
								fatv_caps = 1;
								RF_STEP = 2;
								// remote_respod_atv_event(ATV_MIC_CHAR_CTL_SEARCH,NULL,0);
								break;
							case PTT_MODE:
							case HTT_MODE:
								MULITI_KEY1 = 0;
								MULITI_KEY2 = 0;
								break;
							
							default:
								break;
						}
					default:
						break;
				}
				notify = 1;
				ATV_STEP++;
				break;
			case 1:
				switch(control_t.atv_ctr.gcr.interaction_model)
				{
					case ON_REQUEST_MODE:
						break;
					case PTT_MODE:
					case HTT_MODE:
						fatv_micOpen = 1;
						RF_STEP = 2;
						only_notify = 1;
						break;
					
					default:
						break;
				}
				ATV_STEP++;
				break;
			case 2:
				if(fkey_break)
				{
					KEY_TYPE[0]= 0xaa;
					only_notify = 1;
				}
				break;
			default:
				break;
		}
	}
	else
	{
		switch(ATV_STEP)
		{
			case 2:
				if(control_t.atv_ctr.gcr.interaction_model == HTT_MODE)
				{
					fatv_micClose = 1;
					RF_STEP = 2;
				}
				if(fkey_break)
				{
					fkey_break = 0;
					KEY_TYPE[0]= 0;
				}
				only_notify = 1;
				ATV_STEP = 0;
				break;
			default:
				ATV_STEP = 0;
				break;
		}
	}
#endif

	if(++int_ir_led_100ms_ct>=10)
	{
		int_ir_led_100ms_ct =0;
		ftmode_up = 0;
		if(firk_learn)
			fir_led_ir =1;
		if(fir_study)
		{
			if(fir_study_star)
			{
				if(++int_ir_led_fre_ct>=4)//慢闪频率控制
				{
					int_ir_led_fre_ct =0;
					fir_led_ir =~fir_led_ir;
				}
			}
			else {
				fir_led_ir =~fir_led_ir;//长按学习键常亮到这里快闪

				if(!fkey_doing)
				{
					if(int_500ms_wait_study_ct)//松开按键延时600ms 指示灯常亮
					{
						int_500ms_wait_study_ct--;
						fir_led_ir =1;
					}
				}
			}
		}
		if(ir_led_times)
		{
			ir_led_times--;
			fir_led_ir =~fir_led_ir;
		}
   	}


   if(fir_study || ir_led_times  || firk_clrr || fir_send ||firk_learn)
	 fir_led=1;
   else fir_led =0;

	if(only_notify)
	{
		only_notify = 0;
		ret = post_msg(1, MSG_MATRIX_KEY);
		if (ret != 0) {
			key_printf("error !!! msg pool full \n");
		}
	}
   	if(notify)
	{
		notify =0;
		ret = post_msg(1, MSG_MATRIX_KEY);
		if (ret != 0) {
			key_printf("error !!! msg pool full \n");
		}
		
		if(!key_break_timer)
			key_break_timer =sys_timeout_add(NULL,sys_key_break_timeout,120);	   
		else sys_timer_modify(key_break_timer,120);
	}

}

#if TCFG_CODE_SWITCH
u16 cs_break_timer = 0,cs_irBreak_timer = 0;
static void sys_cs_break_timeout(void)
{
	u8 array[2];

	cs_break_timer = 0;
	array[0] =0;	    //page up
	array[1] =0;
	vble_slave_send_api(COUSTOM_CONTROL_REPORT_ID, array, 2);
	fkey_doing = 0;
}
static void sys_cs_IRbreak_timeout(void)
{
	cs_irBreak_timer = 0;
	fir_send = 0;
	fkey_doing = 0;
}
void code_switch_deal(s8 code_switch_value)
{
	u8 array[2];

	log_info("code_switch_value:%x\n",code_switch_value);
    if(!bt_led_mode)
	{
		if(code_switch_value == 1)
		{
			array[0] = 0x29;//mulit_1
		 	array[1] = 0x00;//mulit_2
			vble_slave_send_api(COUSTOM_CONTROL_REPORT_ID, array, 2);
			cs_break_timer =sys_timeout_add(NULL,sys_cs_break_timeout,100);
		}
		if(code_switch_value == -1)
		{
			array[0] = 0x30;//mulit_1
		 	array[1] = 0x00;//mulit_2
			vble_slave_send_api(COUSTOM_CONTROL_REPORT_ID, array, 2);
			cs_break_timer =sys_timeout_add(NULL,sys_cs_break_timeout,100);
		}
	}
	else 
	{
		if(code_switch_value == 1)
		{
			fir_send = 1;
			system_ir_only_send(0x88,0x90,0x80);
			cs_irBreak_timer = sys_timeout_add(NULL,sys_cs_IRbreak_timeout,100);
		}
		if(code_switch_value == -1)
		{
			fir_send = 1;
			system_ir_only_send(0x88,0x90,0x81);
			cs_irBreak_timer = sys_timeout_add(NULL,sys_cs_IRbreak_timeout,100);
		}
	}
}
#endif

u8 MOUSE_KEY_TOUCH=0;
void rf_send_hid_key(void)
{
    u32 ble_status = 0;
    u8 array[11];
    static u8 kb_data[8]={0},muli_key[2]={0},sys_key=0;

	switch(RF_STEP)
	{
		case 0://key_send
			break;
		case 1://ir_send
			if(fir_send)//ir data
			{
				if(!fir_study_send)
				{
					if(IR_DATA^0xfe)
					{
						IR_ADDR = 0x09F7;
						if(system_ir_only_send(IR_ADDR>>8,IR_ADDR&0x00ff,IR_DATA))//红外busy，需要重新发送
						{
							int ret = post_msg(1, MSG_MATRIX_KEY);
							if (ret != 0) {
								key_printf("error !!! msg pool full \n");
							}
						}
					}
				}
				else system_ir_check();
			}
			if(!bt_led_mode)
				RF_STEP = 0;
			break;
		case 2://atv
			if(fatv_micOpen)
			{
				fatv_micOpen = 0;
				array[0] = AUDIO_GOOGLE_CMD_OPEN;
				atv_remote_protocol_deal(array , 1);
			}
			if(fatv_micClose)
			{
				fatv_micClose = 0;
				array[0] = AUDIO_GOOGLE_CMD_CLOSE;
				atv_remote_protocol_deal(array , 1);
			}
			if(fatv_caps)
			{
				fatv_caps = 0;
				remote_respod_atv_event(ATV_MIC_CHAR_CTL_SEARCH,NULL,0);
			}
			RF_STEP = 0;
			break;
		case 3://key_pair
			ble_set_fix_pwr(7);
			extern void user_key_deal(u8 , u8);
			user_key_deal(0,0);
			break;
		case 4://ir_learn
#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
			sys_ir_study_sub();
			int_500ms_wait_study_ct =6;
#endif
			break;
		case 5://ir_clear
#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
			system_ir_clrr_key();
#endif
			break;
		case 6://dpi_updown
#if(TCFG_GYRO_ENABLE)
			array[0] =0x39;
			array[1] =dpi_data;
			syscfg_write(CFG_USER_dpi_INDEX, array, 2);
#endif
			break;
		case 7://ir_study_stop
#if TCFG_IR_STUDY_ENABLE && TCFG_IR_ENABLE
			user_ir_learn_timeout();
#endif
			break;
		default:
			break;
	}
	if(RF_STEP)
	{
		RF_STEP = 0;
		return;
	}

	memcpy(array, (u8 *)&_key, 11);

	array[0] |=MOUSE_KEY_TOUCH;
	if(array[0]^MOUSE_KEY || MOUSE_X || MOUSE_Y || MOUSE_Z || MOUSE_H)
	{
		// log_info("MOUSE_X:%x,MOUSE_Y:%x\n",MOUSE_X,MOUSE_Y);
		MOUSE_KEY =array[0];
		if (BT_PAIR_LED_CONNECT == bt_led_mode)
			vble_slave_send_api(MOUSE_POINT_REPORT_ID, (u8 *)&_mouse, 5);
		memset(&_mouse.X,0,sizeof(_mouse) - 1);
	}
	if(array[1]^sys_key)
	{
		sys_key =array[1];
		
		if (BT_PAIR_LED_CONNECT == bt_led_mode)
			vble_slave_send_api(SYS_CONTROL_REPORT_ID, (u8 *)&array[1], 1);
	}
	if((array[2]^ muli_key[0]) || (array[3]^ muli_key[1]))
	{
		muli_key[0] =array[2];
		muli_key[1] =array[3];
		if (BT_PAIR_LED_CONNECT == bt_led_mode)
			vble_slave_send_api(COUSTOM_CONTROL_REPORT_ID, muli_key, 2);
	}
	if((array[4]^ kb_data[0]) || (array[5]^ kb_data[2]) || (array[6]^ kb_data[3]) || (array[7]^ kb_data[4]) || 
		(array[8]^ kb_data[5]) || (array[9]^ kb_data[6]) || (array[10]^ kb_data[7]))
	{
		kb_data[0] =array[4];
		kb_data[1] =0;
		kb_data[2] =array[5];
		kb_data[3] =array[6];
		kb_data[4] =array[7];
		kb_data[5] =array[8];
		kb_data[6] =array[9];
		kb_data[7] =array[10];
		put_buf(kb_data,8);
		if (BT_PAIR_LED_CONNECT == bt_led_mode)
			vble_slave_send_api(KEYBOARD_REPORT_ID, kb_data, 8);
	}

	extern void hid_battery_timer_handler(void);
	hid_battery_timer_handler();
} 
static u8 rckey_tick_ct=0;
void rckey_tick_scan(void)
{
	if(++rckey_tick_ct>=5)
	{
		rckey_tick_ct =0;
		#if TCFG_CODE_SWITCH
		code_switch_handler(NULL);
		#endif
		if(!is_rckey_active)
		{
			for (u8 row = 0; row < sizeof(rckey_row); row ++)
			{
				if(!gpio_read(rckey_row[row]))
					is_rckey_active =35;
			}
		}
		if(fir_led)
			is_rckey_active =35;
		if(is_rckey_active)
		{
			rc_key_scan();
			MOUSE_CHECK();
			long_key_ir_set();
		}
	}
}

#include "power_interface.h"
void rckey_active_set(P33_IO_WKUP_EDGE edge)
{
	rc_key_init();
	log_info("wakeup\n");
}

void rc_key_init(void)//函数保证上电后能进行按键扫描
{
	is_rckey_active =35;
	rc_matrix_key_init();
}

void rc_key_matrix_sleep(void)
{
	for(u8 i=0; i< sizeof(rckey_row); i++)
	{

		gpio_hw_set_direction(IO_PORT_SPILT(rckey_row[i]), 1);

		gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_row[i]), 1);
		gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_row[i]), 0);
		gpio_hw_set_die(IO_PORT_SPILT(rckey_row[i]), 1);
		gpio_hw_set_dieh(IO_PORT_SPILT(rckey_row[i]), 1);
		gpio_set_mode(IO_PORT_SPILT(rckey_row[i]), PORT_INPUT_PULLUP_100K);
		p33_io_wakeup_enable(rckey_row[i],1);
	}
	for(u8 i=0; i< sizeof(rckey_col); i++)
	{
		gpio_hw_direction_output(IO_PORT_SPILT(rckey_col[i]),0);
		gpio_hw_set_pull_up(IO_PORT_SPILT(rckey_col[i]), 0);
		gpio_hw_set_pull_down(IO_PORT_SPILT(rckey_col[i]), 0);
	}
}
u8 rckey_idle_query(void)
{
    return !is_rckey_active;
}
REGISTER_LP_TARGET(rckey_lp_target) = {
    .name = "rckey",
    .is_idle = rckey_idle_query,
};