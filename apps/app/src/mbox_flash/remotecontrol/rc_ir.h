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


#ifndef _APP_IR_H_
#define _APP_IR_H_

#define TOTAL_IR_KEY   1
#define IR_TX_PIN	IO_PORTA_00
#define HL_LONG		6
#define IR_CUST_CODE0			0x08
#define IR_CUST_CODE1			0xf7

struct	_IR_STUDY_STRUCT{
 u8  ir_study_use[4];//4+24+99<128
 u32 ir_study_h[HL_LONG];
 u32 ir_study_l[HL_LONG];
 u8  ir_study_buf[76];
};

typedef union 
{
	struct _IR_STUDY_STRUCT ir_buf;
	u8 xdata_ir_buf[128];
}union_xdata_ir;

extern u8	ir_led_times;
extern u8	IR_KEY_NUM;
extern	u16 ir_valid_key;
extern u16  ir_study_timer;
extern u8 int_500ms_wait_study_ct;

void system_ir_initial(void);
void system_ir_vm_initial(void);
void system_ir_clrr_key(void);
void system_ir_check(void);
void system_ir_tran(void);
void system_ir_rece(void);
void user_ir_learn_timeout(void);


u8 ir_set_continue(void);
bool ir_compare_data(u8 a,u8 b);
u8 system_ir_only_send(u8 add1,u8 add2,u8 IR_DAT);


#endif

/***********************************************************
						end file
***********************************************************/

