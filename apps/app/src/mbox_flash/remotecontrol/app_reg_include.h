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


#ifndef _APP_REG_INCLUDE_H_
#define _APP_REG_INCLUDE_H_

#include "typedef.h"

typedef struct _KEY_STRUCT
{
  u8  	MK;  //MOUSE_KEY
  u8	SYS;
  u8 	MULITI1;//MULITI_KEY1
  u8 	MULITI2;//MULITI_KEY2
  u8 	MODIF;  //KEY_MODIF
  u8 	TYPE[6];  //KEY_TYPE1
}KEY_STRUCT;

extern KEY_STRUCT  _key;
#define  MOUSE_KKEY      _key.MK
#define  SYS_KEY 	     _key.SYS
#define  MULITI_KEY1     _key.MULITI1
#define  MULITI_KEY2     _key.MULITI2
#define  KEY_MODIF       _key.MODIF
#define  KEY_TYPE        _key.TYPE


typedef struct _MOUSE_STRUCT
{
  u8  KEY;
  u8	X;
  u8 	Y;
  u8 	Z;
  u8 	H;
}MOUSE_STRUCT;

extern MOUSE_STRUCT  _mouse;
#define  MOUSE_KEY    _mouse.KEY
#define  MOUSE_X 	    _mouse.X
#define  MOUSE_Y     	_mouse.Y
#define  MOUSE_Z     	_mouse.Z
#define  MOUSE_H       	_mouse.H

typedef struct _IR_STRUCT
{
  u16  	ir_addr; 
  u8	ir_data;
}IR_STRUCT;

extern IR_STRUCT  _ir_struct;
#define  IR_ADDR    _ir_struct.ir_addr
#define  IR_DATA 	  _ir_struct.ir_data

typedef struct
{
 u8		bit0:1;
 u8 	bit1:1;
 u8 	bit2:1;
 u8 	bit3:1;
 u8 	bit4:1;
 u8 	bit5:1;
 u8 	bit6:1;
 u8 	bit7:1;
}bool_byte;

typedef struct
{
 u8		bit0:1;
 u8 	bit1:1;
 u8 	bit2:1;
 u8 	bit3:1;
 u8 	bit4:1;
 u8 	bit5:1;
 u8 	bit6:1;
 u8 	bit7:1;
 u8		bit8:1;
 u8 	bit9:1;
 u8 	bit10:1;
 u8 	bit11:1;
 u8 	bit12:1;
 u8 	bit13:1;
 u8 	bit14:1;
 u8 	bit15:1;
}bool_int;

volatile typedef union
{
 u8		_strack;
 bool_byte _strack_bit;
}_bool_byte;

volatile typedef union
{
 u16		_strack;
 bool_int _strack_bit;
}_bool_int;

extern	_bool_byte	_led_bool_bit;
#define _led_bool_byte		_led_bool_bit._strack
#define fled_pair			_led_bool_bit._strack_bit.bit0
#define fled_pow			_led_bool_bit._strack_bit.bit1
#define fled_charge			_led_bool_bit._strack_bit.bit2
#define fled_r				_led_bool_bit._strack_bit.bit3
#define fled_g				_led_bool_bit._strack_bit.bit4
#define fled_b				_led_bool_bit._strack_bit.bit5

extern _bool_int  _key_bool_bit;
#define _key_bool_byte	_key_bool_bit._strack
#define key_check			_key_bool_bit._strack_bit.bit0
#define key_check_again		_key_bool_bit._strack_bit.bit1
#define fn_ok				_key_bool_bit._strack_bit.bit2
#define fkey_doing			_key_bool_bit._strack_bit.bit3
#define fm_doing			_key_bool_bit._strack_bit.bit4
#define fkey_blt			_key_bool_bit._strack_bit.bit5
#define fkey_blt_rem		_key_bool_bit._strack_bit.bit6
#define fkey_mic			_key_bool_bit._strack_bit.bit7
#define flong_key			_key_bool_bit._strack_bit.bit8
#define fn_pair				_key_bool_bit._strack_bit.bit9
#define flong_home			_key_bool_bit._strack_bit.bit10
#define flong_mic_key		_key_bool_bit._strack_bit.bit11
#define fkey_break			_key_bool_bit._strack_bit.bit12
#define fkey_wakeUp         _key_bool_bit._strack_bit.bit13
#define fkey_double_pre       _key_bool_bit._strack_bit.bit14
#define fmode_24g       _key_bool_bit._strack_bit.bit15
extern _bool_byte	_mouse_bool_bit;
#define _mouse_bool_byte	_mouse_bool_bit._strack
#define MOUSE_KEY_L			_mouse_bool_bit._strack_bit.bit0
#define MOUSE_KEY_U			_mouse_bool_bit._strack_bit.bit1
#define MOUSE_KEY_D			_mouse_bool_bit._strack_bit.bit2
#define MOUSE_KEY_R			_mouse_bool_bit._strack_bit.bit3

extern	_bool_int	_gy_bool_bit;
#define _gy_bool_byte		_gy_bool_bit._strack
#define fgy_initi			_gy_bool_bit._strack_bit.bit0
#define fdpi_save			_gy_bool_bit._strack_bit.bit1
#define gy_off_set			_gy_bool_bit._strack_bit.bit2
#define fgy_np				_gy_bool_bit._strack_bit.bit3
#define fgyro_calib			_gy_bool_bit._strack_bit.bit4
#define fgyro_330			_gy_bool_bit._strack_bit.bit5
#define fm_open				_gy_bool_bit._strack_bit.bit6
#define fgy_lowp			_gy_bool_bit._strack_bit.bit7
#define fgyros_3chanel	    _gy_bool_bit._strack_bit.bit8
#define fgy_np_pre    _gy_bool_bit._strack_bit.bit9
#define fgy_key_pause    _gy_bool_bit._strack_bit.bit10
#define fgy_inside    _gy_bool_bit._strack_bit.bit11
#define fkey_24gpair        _gy_bool_bit._strack_bit.bit12
#define fsys_initok          _gy_bool_bit._strack_bit.bit13
extern _bool_byte	_iic_bool_bit;
#define _iic_bool_byte	_iic_bool_bit._strack
#define ftx_rx_mode			_iic_bool_bit._strack_bit.bit0
#define fiic_busy			_iic_bool_bit._strack_bit.bit1
#define fiic_err			_iic_bool_bit._strack_bit.bit2
#define fiic_rx_star		_iic_bool_bit._strack_bit.bit3
#define fiic_re_addr		_iic_bool_bit._strack_bit.bit4
#define fiic_read_ok		_iic_bool_bit._strack_bit.bit5


extern _bool_int  _ir_bool_bit;
#define _ir_bool_byte		_ir_bool_bit._strack
#define fir_send			_ir_bool_bit._strack_bit.bit0
#define fir_retran			_ir_bool_bit._strack_bit.bit1
#define fir_star			_ir_bool_bit._strack_bit.bit2
#define fir_study			_ir_bool_bit._strack_bit.bit3
#define fir_study_star		_ir_bool_bit._strack_bit.bit4
#define fir_max_valid		_ir_bool_bit._strack_bit.bit5
#define fir_learn_finish	_ir_bool_bit._strack_bit.bit6
#define firk_learn			_ir_bool_bit._strack_bit.bit7
#define fir_led_ir			_ir_bool_bit._strack_bit.bit8
#define fir_led				_ir_bool_bit._strack_bit.bit9
#define firk_clrr			_ir_bool_bit._strack_bit.bit10
#define fir_learn_parity	_ir_bool_bit._strack_bit.bit11
#define fir_VALID_KEY	_ir_bool_bit._strack_bit.bit12
#define fir_learn_fail	_ir_bool_bit._strack_bit.bit13
#define fir_study_send    _ir_bool_bit._strack_bit.bit14
#define fir_led_long    _ir_bool_bit._strack_bit.bit15

extern _bool_byte	_rf_bool_bit;
#define _rf_bool_byte		_rf_bool_bit._strack
#define fmic_vld			_rf_bool_bit._strack_bit.bit0
#define fatv_micOpen	_rf_bool_bit._strack_bit.bit1
#define fatv_micClose	_rf_bool_bit._strack_bit.bit2
#define frf_key				_rf_bool_bit._strack_bit.bit3
#define frf_mouse			_rf_bool_bit._strack_bit.bit4
#define frf_mulit			_rf_bool_bit._strack_bit.bit5
#define fatv_caps			_rf_bool_bit._strack_bit.bit6

extern _bool_int	_system_bool_bit;
#define _system_bool_byte	_system_bool_bit._strack
#define flow_pow			    _system_bool_bit._strack_bit.bit0
#define fmic_order_1		  _system_bool_bit._strack_bit.bit1
#define fbt_donglePair		_system_bool_bit._strack_bit.bit2
#define fkey_pair			    _system_bool_bit._strack_bit.bit3
#define fsys_powup         _system_bool_bit._strack_bit.bit4
#define ftmode_up        _system_bool_bit._strack_bit.bit5
#define fsys_isAtv       	_system_bool_bit._strack_bit.bit6
#define fmic_send_busy		_system_bool_bit._strack_bit.bit7
#define fcc_pair          _system_bool_bit._strack_bit.bit8
#define fsys_100mstick     _system_bool_bit._strack_bit.bit9
#define fpwr_debug_pre     _system_bool_bit._strack_bit.bit10
#define fsys_fccSingle     _system_bool_bit._strack_bit.bit11
#define fkey_pair_cc			    _system_bool_bit._strack_bit.bit12
#define fhogp_open			    _system_bool_bit._strack_bit.bit13
#define fpwr_debug  _system_bool_bit._strack_bit.bit14
#define fkey_led           _system_bool_bit._strack_bit.bit15
#endif

/***********************************************************
						end file
***********************************************************/

