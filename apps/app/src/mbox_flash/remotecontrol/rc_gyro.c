/*********************************************************************************************
    *   Filename        : app_gyro.c

    *   Description     :

    *   Author          :

    *   Email           :

    *   Last modifiled  : 2019-07-05 10:09

    *   Copyright:(c)XingWei  2011-2019  @ , All Rights Reserved.


*********************************************************************************************/
#include "typedef.h"
#include "msg.h"
#include "app_config.h"
#include "rc_app.h"
#include "config.h"
#include "gpio.h"
#include "vble_complete.h"
#include "bt_ble.h"

#include "gptimer.h"
#include "sys_timer.h"
#include "hw_timer.h"
#include "rc_key.h"
#include "app_reg_include.h"
#include "rc_gyro.h"
#include "iic_soft.h"
#include "rc_led.h"
#if(TCFG_GYRO_ENABLE)
#include "app_gyro_lib.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[RC_GYRO]"
#include "log.h"

extern void delay(unsigned int cnt);
static u8 gyro_write(u8 *buf, u8 addr, u8 reg, u8 len);
static u8 gyro_read(u8 *buf, u8 addr, u8 reg,u8 len);

static volatile u8 is_gyro_active = 0;

_bool_int	_gy_bool_bit;

s16 gyro_FIFO[6];
static s16 GYRO_OFFSET[3];
u8 dpi_data;
u16 gyro_initi_timer = 0,gy_scan_timer = 0;
u8 iic_Waddr,id_channel,gy_id;
s16	anglex,pre_accx,pre_accz,anglex_kal;
static u16 min_interval_pre = 0;
u16 min_interval = 10;
static u8 no_gy_time = 0;
static u8 gy_pwg = 0xff;
static u8 event_gesture = 0;



static u16 const Gyro_Initial_Tab_S0[] = {
    0x2227,0x1e38,0x1f38,0x2071,0x1069,0x1240,0xffff};

static u16 const Gyro_Initial_Tab_S2[] = {
    0x1227,0x1938,0x1838,0x1158,0x1059,0xffff};

// static u16 const Gyro_Initial_Tab_2628[] = {
//     0x7f00,0x0680,0x0609,0x0570,0x7f20,0x0004,0x0115,0x1413,0x7f00,0xffff};
static u16 const Gyro_Initial_Tab_2628[] = {
    0x7f00,0x0680,0x0609,0x7f20,0x0004,0x0115,0x1413,0x7f00,0xffff};
static u16 const Gyro_Initial_Tab_6881[] = {
    0x6b09,0x6c00,0x1c08,0x1b10,0x1a05,0x1909,0xffff};

static u16 const Gyro_addr_reg_who_am_i[] =
{0xd075,0xd275,0xd000,0xd200,0X3C0F,0xd40f,0xd00f,0xd60f,0xffff};

//General array for all modules
static u16 const Acc_Initial_Tab_330[] = {
        0x2301,0x2077,0x2448,0x2510,0xffff
	//	reset	400hz	bw_200hz 4g
									};
//General array for all modules
static u16 const Gyro_Initial_Tab_330[] = {//.b  agd8 agd2 330
        0x20ff,0x2120,0x2310,0x2410,0xffff};

#define LSM330_ACC_ADDR_L  0X3C
#define LSM330_GYRO_ADDR_L  0XD4

#define GY_3CHANNEL_6881	0

int system_gyro_initi_matrix(void)
{
	log_info("system_gyro_initi_matrix fgy_initi = %d",fgy_initi);

	is_gyro_active = 50;
    if(fgy_initi)
	{
		if(gyro_initi_timer)
			sys_timer_del(gyro_initi_timer);
		gyro_initi_timer = sys_timer_add(NULL, system_gyro_initial, 200);
	}
	else {
		fgy_inside = 1;
		soft_iic_init();
		system_gyro_power_save_read();
		system_gyro_power_mgmt(1);
	}
	return 0;
}
void system_gyro_initial(void)
{
	u8	array[12],id_reg,id_channel_pre;
	u16	buf,*Gyro_initi_tab;

	log_info("fir_study = %d,fgy_initi = %d",fir_study,fgy_initi);
	if(fir_study || !fgy_initi)
		return;
	gy_pwg =0;
	// is_gyro_active = 50;
	soft_iic_init();
	id_channel =0;
	log_info("system_gyro_initial");

	syscfg_read(CFG_USER_GY_KAL_INDEX, array, 12);
	if(array[0])
		array[0] =0;
	syscfg_write(CFG_USER_GY_KAL_INDEX, array, 12);

	do{
	 	iic_Waddr =Gyro_addr_reg_who_am_i[id_channel]>>8;
		id_reg =Gyro_addr_reg_who_am_i[id_channel]&0x00ff;
		if(iic_Waddr==0xff)
		{
			if(++no_gy_time>=5)
			{
				no_gy_time =0;
				if(gyro_initi_timer)
					sys_timer_del(gyro_initi_timer);
				gyro_initi_timer =0;
				is_gyro_active = 0;
			}
			return;//err
		}
		if(!gyro_read(array,iic_Waddr,id_reg,1))
		{
			gy_id =array[0];
			log_info("gy_id=%x",gy_id);
			id_channel_pre =id_channel;
			switch(id_channel)
			{
				case 0:
				case 1:
					if((gy_id==0x11)||(gy_id==0x95)|| (gy_id==0x78) || (gy_id==0x70) || (gy_id==0x79) || (gy_id==0x74) || (gy_id==0xae) || (gy_id==0xaf)||(gy_id==0x91)||(gy_id==0x90))
						Gyro_initi_tab =Gyro_Initial_Tab_6881;
					else id_channel++;//err
					break;
				case 2:
				case 3:
					if((gy_id==0x11)|| (gy_id==0xa2) || (gy_id==0xa8) || (gy_id==0xa7))
						Gyro_initi_tab =Gyro_Initial_Tab_2628;
					else id_channel++;//err
					break;
				case 4:
					if(gy_id==0x40)
					{
						fgyro_330 =1;
						Gyro_initi_tab =Acc_Initial_Tab_330;
					}
					else id_channel++;//err
					break;
				case 5:
					if((gy_id==0x69)|| (gy_id==0x6a) || (gy_id==0x6c) || (gy_id==0x6d))
						Gyro_initi_tab =Gyro_Initial_Tab_S2;
					else if(gy_id==0x68)
						Gyro_initi_tab =Gyro_Initial_Tab_S0;
					else if(gy_id==0xd4)
					{
						if(!gyro_read(array,LSM330_ACC_ADDR_L,0xf,1))
						{
							if(array[0]==0x40)
							{
								Gyro_initi_tab =Acc_Initial_Tab_330;
								id_channel =4;
								gy_id=0x40;
								fgyro_330 =1;
							}
							else fgyros_3chanel =1;
						}
						else fgyros_3chanel =1;
					}
					else if(gy_id==0xd5 || gy_id==0xd7)
						fgyros_3chanel =1;
					else id_channel++;//err
					break;
				case 6:
				case 7:
					if(gy_id==0xd3 || gy_id==0xd5 || gy_id==0xd7)
						fgyros_3chanel =1;
					else id_channel++;//err
					break;
				default:
					id_channel++;
					break;
			}
			if(id_channel == id_channel_pre)
			 	break;
		}
		else id_channel++;//err
	}while(1);
	log_info("id_channel=%x",id_channel);
	if(fgyro_330)//
	{
		do{
			buf = *Gyro_initi_tab;
			array[0] =buf&0x00ff;
			id_reg =buf>>8;
			if(id_reg==0xff)
				break;
			if(!gyro_write(array,iic_Waddr,id_reg,1))
				Gyro_initi_tab++;
			else return;//err
		}while(1);
		iic_Waddr =LSM330_GYRO_ADDR_L;
		gy_id =0xd4;
		Gyro_initi_tab =Gyro_Initial_Tab_330;
	}
	if(fgyros_3chanel)
		Gyro_initi_tab =Gyro_Initial_Tab_330;
	 ///////////////////////////////////////////////
	do
	{
		delay(50000);
		delay(50000);
		buf = *Gyro_initi_tab;
		array[0] =buf&0x00ff;
		id_reg =buf>>8;
		if(id_reg==0xff)
			break;
		if(!gyro_write(array,iic_Waddr,id_reg,1))
			Gyro_initi_tab++;
		else {
			log_info("iic_erro ,iic_Waddr:%x\n",iic_Waddr);
			return;//err
		}
	}while(1);
	fgy_inside = 1;
	system_gyro_power_mgmt(0);
	if(gyro_initi_timer)
		sys_timer_del(gyro_initi_timer);
	gyro_initi_timer =0;
	system_gyro_power_mgmt(1);

	system_gyro_power_save_read();
	sys_gyro_open_sleep_handle();
	fgy_initi =0;
	log_info("gyro is initi succ!\n");
}

void system_gyro_power_save_read(void)
{
	u8 array[15];
	u8 buf;

	buf = syscfg_read(CFG_USER_GY_OFFSET_INDEX, array, 7);

	if(array[0]==0x38)
	{
		GYRO_OFFSET[0] =array[1]<<8 |array[2];
		GYRO_OFFSET[1] =array[3]<<8 |array[4];
		GYRO_OFFSET[2] =array[5]<<8 |array[6];
	}
	else
	{
		GYRO_OFFSET[0]=0;
		GYRO_OFFSET[1]=0;
		GYRO_OFFSET[2]=0;
	}

	buf = syscfg_read(CFG_USER_dpi_INDEX, array, 2);
	if(array[0]==0x39)
	  dpi_data =array[1];
	else dpi_data =1;

	syscfg_read(CFG_USER_GY_KAL_INDEX, array, 12);
	if(array[0]==0x3A)
	 {
		iic_Waddr=array[1];
		id_channel=array[2];
		if(id_channel==4)
			fgyro_330 =1;
		gy_id=array[3];
		if(gy_id==0xd3 || gy_id==0xd4|| gy_id==0xd5 || gy_id==0xd7)
		{
			if(!fgyro_330)
				fgyros_3chanel =1;
		}

		anglex =array[4]&0x00ff;
		anglex |=array[5]<<8;

		pre_accx =array[6]&0x00ff;
		pre_accx |=array[7]<<8;

		pre_accz =array[8]&&0x00ff;
		pre_accz |=array[9]<<8;

		anglex_kal =array[10]&0x00ff;
		anglex_kal |=array[11]<<8;
	}
	else
	{
		anglex=0x4000;
		pre_accx =0;
		pre_accz =0x2000;
		anglex_kal=0x4000;
	}
}

void system_gyro_power_mgmt(u8 mode)
{
	u8 reg_dat[12],id_reg;

	if(gy_pwg==mode || !fgy_inside)
	{
		//printf("system_gyro_power_mgmt same\n");
		return;
	}
	if(int_gy_sleep_timer)
		sys_timer_del(int_gy_sleep_timer);
	int_gy_sleep_timer =0;
	log_info("system_gyro_power_mgmt %d",mode);
	gy_pwg =mode;

	reg_dat[0] =0x3A;

	reg_dat[1] =iic_Waddr;
	reg_dat[2] =id_channel;
	reg_dat[3] =gy_id;

	reg_dat[4] =anglex&0x00ff;
	reg_dat[5] =anglex>>8;

	reg_dat[6] =pre_accx&0x00ff;
	reg_dat[7] =pre_accx>>8;

	reg_dat[8] =pre_accz&0x00ff;
	reg_dat[9] =pre_accz>>8;

	reg_dat[10] =anglex_kal&0x00ff;
	reg_dat[11] =anglex_kal>>8;
	if(fgy_initi || !fmic_vld)
		syscfg_write(CFG_USER_GY_KAL_INDEX, reg_dat, 12);

	switch(id_channel)
	{
		case 0://6881
		case 1:
			if(mode)
				reg_dat[0] =0x09;
			else reg_dat[0] =0x49;
			id_reg =0x6b;
			gyro_write(reg_dat,iic_Waddr,id_reg,1);
			break;
		case 2:
		case 3:
			if(mode)
				reg_dat[0] =0x09;
			else reg_dat[0] =0x49;
			id_reg =0x06;
			gyro_write(reg_dat,iic_Waddr,id_reg,1);
			break;
		case 4:
			if(mode)
				reg_dat[0] =0x77;
			else reg_dat[0] =0x0;
			id_reg =0x20;
			iic_Waddr =LSM330_ACC_ADDR_L;
			gyro_write(reg_dat,iic_Waddr,id_reg,1);
			if(mode)
				reg_dat[0] =0xdf;
			else reg_dat[0] =0x0;
			id_reg =0x20;
			iic_Waddr =LSM330_GYRO_ADDR_L;
			gyro_write(reg_dat,iic_Waddr,id_reg,1);
			break;
		case 5:
			if(gy_id==0xd4 || gy_id==0xd5 || gy_id==0xd7)
			{
				if(mode)
				reg_dat[0] =0xff;
				else reg_dat[0] =0x0;
				id_reg =0x20;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);
			}
			else if(gy_id^0x68)
			{
				if(mode)
					reg_dat[0] =0x59;
				else reg_dat[0] =0;
				id_reg =0x10;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);

				if(mode)
					reg_dat[0] =0x58;
				else reg_dat[0] =0;
				id_reg =0x11;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);

				if(mode)
					reg_dat[0] =0;
				else reg_dat[0] =0x40;
				id_reg =0x13;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);
			}
			else {
				if(mode)
					reg_dat[0] =0x71;
				else reg_dat[0] =0;
				id_reg =0x20;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);

				if(mode)
					reg_dat[0] =0x69;
				else reg_dat[0] =0;
				id_reg =0x10;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);

				if(mode)
					reg_dat[0] =0;
				else reg_dat[0] =0x40;
				id_reg =0x23;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);
			}
			break;
		case 6:
		case 7:
			if(gy_id==0xd3 || gy_id==0xd7 || gy_id==0xd5)
			{
				if(mode)
					reg_dat[0] =0xff;
				else reg_dat[0] =0x0;
				id_reg =0x20;
				gyro_write(reg_dat,iic_Waddr,id_reg,1);
			}
			break;
		default:
			printf("gyro id_channel err  %d",id_channel);
			break;
	}

	if(gy_scan_timer)
	{
	  sys_timer_del(gy_scan_timer);
	  gy_scan_timer =0;
	}
	if(mode)
	{
		min_interval_pre =0xff;
		gy_scan_timer =sys_timer_add(NULL,system_gyro_check,10);
	}
	else is_gyro_active =0;
}

void gyro_acc_move_check(s16 *FIFO)
{
	static s16 acc_pre[3];
	u32 total;
	u8 array[2];
	u8 i;

	if(fgy_lowp)
	{
		total =0;
		for(i=0;i<3;i++)
		total +=abs(FIFO[i+3]-acc_pre[i]);
		if(total>=0x200)
		{
			fgy_lowp =0;
			min_interval_pre =0xff;
			sys_timer_modify(gy_scan_timer,10);
			if(fgyro_330)
			{
				array[0] =0xff;
				gyro_write(array,LSM330_GYRO_ADDR_L,0x20,1);
			}
		}
	}
	for(i=0;i<3;i++)
		acc_pre[i] =FIFO[i+3];
}


void system_gyro_check(void)
{
	static  u8 int_iic_err_ct =0;
	static	s8 inc_mv[2];
	static  u8 int_m_doing_ct;
	static s16  pre_gy_buf[3];
	u8	array[15],i,GY_X,GY_Y;
	s16 reg_dat[4];
	u8 row,cur_key_value = 0;

	if(is_gyro_active)
		is_gyro_active--;
	fm_doing = 0;
	if(fgy_initi || !gy_pwg || fir_study)
		return;
	if(min_interval^min_interval_pre)
	{
		min_interval_pre = min_interval*125;
		min_interval_pre = min_interval_pre/100;
		min_interval_pre++;
		if(min_interval_pre<10)
			min_interval_pre =10;
		sys_timer_modify(gy_scan_timer,min_interval_pre);
		min_interval_pre =min_interval;
	}
	event_gesture=0;

	if(id_channel==0 || id_channel==1)//6881
	{
		if(!gyro_read(array,iic_Waddr,0x3a,1))
		{
			if(array[0]&0x1)
			{
				if(!gyro_read(array,iic_Waddr,0x3b,14))
				{
					for(i=0;i<3;i++)
					{
					gyro_FIFO[i+3] =array[2*i+1];
					gyro_FIFO[i+3] |=array[2*i]<<8;
					gyro_FIFO[i] =array[2*i+1+8];
					gyro_FIFO[i] |=array[2*i+8]<<8;
					}
					#ifdef GY_3CHANNEL_6881
					if(GY_3CHANNEL_6881)
					{
						fgyros_3chanel =1;
						gyro_FIFO[1] =-gyro_FIFO[2];
						gyro_FIFO[2] =0;
						gyro_FIFO[0] =-gyro_FIFO[0];
					}
					#endif
				}
			}
			else return ;
		}
		else fgy_initi =1;
	}
	else if(id_channel==2 || id_channel==3)//2628
	{
		if(!gyro_read(array,iic_Waddr,0x1a,1))
		{
			if(array[0]&0x1)
			{
				if(!gyro_read(array,iic_Waddr,0x2d,12))
				{
					for(i=0;i<3;i++)
					{
					gyro_FIFO[i+3] =array[2*i+1];
					gyro_FIFO[i+3] |=array[2*i]<<8;
					gyro_FIFO[i] =array[2*i+1+6];
					gyro_FIFO[i] |=array[2*i+6]<<8;
					}

					pre_gy_buf[0] +=gyro_FIFO[0];
					pre_gy_buf[0] >>=1;
					gyro_FIFO[0] =pre_gy_buf[0];

					pre_gy_buf[1] +=gyro_FIFO[1];
					pre_gy_buf[1] >>=1;
					gyro_FIFO[1] =pre_gy_buf[1];

					pre_gy_buf[2] +=gyro_FIFO[2];
					pre_gy_buf[2] >>=1;
					gyro_FIFO[2] =pre_gy_buf[2];
				}
			}
		}
		else fgy_initi =1;
	}
	else if(id_channel==4)//330
	{
		if(!fgy_lowp)
		{
			if(!gyro_read(array,LSM330_GYRO_ADDR_L,0x27,1))
			{
				if(array[0]&0x8)
				{
					if(!gyro_read(array,LSM330_GYRO_ADDR_L,0x28|0X80,6))
					{
						if(!gyro_read(&array[6],LSM330_ACC_ADDR_L,0x28|0X80,6))
						{
						gyro_FIFO[0] =array[2]&0x00ff;
						gyro_FIFO[0] |=array[3]<<8;
						gyro_FIFO[1] =array[0]&0x00ff;
						gyro_FIFO[1] |=array[1]<<8;
						gyro_FIFO[2] =array[4]&0x00ff;
						gyro_FIFO[2] |=array[5]<<8;

						gyro_FIFO[3] =array[8]&0x00ff;
						gyro_FIFO[3] |=array[9]<<8;
						gyro_FIFO[4] =array[6]&0x00ff;
						gyro_FIFO[4] |=array[7]<<8;
						gyro_FIFO[5] =array[10]&0x00ff;
						gyro_FIFO[5] |=array[11]<<8;

						gyro_FIFO[1] =-gyro_FIFO[1];
						gyro_FIFO[4] =-gyro_FIFO[4];
						gyro_FIFO[0] >>=1;
						gyro_FIFO[1] >>=1;
						gyro_FIFO[2] >>=1;
						}
					}
				}
			}
			else fgy_initi =1;
		}
		else
		{
			if(!gyro_read(&array[6],LSM330_ACC_ADDR_L,0x28|0X80,6))
			{
				gyro_FIFO[0] =0;
				gyro_FIFO[1] =0;
				gyro_FIFO[2] =0;

				gyro_FIFO[3] =array[8]&0x00ff;
				gyro_FIFO[3] |=array[9]<<8;
				gyro_FIFO[4] =array[6]&0x00ff;
				gyro_FIFO[4] |=array[7]<<8;
				gyro_FIFO[5] =array[10]&0x00ff;
				gyro_FIFO[5] |=array[11]<<8;

				gyro_FIFO[4] =-gyro_FIFO[4];
			}
			else fgy_initi =1;
		}
	}
	else if(id_channel==5)//st
	{
		if((gy_id==0xd4) || (gy_id==0xd5) || (gy_id==0xd7))//.B
		{
			if(!gyro_read(array,iic_Waddr,0x27,1))
			{
				if(array[0]&0x8)
				{
					if(!gyro_read(array,iic_Waddr,0x28|0X80,6))
					{
					gyro_FIFO[0] =array[2]&0x00ff;
					gyro_FIFO[0] |=array[3]<<8;
					gyro_FIFO[1] =array[0]&0x00ff;
					gyro_FIFO[1] |=array[1]<<8;
					gyro_FIFO[2] =array[4]&0x00ff;
					gyro_FIFO[2] |=array[5]<<8;
					}
				}
			}
			else fgy_initi =1;
		}
		else if(gy_id^0x68)
		{
			if(!gyro_read(array,iic_Waddr,0x1e,1))//s2
			{
				if(array[0]&0x2)
				{
					if(!gyro_read(array,iic_Waddr,0x22,12))
					{
						for(i=0;i<6;i++)
						{
						gyro_FIFO[i] =array[2*i];
						gyro_FIFO[i] |=array[2*i+1]<<8;
						}
					}
				}
			}
			else fgy_initi =1;
		}
		else
		{
			if(!gyro_read(array,iic_Waddr,0x17,1))//s0
			{
				if(array[0]&0x2)
				{
					if(!gyro_read(array,iic_Waddr,0x18,6))
					{
						if(!gyro_read(&array[6],iic_Waddr,0x28,6))
						{
							for(i=0;i<3;i++)
							{
								gyro_FIFO[i] =array[2*i];
								gyro_FIFO[i] |=array[2*i+1]<<8;
							}
							for(i=0;i<3;i++)
							{
							gyro_FIFO[i+3] =array[2*i];
							gyro_FIFO[i+3] |=array[2*i+1]<<8;
							}
						}
					}
				}
			}
			else fgy_initi =1;
		}
	}
	else if(id_channel==6 || id_channel==7)//st
	{
		if(gy_id==0xd3 || gy_id==0xd5  || gy_id==0xd7)
		{
			if(!gyro_read(array,iic_Waddr,0x27,1))
			{
				if(array[0]&0x8)
				{
					if(!gyro_read(array,iic_Waddr,0x28|0X80,6))
					{
						gyro_FIFO[0] =array[2]&0x00ff;
						gyro_FIFO[0] |=array[3]<<8;
						gyro_FIFO[1] =array[0]&0x00ff;
						gyro_FIFO[1] |=array[1]<<8;
						gyro_FIFO[2] =array[4]&0x00ff;
						gyro_FIFO[2] |=array[5]<<8;
					}
				}
			}
			else fgy_initi =1;
		}
	}

	if(fgy_initi)
	{
		if(++int_iic_err_ct>=5)
		{
			int_iic_err_ct =0;
			system_gyro_initi_matrix();

		}
		else{
			soft_iic_init();
			fgy_initi =0;
		}
		return;
	}
	else int_iic_err_ct =0;

	if(fgy_lowp && !fgyros_3chanel)
	{
		gyro_acc_move_check(gyro_FIFO);
		return;
	}
	for (row = 0; row < sizeof(rckey_row); row ++)
		cur_key_value |= gpio_read(rckey_row[row])<<row;

	if((cur_key_value ^ krow_num) & krow_num)
	{
		if(!fkey_doing)
			return;
	}

	if(fgyros_3chanel)
	{
		#if GY_3CHANNEL_6881
			pre_gy_buf[0] =(pre_gy_buf[0]>>1) + (gyro_FIFO[0]>>1);
			gyro_FIFO[0] =pre_gy_buf[0];
			pre_gy_buf[1] =(pre_gy_buf[1]>>1) + (gyro_FIFO[1]>>1);
			gyro_FIFO[1] =pre_gy_buf[1];
		#else
		 gyro_FIFO[0]>>=1;
		 gyro_FIFO[1]>>=1;
		 gyro_FIFO[2]>>=1;

		 pre_gy_buf[0] =pre_gy_buf[0]*3;
		 pre_gy_buf[0] +=gyro_FIFO[0];
		 pre_gy_buf[0] >>=2;
		 gyro_FIFO[0] =pre_gy_buf[0];

		 pre_gy_buf[1] =pre_gy_buf[1]*3;
		 pre_gy_buf[1] +=gyro_FIFO[1];
		 pre_gy_buf[1] >>=2;
		 gyro_FIFO[1] =pre_gy_buf[1];

		 pre_gy_buf[2] =pre_gy_buf[2]*3;
		 pre_gy_buf[2] +=gyro_FIFO[2];
		 pre_gy_buf[2] >>=2;
		 gyro_FIFO[2] =pre_gy_buf[2];

		#endif
	}
	gyro_read_defult(gyro_FIFO);
	if(fgyros_3chanel)
	{
		gyro_FIFO[0] =-gyro_FIFO[1];
		gyro_FIFO[1] =-gyro_FIFO[2];
	}
	else {
		reg_dat[0] = anglex;
		reg_dat[1] = pre_accx;
		reg_dat[2] = pre_accz;
		reg_dat[3] = anglex_kal;
		Kalman_filter(gyro_FIFO,reg_dat);
	}
	anglex = reg_dat[0];
	pre_accx = reg_dat[1];
	pre_accz = reg_dat[2];
	anglex_kal = reg_dat[3];

	GY_X =tolerance_sub(gyro_FIFO[1],inc_mv);
	GY_Y =-tolerance_sub(gyro_FIFO[0],&inc_mv[1]);
	if(fgyro_330 || fgyros_3chanel)
	{
		GY_Y =-GY_Y;
	}

	if(fgy_key_pause || (fmic_vld && !fbt_donglePair))
	{
		GY_X = 0;
		GY_Y = 0;
	}
	if(!fm_open)
	{
		GY_X = 0;
		GY_Y = 0;
		fm_doing = 0;
	}

	if(GY_X || GY_Y)
	{
		is_gyro_active = 50;

		if(int_m_doing_ct<5)
			int_m_doing_ct++;
		else
		{
			if(!bt_led_mode)
				fm_doing = 1;
			int_m_doing_ct = 5;
			MOUSE_X = GY_X;
			MOUSE_Y = GY_Y;
			// log_info("GY_X:%x,GY_Y:%x\n",GY_X,GY_Y);
		}

	}
	else
	{
		int_m_doing_ct =0;
		fm_doing = 0;
	}
	if(gy_scan_timer)
		is_gyro_active = 50;
	else is_gyro_active = 0;

	if(fm_doing)
	{
		int ret = post_msg(1, MSG_IO_MOUSE);
		if (ret != 0) {
			log_info("error !!! msg pool full \n");
		}
	}
}

//-------------------------------------------------------
//Func  : У��
//Desc	:
//Input	:
//Output:
//Return:
//Author: Owen
//Date	: 2013/07/29
//-------------------------------------------------------
void gyro_read_defult(s16 *FIFO)
{
	static s16   gyro_defult[3];
	static u8   Len=0;
	int	buf;
	u8 array[8];
	u8 reg_dat[2];
	u16 j;

	u8 i=0;

	while(i<3)
	{
		if(fgyro_calib)
			j =0x100;
		else j =0x35;
		if(abs(FIFO[i]-gyro_defult[i])>j)
		{
			Len =0;
			gyro_defult[0] = FIFO[0];
			gyro_defult[1] = FIFO[1];
			gyro_defult[2] = FIFO[2];
			break; 								   //����ѭ��
		}
		i++;
	}
  	if(Len++>=60)
   	{
		Len =0;
		if(fgyro_calib)
			fgyro_calib =0;
		for(i=0;i<3;i++)
		{
			GYRO_OFFSET[i]=(gyro_defult[i]>>1) + (FIFO[i]>>1);
			FIFO[i] =0;
		}
		if(!gy_off_set&&!fmic_vld)
		{
			gy_off_set =1;
			sys_gyro_offset_save();
		}
		if(!fgy_lowp && !fgyros_3chanel)
		{
			fgy_lowp =1;
			sys_timer_modify(gy_scan_timer,100);//only acc
			if(fgyro_330)
			{
				reg_dat[0] =0xd8;
				gyro_write(reg_dat,LSM330_GYRO_ADDR_L,0x20,1);
			}

		}
	}
	else for(i=0; i<3; i++)
		FIFO[i] =FIFO[i]-GYRO_OFFSET[i];
}

void sys_gyro_offset_save(void)
{
   u8 array[12];

	array[0]=0x38;
	array[1]=GYRO_OFFSET[0]>>8;
	array[2]=GYRO_OFFSET[0]&0x00ff;
	array[3]=GYRO_OFFSET[1]>>8;
	array[4]=GYRO_OFFSET[1]&0x00ff;
	array[5]=GYRO_OFFSET[2]>>8;
	array[6]=GYRO_OFFSET[2]&0x00ff;

	syscfg_write(CFG_USER_GY_OFFSET_INDEX, array, 7);

	array[0] =0x3A;
	array[1] =iic_Waddr;
	array[2] =id_channel;
	array[3] =gy_id;

	array[4] =anglex&0x00ff;
	array[5] =anglex>>8;

	array[6] =pre_accx&0x00ff;
	array[7] =pre_accx>>8;

	array[8] =pre_accz&0x00ff;
	array[9] =pre_accz>>8;

	array[10] =anglex_kal&0x00ff;
	array[11] =anglex_kal>>8;
	syscfg_write(CFG_USER_GY_KAL_INDEX, array, 12);

	array[0] =0x39;
	array[1] =dpi_data;
	syscfg_write(CFG_USER_dpi_INDEX, array, 2);

	array[0] = 0x2;
	syscfg_write(CFG_USER_KEY_FLAG,array,1);
}

//-------------------------------------------------------
//Func	: my_abs
//Desc	: 8λ��ȡ����ֵ
//Input :
//Output:
//Return:
//Author: Owen
//Date	: 2013/06/13
//-------------------------------------------------------
char my_abs(u8 buf)
{
 if(buf &0x80)
	buf = -buf;
 return buf;
}

//-------------------------------------------------------
//Func  :����
//Desc	:
//Input	:
//Output:
//Return:
//Author: Owen
//Date	: 2017/09/2
//-------------------------------------------------------
u8 tolerance_sub(s16 dat,s8 *co)
{
  s16 m,h;
  u8 n;

    m =dat>>1;
	h =dat;
  if(dat&0x1)
  	m++;
  dat =dat<<1;
  dat =dat+m;

  if((dpi_data==1) || (dpi_data==0xff))
  	dat =dat+m;
  else if(dpi_data==2)
  	dat =dat+h;
  else if(dpi_data==3)
  	dat =dat+h+m;
  else if(!dpi_data)
  	dat =m + h;

    m = abs(dat);
    if(m>GY_TOL)
      {
       m =m-GY_TOL;
	   if(dat&0x8000)
	   	*co -=GY_TOL;
	   else *co +=GY_TOL;
      }
	else {
		 *co +=dat;
		 m =0;
		}
	n=my_abs(*co);
	if(n>=GY_TOL)
		{
		 m++;
		 n=n-GY_TOL;
		 if(*co&0x80)
		 	n = -n;
		 *co=n;
		}
	if(dat&0x8000)
		m =-m;
    n =m>>8;
	if(m&0x80)
		n++;
	n =-n;
	return n;
}


static u8 gyro_write(u8 *buf, u8 addr,u8 gy_reg, u8 len)
{
    int ret;

	soft_iic_start();
	ret = soft_iic_tx_byte(addr);
	if (!ret)
		{
		 soft_iic_stop();
		 return 0xfe;
		}
	soft_iic_tx_byte(gy_reg);
	if (!ret)
		{
		 soft_iic_stop();
		 return 0xfe;
		}
	ret = soft_iic_write_buf(buf, len);
	if (ret < len)
		{
		 soft_iic_stop();
		 return 0xfe;
		}
	soft_iic_stop();
	return 0;
}

static u8 gyro_read(u8 *buf, u8 addr,u8 gy_reg, u8 len)
{
    int ret;

	soft_iic_start();
	ret = soft_iic_tx_byte(addr);
	if (!ret)
		{
		 soft_iic_stop();
		 return 0xfe;
		}
	soft_iic_tx_byte(gy_reg);
	if (!ret)
		{
		 soft_iic_stop();
		 return 0xfe;
		}
	soft_iic_start();
	ret = soft_iic_tx_byte(addr|1);
	if (!ret)
		{
		 soft_iic_stop();
		 return 0xfe;
		}
	soft_iic_read_buf(buf, len);
	soft_iic_stop();
	return 0;
}

static u8 gyro_idle_query(void)
{
	return !is_gyro_active;
}
REGISTER_LP_TARGET(gyro_lp_target) = {
    .name = "gyro_lp",
    .is_idle = gyro_idle_query,
};


#endif

