//#include "system/init.h"
#include "iic_soft.h"
#include "gpio.h"
#include "clock.h"
#include "cpu.h"

#define IIC_DIR_INPUT  	gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DP), 1);\
                        gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DM), 1)

#define IIC_DIR_OUTPUT	gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DP), 0);\
                        gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DM), 0)

#define IIC_DIE_GPIO  	gpio_hw_set_die(IO_PORT_SPILT(IO_PORT_DP), 1);\
	 	                gpio_hw_set_dieh(IO_PORT_SPILT(IO_PORT_DP), 1);\
                        gpio_hw_set_die(IO_PORT_SPILT(IO_PORT_DM), 1);\
	 	                gpio_hw_set_dieh(IO_PORT_SPILT(IO_PORT_DM), 1)
#define IIC_DIE_ANA		gpio_hw_set_die(IO_PORT_SPILT(IO_PORT_DP), 0);\
	 	                gpio_hw_set_dieh(IO_PORT_SPILT(IO_PORT_DP), 0);\
                        gpio_hw_set_die(IO_PORT_SPILT(IO_PORT_DM), 0);\
	 	                gpio_hw_set_dieh(IO_PORT_SPILT(IO_PORT_DM), 0)

#define IIC_PU_EN  	    gpio_hw_set_pull_up(IO_PORT_SPILT(IO_PORT_DP), 1);\
                        gpio_hw_set_pull_up(IO_PORT_SPILT(IO_PORT_DM), 1)
#define IIC_PU_DIS	    gpio_hw_set_pull_up(IO_PORT_SPILT(IO_PORT_DP), 0);\
                        gpio_hw_set_pull_up(IO_PORT_SPILT(IO_PORT_DM), 0)

#define IIC_PD_EN  	    gpio_hw_set_pull_down(IO_PORT_SPILT(IO_PORT_DP), 1);\
                        gpio_hw_set_pull_down(IO_PORT_SPILT(IO_PORT_DM), 1)
#define IIC_PD_DIS	    gpio_hw_set_pull_down(IO_PORT_SPILT(IO_PORT_DP), 0);\
                        gpio_hw_set_pull_down(IO_PORT_SPILT(IO_PORT_DM), 0)

#define SCL_DIR_INPUT   gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DP), 1)
#define SCL_DIR_OUTPUT  gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DP), 0)

#define SDA_DIR_INPUT   gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DM), 1)
#define SDA_DIR_OUTPUT  gpio_hw_set_direction(IO_PORT_SPILT(IO_PORT_DM), 0)

#define SCL_IN    	    gpio_hw_read_port(IO_PORT_SPILT(IO_PORT_DP))
#define SDA_IN 	        gpio_hw_read_port(IO_PORT_SPILT(IO_PORT_DM))

#define SCL_OUT_H       gpio_hw_write_port(IO_PORT_SPILT(IO_PORT_DP), 1)
#define SCL_OUT_L       gpio_hw_write_port(IO_PORT_SPILT(IO_PORT_DP), 0)

#define SDA_OUT_H       gpio_hw_write_port(IO_PORT_SPILT(IO_PORT_DM), 1)
#define SDA_OUT_L       gpio_hw_write_port(IO_PORT_SPILT(IO_PORT_DM), 0)


int soft_iic_init(void)
{
    usb_iomode(1);

	IIC_DIE_GPIO;

	IIC_DIR_INPUT;

	IIC_PU_DIS;
	IIC_PD_DIS;
	
	SCL_OUT_H;//SCL 1
	SDA_OUT_H; //SDA 1
    return 0;
}

void soft_iic_start(void)
{
	IIC_DIR_OUTPUT;//SCL SDA OUTPUT
	SDA_OUT_H;//SDA 1
	delay(2*5);
	SCL_OUT_H;//SCL 1
	delay(2*5);
	SDA_OUT_L; // SDA 0
}

void soft_iic_stop(void)
{
	SDA_OUT_L; // SDA 0
	delay(2*5);
	SCL_OUT_H;//SCL 1
	delay(2*5);
	SDA_OUT_H; //SDA 1
}

static u8 soft_iic_check_ack(void)
{
    u8 ack;

	SCL_OUT_L;
	SDA_DIR_INPUT;//SDA  INPUT
	delay(1*5);
	SCL_OUT_H;
	delay(2*5);
	if(SDA_IN ==0 )
		ack = 1;
	else ack = 0;
	
    SCL_OUT_L;
	SDA_DIR_OUTPUT;//SDA  OUTPUT

    return ack;
}

static void soft_iic_rx_ack(void)
{
	SDA_OUT_L;
	delay(1*5);
	SCL_OUT_H;
	delay(2*5);
	SCL_OUT_L;
}

static void soft_iic_rx_nack(void)
{
	SDA_OUT_H;
	delay(1*5);
	SCL_OUT_H;
	delay(2*5);
	SCL_OUT_L;
}

u8 soft_iic_tx_byte(u8 byte)
{
    u8 i, ret;
	
	SCL_OUT_L;
    for (i = 0; i < 8; i++) {  //MSB FIRST
    	delay(1*5);
        if ((byte << i) & 0x80) {
           SDA_OUT_H;
        } else {
            SDA_OUT_L;
        }
		SCL_OUT_H;
		SCL_DIR_INPUT;//SCL INPUT 
		while(!SCL_IN);
		SCL_DIR_OUTPUT;//SCL OUTPUT  
		delay(1*5);
		SCL_OUT_L;
    }
    return soft_iic_check_ack();
}

u8 soft_iic_rx_byte(u8 ack)
{
    u8 byte = 0, i;
	
	SDA_DIR_INPUT;
    for (i = 0; i < 8; i++) {
		delay(2*5);
        SCL_OUT_H;
		IIC_DIR_INPUT;//SCL INPUT   SDA  INPUT
		while(!SCL_IN);
		SCL_DIR_OUTPUT;//SCL OUTPUT   SDA  INPUT
		delay(1*5);
        byte = byte << 1;
		if(SDA_IN){
            byte |= 1;
        }
		SCL_OUT_L;
    }

	SDA_DIR_OUTPUT;//SDA  OUTPUT
    if (ack) {
        soft_iic_rx_ack();
    } else {
        soft_iic_rx_nack();
    }

    return byte;
}


int soft_iic_read_buf(void *buf, int len)
{
    int i = 0;

    if (!buf || !len) {
        return -1;
    }
    for (i = 0; i < len - 1; i++) {
        ((u8 *)buf)[i] = soft_iic_rx_byte(1);
    }
    ((u8 *)buf)[len - 1] = soft_iic_rx_byte(0);
    return len;
}

int soft_iic_write_buf(const void *buf, int len)
{
    int i;
    u8 ack;

    if (!buf || !len) {
        return -1;
    }
    for (i = 0; i < len; i++) {
        ack = soft_iic_tx_byte(((u8 *)buf)[i]);
        if (ack == 0) {
            break;
        }
    }
    return i;
}

