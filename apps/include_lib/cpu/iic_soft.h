#ifndef	_IIC_SOFT_H_
#define _IIC_SOFT_H_

#include "typedef.h"
#include "gpio.h"

#include "iic_api.h"


int soft_iic_init(void);
void soft_iic_start(void);
void soft_iic_stop(void);
u8 soft_iic_tx_byte(u8 byte);
u8 soft_iic_rx_byte(u8 ack);
int soft_iic_read_buf(void *buf, int len);
int soft_iic_write_buf(const void *buf, int len);


#endif

