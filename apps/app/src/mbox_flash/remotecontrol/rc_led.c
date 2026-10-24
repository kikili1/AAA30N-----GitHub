#include "typedef.h"
#include "rc_key.h"
#include "rc_led.h"
#include "msg.h"
#include "app_config.h"
#include "rc_app.h"
#include "config.h"
#include "gpio.h"
#include "bt_ble.h"
#include "rc_ir.h"
#include "sys_timer.h"
#include "power_api.h"
#include "vble_complete.h"
#include "hw_timer.h"
#include "audio2rf_send.h"
#include "audio_adc_api.h"
#include "encoder_stream.h"
#include "encoder_mge.h"
#include "clock.h"
#include "rc_gyro.h"
#include "app_reg_include.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[rc_led]"
#include "log.h"

_bool_byte	_led_bool_bit;

static u8 is_rcled_active=0;

bt_pair_led_enum 	bt_led_mode=BT_PAIR_LED_DISCON;


#define SYS_SOFTOFF_DISCON_TIME_BLE		4*1000//10s 两个定向回连周期
#define SYS_SOFTOFF_PAIR_TIME_BLE		60*1000//2*60*1000
#define SYS_SOFTOFF_CONN_TIME_BLE		1*60*60*1000//whole day
static u16 int_sys_off_timer=0;

#define DISCON_LED_FLASH			0
#define SYS_DISCON_LED_FLASH_TIME		7//80MS
#define SYS_PAIR_LED_FLASH_TIME			3//80MS
static u16  int_pair_bias_timer=0;
static u16  int_pair_led_timer=0;
static u16  int_blt_15s_timer =0;

int int_gy_sleep_timer = 0;
#if TCFG_GYRO_ENABLE
#define SYS_GY_SLEEP_TIME			30*1000//MIN
void sys_gyro_open_sleep_handle(void);
#endif

//sys sleep pmu

void sys_write_sleep(void)
{
	u8 array[2],buf;

	buf = 0;
	buf |= 2;	//陀螺仪上电校准标志
	if(!fm_open)
		buf |= 1;
	if(fkey_blt_rem || fkey_blt)
		buf |= 4;
	if(fgy_initi)
		buf |= 8;

	array[0] = buf;
	array[1] = RGB_CT;
	syscfg_write(CFG_USER_KEY_FLAG,array,sizeof(array));
}
void sys_read_vm_powerup(void)
{
	u8 array[2];

#if 0//TCFG_GYRO_ENABLE
	fgyro_calib = 1;//陀螺仪上电校准标志
#endif
	if(fsys_powup)//唤醒
	{
		log_info("sys_wake");
		fsys_powup =0;
		if(syscfg_read(CFG_USER_KEY_FLAG,array,sizeof(array))==sizeof(array))
		{
			log_info("power_up_read=%x ", array[0]);
			if(array[0]&1)
				fm_open =0;
			else fm_open=1;
			if(array[0]&2)
				fgyro_calib = 0;
			if(array[0]&4)
				fkey_blt = 1;
			else fkey_blt = 0;
			if(array[0]&8)				
				fgy_initi =1;			 
			else fgy_initi =0;
		}
	}
	else{//上电
		log_info("sys_powup");
		fm_open = 0;
		ftmode_up = 1;//上电产测标志
#if TCFG_GYRO_ENABLE
		fgy_initi = 1;
		//  fm_open = 1; //pow up  gy need initial
#endif
		if(syscfg_read(CFG_USER_KEY_FLAG,array,sizeof(array))==sizeof(array))
		{
			if(array[0]&2)
				fgyro_calib = 0;
		}
	}
	system_ir_vm_initial();
}

void sys_softoff_handle(void)
{
	int_sys_off_timer =0;
#if TCFG_GYRO_ENABLE
	system_gyro_power_mgmt(0);
#endif
	sys_softoff();
}
static void sys_power_off_time(void)
{
	u32 timer=0;
	switch(bt_led_mode)
	{
		case BT_PAIR_LED_CONNECT:
			timer = SYS_SOFTOFF_CONN_TIME_BLE;
			break;
		case BT_PAIR_LED_DISCON:
			timer = SYS_SOFTOFF_DISCON_TIME_BLE;
			break;
		case BT_PAIR_LED_PAIRING:
			timer = SYS_SOFTOFF_PAIR_TIME_BLE;
			break;

		default:
			break;
	}

	if(fir_study)
	{
		if(timer<20*1000)
			timer =20*1000;
	}
	if(int_sys_off_timer) 
		sys_timer_modify(int_sys_off_timer,timer);
	else int_sys_off_timer =sys_timeout_add(NULL,sys_softoff_handle,timer);
}

static u16 int_gy_key_pause_timer =0;
static void gy_key_pause_handle(void)
{
	int_gy_key_pause_timer =0;
	fgy_key_pause =0;
}
static void sys_blt_off_handle(void)
{
	int_blt_15s_timer =0;
	fkey_blt =0;
	fkey_blt_rem =1;
}

void sys_bias_led_handle(void)
{
	fsys_100mstick =1;
}

void sys_led_tick(void)
{
	static u16 int_led_tick=0;

	if(is_rcled_active)
		is_rcled_active--;
	if(fsys_100mstick)
	{
		fsys_100mstick =0;
		int_led_tick++;
	}
	if(int_pair_led_timer==0xffff)
	{
		fled_pair =1;
		if(fir_send )
			fir_led_ir = 1;
	}
	else if(int_pair_led_timer)
	{
		if(int_led_tick>=int_pair_led_timer)
		{
			int_led_tick =0;
			switch(int_pair_led_timer)
			{
				case 1://dis
					if(fir_send&&!firk_learn)
						fir_led_ir = ~fir_led_ir;
					if(firk_clrr)
						fir_led_ir = 1;
						break;
				case 3://pair
				case 10://fcc
					fled_pair =~fled_pair;
					fir_led = 0;
					break;

				default:
					break;
			}
		}
	}
	else {//int_pair_led_timer==0;
		fled_pair =0;
	}
	if(bt_led_mode == BT_PAIR_LED_PAIRING)
		fir_led = 0;
}


void sys_power_led_pmu(void)
{
	static bt_pair_led_enum old_status=0xff;

	if(old_status^bt_led_mode)//ble状态更新，需要更新POWEROFF计时
	{
		old_status =bt_led_mode;
		log_info("bt_led_mode =%d\n",bt_led_mode);

		sys_power_off_time();
	}

	if(fmic_vld)
	{
		if (BT_PAIR_LED_CONNECT != bt_led_mode) {
			audio_mic_switch(0);
			sys_power_off_time();
		}
	}
	
	if(fm_doing)//光标移动，清陀螺自动休眠计数和poweroff计数
	{
		#if TCFG_GYRO_ENABLE
		sys_gyro_open_sleep_handle();
		#endif 
		sys_power_off_time();
	}
	
	if(fkey_doing)//按键有效，清陀螺自动休眠计数和poweroff计数,并且如果陀螺休眠要打开，同时控制指示灯常亮或者闪烁状态(如果掉线闪灯则不指示)
	{
		switch(bt_led_mode)
		{
			case BT_PAIR_LED_CONNECT:
				int_pair_led_timer = 0xffff;//连接状态按键指示灯常亮
				break;
			case BT_PAIR_LED_DISCON:
				int_pair_led_timer = 1;//掉线状态指示灯闪烁
				break;
			case BT_PAIR_LED_PAIRING:
				int_pair_led_timer = SYS_PAIR_LED_FLASH_TIME;
				break;

			default:
				break;
		}
		#if TCFG_RGB_ENABLE
		if(fkey_blt)
		{	
			is_rcled_active =30;
			if(int_blt_15s_timer)
				sys_timer_modify(int_blt_15s_timer,15*1000);
		}
		#endif
		#if TCFG_GYRO_ENABLE
		if(fgy_key_pause)
		{
			if(!int_gy_key_pause_timer)
				int_gy_key_pause_timer =sys_timeout_add(NULL,gy_key_pause_handle,300);
		}
		system_gyro_power_mgmt(1);//key wakeup gy
		sys_gyro_open_sleep_handle();
		#endif		

		sys_power_off_time();
		
	}
	else {
		switch(bt_led_mode)
		{
			case BT_PAIR_LED_CONNECT:
				int_pair_led_timer = 0;//连接状态按键指示灯常灭
				break;
			case BT_PAIR_LED_DISCON:
				#if DISCON_LED_FLASH
				int_pair_led_timer = SYS_DISCON_LED_FLASH_TIME;//掉线状态指示灯闪
				#else 
				int_pair_led_timer = 0;//掉线状态指示灯灭
				#endif
				break;
			case BT_PAIR_LED_PAIRING:
				int_pair_led_timer = SYS_PAIR_LED_FLASH_TIME;
				break;

			default:
				break;
		}
	}
	if((bt_led_mode==BT_PAIR_LED_CONNECT) && !fir_led)//连接状态且无红外指示需求，消掉LED时基，idle频繁唤醒增加功耗，若有其他指示需求此处需增加标志
	{
		if(int_pair_bias_timer)
			sys_timer_del(int_pair_bias_timer);
		int_pair_bias_timer =0;
	}
	if(bt_led_mode || fir_led)
	{
		if(!int_pair_bias_timer)
			int_pair_bias_timer =sys_timer_add(NULL,sys_bias_led_handle,80);
	}
	if(fir_led || fhogp_open)
		is_rcled_active =30;

	if(fir_star)
		ble_set_fix_pwr(0);
	else if(fpwr_debug_pre)
	{
		fpwr_debug_pre = 0;
		ble_set_fix_pwr(7);
	}

	#if TCFG_GYRO_ENABLE
	if(fgy_key_pause)
		is_rcled_active =30;
	#endif

	sys_led_tick();

	cover_led_high_low();
}

#if TCFG_GYRO_ENABLE
void sys_gyro_open_sleep_handle(void)//更新gyro休眠时间
{
		if(int_gy_sleep_timer)
			sys_timer_modify(int_gy_sleep_timer,SYS_GY_SLEEP_TIME);
		else int_gy_sleep_timer =sys_timeout_add((void *)0, system_gyro_power_mgmt, SYS_GY_SLEEP_TIME);//gy sleep 30s
}
#endif

static u16 int_powerwarn_timer=0;
void power_warn_led_handle(void)
{
	static u8 power_warn_led_ct=0;

	fled_pow =~fled_pow;
	// if(++power_warn_led_ct>=6)
	// {
	// 	power_warn_led_ct =0;
	// 	flow_pow =0;
	// 	if(int_powerwarn_timer)
	// 	{
	// 		sys_timer_del(int_powerwarn_timer);
	// 		int_powerwarn_timer = 0;
	// 	}
	// }
	cover_led_high_low();
}
int app_power_led_handler(void)
{
	//log_info("----%s", __FUNCTION__);
	if(flow_pow)
	{
		if(!int_powerwarn_timer)
			int_powerwarn_timer = sys_timer_add(NULL,power_warn_led_handle,500);
	}
	return 0;
}


//+============================================================
u8 Connect_led_set(u8 en)
{
	gpio_hw_direction_output(IO_PORT_SPILT(CONNECT_LED_PIN),en);
	return en;
}
void ir_led_set(u8 en)
{
	gpio_hw_direction_output(IO_PORT_SPILT(IR_LED_PIN),en);
}
#if TCFG_RGB_ENABLE
static void r_led_set(u8 en)
{
	gpio_hw_direction_output(IO_PORT_SPILT(R_LED_PIN),en);
}
static void g_led_set(u8 en)
{
	gpio_hw_direction_output(IO_PORT_SPILT(G_LED_PIN),en);
}
static void b_led_set(u8 en)
{
    gpio_hw_direction_output(IO_PORT_SPILT(B_LED_PIN),en);
}
void MOT_led_set(u8 en)
{
	#if (TCFG_UART0_ENABLE==0)
	gpio_hw_direction_output(IO_PORT_SPILT(LED_MOT),!en);
	#endif
}
void blt_led_set(void)
{
	if(fkey_blt)
	{	
		fled_b =0;
		fled_g =0;
		fled_r =0;
		switch(RGB_CT)
		{
			case 1:
				fled_r =1;
				break;
			case 2:
				fled_r =1;
				fled_g =1;
				fled_b =1;
				break;
			case 3:
				fled_r =1;
				fled_g =1;
				break;
			case 4:
				fled_g =1;
				break;
			case 5:
				fled_g =1;
				fled_b =1;
				break;
			case 6:
				fled_b =1;
				break;
			case 7:
				fled_b =1;
				fled_r =1;
				break;
			default:
				break;
		}
		MOT_led_set(0);
		is_rcled_active =30;
		if(!int_blt_15s_timer)
			int_blt_15s_timer =sys_timeout_add(NULL,sys_blt_off_handle,15*1000);
	}
	else {
		fled_b =0;
		fled_g =0;
		fled_r =0;
		MOT_led_set(1);
		if(int_blt_15s_timer)
			sys_timeout_del(int_blt_15s_timer);
		int_blt_15s_timer =0;
	}
	// if(fkey_blt)
	// {
	// 	fled_b =1;
	// 	fled_g =1;
	// 	fled_r =1;
	// }
	// else {
	// 	fled_b =0;
	// 	fled_g =0;
	// 	fled_r =0;
	// }
	if(fled_r)
		r_led_set(1);
	else r_led_set(0);
	if(fled_g)
		g_led_set(1);
	else g_led_set(0);
	if(fled_b)
		b_led_set(1);
	else b_led_set(0);
}
#endif
void cover_led_high_low(void)
{
	u8 ret;

	ret = 0;

	// if(fsys_enterSniff)
	// {
	// 	// rc_key_matrix_sleep();
	// 	return;
	// }
	// gpio_hw_direction_output(IO_PORT_SPILT(LED_MOT),1);

	if(fir_led)
	{
		if(fir_led_ir)
			ir_led_set(1);
		// else ir_led_set(0);
		Connect_led_set(0);
	//  else Connect_led_set(0);
	}
	else {
		if(flow_pow)
		{
			if(fled_pow)
				ret = Connect_led_set(1);
		//  else Connect_led_set(0);
		}
		else {
			if(fled_pair)
				ret = Connect_led_set(1);
			// else Connect_led_set(0);
		}
		ir_led_set(0);
	}
	if(fgyro_calib)
		ret = Connect_led_set(1);
#if TCFG_RGB_ENABLE
	blt_led_set();
#endif
	if(ret || fir_led)
		gpio_hw_direction_output(IO_PORT_SPILT(LED_MOT),0);
}


u8 rcled_idle_query(void)
{
    return !is_rcled_active;
}
REGISTER_LP_TARGET(rcled_lp_target) = {
    .name = "rcled",
    .is_idle = rcled_idle_query,
};

