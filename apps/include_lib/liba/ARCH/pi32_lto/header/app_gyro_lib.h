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


#ifndef _APP_GYRO_LIB_H_
#define _APP_GYRO_LIB_H_
#include "stdlib.h"


//gyro lib
unsigned char Kalman_filter(signed short *gyro,signed short *kal_dat);
void angle_line_arg(signed short *_gyro,signed short angx);
unsigned char angle_get_arg(unsigned int *angle);
char Kalman_filter_get_angle(signed short X,signed short Y,signed short);
signed short Kalman_filter_arg(signed short,signed short acc_ang,unsigned char);
unsigned char Kalman_filter_Hk(unsigned short r);
signed short Kalman_axis_limit(signed short gy,signed short angle);
unsigned char arg_bit16_to_bit8(signed short buff);
unsigned int mul13_16(unsigned short A,unsigned short B);

//audio lib
void aosp_onquest_hanlde(unsigned char *input,unsigned char *output,unsigned char input_size);
void aosp_htt_handle(unsigned char *input , unsigned char input_size);

#endif

/***********************************************************
						end file
***********************************************************/

