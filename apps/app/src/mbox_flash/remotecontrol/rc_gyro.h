/***********************************************************
Copyright(C): XingWei Technology
FileName	:
Author	  	: Owen
Date		:2016/05/17
Description	:
Version		: V1.0
History		: Author      Date                 Ver       Note
              Owen        2016/05/17           1.0       created
***********************************************************/


#ifndef _APP_GYRO_H_
#define _APP_GYRO_H_

#define GY_TOL 0x2a


extern u8  dpi_data;
extern u16 gyro_initi_timer,gy_scan_timer;
extern int int_gy_sleep_timer;
extern u16 min_interval;

int system_gyro_initi_matrix(void);
void system_gyro_initial(void);
void system_gyro_power_mgmt(u8 mode);
void system_gyro_power_save_read(void);
void sys_gyro_open_sleep_handle(void);
void gyro_step_read(u8 *);
void system_gyro_check(void);
void gyro_read_defult(s16  *FIFO);
void sys_gyro_offset_save(void);
u8 tolerance_sub(s16 dat,s8 *co);

#endif

/***********************************************************
						end file
***********************************************************/

