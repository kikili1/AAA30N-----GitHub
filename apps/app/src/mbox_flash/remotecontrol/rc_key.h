#ifndef _RC_KEY_H_
#define _RC_KEY_H_

#include "gpio.h"

extern u8 RGB_CT;
#define ROW_MAX               6
#define COW_MAX               5
#define COW_ADK               0

#define KEYBOARD_REPORT_ID          0x1
#define COUSTOM_CONTROL_REPORT_ID   0x2
#define SYS_CONTROL_REPORT_ID   	0x3
#define MOUSE_POINT_REPORT_ID       0x4

static u32 krow_num = 0,kcol_num =0;
static u8 rckey_row[] = { //normal io  这里IO有变化则要注意设置power_api.c的唤醒口
    IO_PORTB_02,//R0
    IO_PORTB_01,//R1
    IO_PORTA_00,//R2
    IO_PORTA_15,//R3
    IO_PORTA_14,//R4
    IO_PORTA_06,//R6
};
static u8 rckey_col[] = { //normal io
    IO_PORTA_13,//C0
    IO_PORTA_12,//C1
    IO_PORTA_11,//C2
    IO_PORTA_05,//C3
    IO_PORTA_02,//C4
};

void rc_matrix_key_init(void);
void rc_key_scan(void);
void rckey_tick_scan(void);
void rf_send_hid_key(void);
void rc_key_init(void);
void rc_key_matrix_sleep(void);
// void matrix_key_col_status(enum gpio_mode mode);
//
//  key code
//

#define K_NULL	0x00

//×ÖÄ¸
#define KA    0x04		//A
#define KB    0x05		//B
#define KC	  0x06		//C
#define KD 	  0x07		//D
#define KE 	  0x08 		//E
#define KF    0x09		//F
#define KG    0x0A		//G
#define KH    0x0B		//H
#define KI    0x0C		//I
#define KJ    0x0D		//J
#define KK    0x0E	  //K
#define KL    0x0F		//L
#define KM    0x10		//M
#define KN    0x11		//N
#define KO    0x12  	//O
#define KP    0x13		//P
#define KQ    0x14 		//Q
#define KR    0x15		//R
#define KS    0x16	  //S
#define KT    0x17		//T
#define KU    0x18		//U
#define KV    0x19		//V
#define KW    0x1A 		//W
#define KX    0x1B		//X
#define KY    0x1C		//Y
#define KZ    0x1D		//Z

//F1 -- F12
#define KF1  0x3A	     //F1
#define KF2  0x3B   	 //F2
#define KF3  0x3C	     //F3
#define KF4  0x3D	     //F4
#define KF5  0x3E	     //F5
#define KF6  0x3F	     //F6
#define KF7  0x40	     //F7
#define KF8  0x41	     //F8
#define KF9  0x42	     //F9
#define KF10 0x43      //F10
#define KF11 0x44      //F11
#define KF12 0x45	     //F12

//ÉÏÊý×Ö
#define K0    0x27		//) 0
#define K1    0x1E		//! 1
#define K2    0x1F		//@ 2
#define K3    0x20		//# 3
#define K4    0x21		//$ 4
#define K5    0x22		//%5
#define K6    0x23		//^6
#define K7    0x24		//& 7
#define K8    0x25		//* 8
#define K9    0x26		//( 9
#define KMi   0x2D	  //¡ª¡ª _
#define KEq   0x2E	  //+=

// num pad
#define KNum 	 0x53		  //Num lock
#define KNMulti  0x55			//*
#define KNDivi   0x54			// /
#define KNPlus 	 0x57		  //+
#define KNMinus  0x56			//-
#define KN9 	   0x61			//9
#define KN8 	   0x60			//8
#define KN7 	   0x5F   	//7
#define KN6 	   0x5E			//6
#define KN5 	   0x5D			//5
#define KN4 	   0x5C			//4
#define KN3 	   0x5B			//3
#define KN2 	   0x5A			//2
#define KN1 	   0x59			//1
#define KN0 	   0x62			//0
#define KNP  	   0x63			//.
#define KNEnter  0x58			//£¨Ð¡Enter£©

//¿ØÖÆ
#define KHome 	 0x4A	    //Home
#define KIns 	   0x49	    //insert
#define KDel	   0x4C	    //delete
#define KEnd 	   0x4D	    //end
#define KPu      0x4B	    //page up
#define KPd      0x4E	    //page down


#define KEsc     0x29	     //ESC
#define KCaps    0x39	     //Caps lock
#define KTab     0x2B	     //tab
#define KScrol   0x47	     //Scroll lock
#define KPrcsc	 0x46	     //prcsc sysrq
#define KPause 	 0x48	     //Pause break
#define KBack 	 0x2A	     //Back space
#define KEnter 	 0x1041//0x28     	 //Enter
#define KSpace 	 0x2C	     //space

#define KVBar 	 0x31	     //;|"\"
#define KDQuote  0x34      //;" '
#define KColon 	 0x33	     //: ;
#define KThrow 	 0x35    	 //~ `
#define KLTS 		 0x36	     //<,
#define KGTS 		 0x37	     //>.
#define KQM 		 0x38	     //?/
#define KBR 		 0x30	     //}]
#define KBL 		 0x2F	     //{[
#define K_29         0x31
#define K_42         0x32
#define K_45         0x64
#define K_App        0x65

// #define KUp		  0x1042		//Up arrow
// #define KLeft   0x1044		//Left arrow
// #define KRight  0x1045		//Right arrow
// #define KDown   0x1043		//Down arrow

#define KUp		  0x52		//Up arrow
#define KLeft   0x50		//Left arrow
#define KRight  0x4F		//Right arrow
#define KDown   0x51		//Down arrow

#define K_Ctrl   0x4101
#define K_Shift  0x4102
#define K_Alt    0x4104
#define K_Win    0x4108
#define K_RCtrl  0x4110
#define K_RShift 0x4120
#define K_RAlt   0x4140
#define K_RWin   0x4180

#define K_CAL   0x1192    //cal
#define K_WBACK 0x1224   //www back
#define K_WREG  0x1227   //
#define K_WSEA  0x1221   //
#define K_WFAV  0x122A   //
#define K_WFOR  0x1225   //
#define K_MED   0x1183   //
#define K_WHOME 0x1223//   //
#define K_COMP  0x1194   //
#define K_EMAIL 0x118A   //
#define K_NEXT  0x10B5   //
#define K_PRE   0x10B6   //
#define K_STOP  0x10B7   //
#define K_PLAY  0x10CD   //
#define K_VOLI  0x10E9   //
#define K_VOLD  0x10EA   //
#define K_MUTE  0X10E2//0x10E2   //
#define K_REW   0x10B4   //
#define K_FFOR  0x10B3   //
#define K_Mail  0x118a   //
#define K_ENET  0x1196   //we
#define K_MIC	0x1221
#define K_NAR   0x122e
#define K_EXP   0x122D
#define K_EMX3  0x108A   //we

#define K_WPU	0x106b
#define K_WPD	0x106c
#define K_SET	0x1068
#define K_WApp  0x102a
#define K_WUp	  0x1042		//Up arrow
#define K_WLeft   0x1044		//Left arrow
#define K_WRight  0x1045		//Right arrow
#define K_WDown   0x1043		//Down arrow
#define K_OK 	0x1041 // 0X28
#define K_WSleep 0x1030
#define K_BTMIC	0x1221

#define K_Sleep     0x66//0x1030//0x2101//

#define K_Power 0X2102

#define K_ML    0x3101
#define K_MR    0x3102
#define K_MM    0x3104


#define K_FN		0x5001
#define K_TOUCH 	0x5002
#define K_FLAG	 	0x5003 
#define K_SET	 	0x100a//0x100A//0x108f//0x109f//
#define K_PRIME 	0x1079
#define K_SOURCE 	0x11bb
#define K_BLT		0x500B
#define K_Table		0x1040//0X1011//0x1229//0x102a
#define K_TV		0x1089//0X72////0x108d//0x1089




#define K_YOUT    	0X1009//0x1009//0X1056//0x1077//0x100f//
#define K_NETF   	0X100E//0x100E//0X103B//0x1078//0x106f//
#define K_GOOGP     0x1007//0X103D//0x107a//0x1072//
#define K_TIVI      0x1005//0X103E//0X5005
#define K_Lenmo      0x1089

#define K_LANG 	    0x1089
#define K_Delete    0x2a//0x1040

#define K_PRIME     0x1006
#define K_G      0x1007
#define K_Y      0x1063
#define K_B      0x1062


#define PN_K	0x8000

#endif