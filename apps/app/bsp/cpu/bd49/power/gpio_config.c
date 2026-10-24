#include "asm/power_interface.h"

//-----------------------------------------------------------------------------------------------------------------------
/* config
 */
#include "uart.h"
#include "gpio.h"
#include "adc_api.h"
#include "app_config.h"
#include "testbox_uart_update.h"
#include "audio.h"
#include "usb/otg.h"
#include "usb/host/usb_host.h"
#include "remotecontrol/rc_key.h"
#include "remotecontrol/rc_led.h"
#include "app_reg_include.h"
#include "rc_cswitch.h"

u32 get_port0_iomap();

extern struct _power_pdata power_pdata;

#define __TCFG_IO_CFG_AT_POWER_ON 	0//TCFG_IO_CFG_AT_POWER_ON
#define __TCFG_IO_CFG_AT_POWER_OFF	0//TCFG_IO_CFG_AT_POWER_OFF

/*-----------------------------------------------------------------------
 *进入、退出低功耗函数回调状态，函数单核操作、关中断，请勿做耗时操作
 *
 */
static u32 usb_io_con = 0;
extern const u32 usb_bk;
void sleep_enter_callback(u8 step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
	    putchar('<');
	    /* audio_off(); */
	    /* adc_hw_enter_sleep(); */
	    //USB IO打印引脚特殊处理
	    // if (usb_otg_online(0) == HOST_MODE) {
	    //     usb_host_suspend(0);
        // }

    } else {
    //     rc_key_matrix_sleep();
    // #if TCFG_CODE_SWITCH
    //     code_switch_ioClose();
    // #endif
    // #if TCFG_GYRO_ENABLE
    //     gpio_hw_direction_output(IO_PORT_SPILT(GYRO_VDD_SW),1); //todo gyro vdd
    // #endif
    //     // gpio_hw_direction_output(IO_PORT_SPILT(IR_TX_PIN),1);
    //     fsys_enterSniff = 1;
    }
}

void sleep_exit_callback(u32 usec)
{

    //USB IO打印引脚特殊处理
    /* adc_hw_exit_sleep(); */
    putchar('>');
}

/*-----------------------------------------------------------------------
 * 开机关机gpio配置，可视化工具配置
 * 请写注释及使用对应宏
 *
 */

static void __gpio_uninit();
void board_set_soft_poweroff(void)
{
    printf("%s, app", __FUNCTION__);

    __gpio_uninit();
	rc_key_matrix_sleep();
    gpio_hw_direction_output(IO_PORT_SPILT(CONNECT_LED_PIN),0);
    gpio_hw_direction_output(IO_PORT_SPILT(IR_TX_PIN),0);
#if TCFG_CODE_SWITCH
    code_switch_ioClose();
#endif
#if TCFG_RGB_ENABLE
    MOT_led_set(1);
#endif
    sys_write_sleep();
    /* do_platform_uninitcall(); */
}

struct gpio_cfg_item {
    u8 len;
    u16 uuid;
    u8  mode;
    u8  hd;
} __attribute__((packed));

static int __gpio_init()
{
#if __TCFG_IO_CFG_AT_POWER_ON
    struct gpio_cfg_item item;
    puts("gpio_cfg_at_power_on\n");

    for (int i = 0; ; i++) {
        int len = syscfg_read_item(CFG_ID_IO_CFG_AT_POWERON, i, &item, sizeof(item));
        if (len != sizeof(item)) {
            break;
        }
        u8 gpio = uuid2gpio(item.uuid);
        struct gpio_config config = {
            .pin   = BIT(gpio % 16),
            .mode   = item.mode,
            .hd     = item.hd,
        };
        gpio_init(gpio / 16, &config);
    }
#endif

    return 0;
}

//platform_initcall(__gpio_init);


//maskrom 使用到的io
static void __mask_io_cfg()
{
    struct boot_soft_flag_t boot_soft_flag = {0};

    boot_soft_flag.flag0.boot_ctrl.flash_stable_delay_sel = 0;   //0: 0mS;   1: 4mS
    boot_soft_flag.flag0.boot_ctrl.sfc_flash_stable_delay_sel = 1; //0: 0.5mS; 1: 1mS  配置1快速启动
    boot_soft_flag.flag0.boot_ctrl.sfc_fast_boot = 1;                                 //配置1快速启动

    boot_soft_flag.flag1.boot_ctrl.usbdm = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag1.boot_ctrl.usbdp = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag2.boot_ctrl.pa0 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag2.boot_ctrl.uart_key_port_pull_down = 0;

    boot_soft_flag.flag3.boot_ctrl.pa9 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag3.boot_ctrl.pa10 = SOFTFLAG_HIGH_RESISTANCE;
    mask_softflag_config(&boot_soft_flag);
}

static void __gpio_uninit()
{
    PORT_TABLE(g);

#if __TCFG_IO_CFG_AT_POWER_OFF
    struct gpio_cfg_item item;

    for (int i = 0; ; i++) {
        int len = syscfg_read_item(CFG_ID_IO_CFG_AT_POWEROFF, i, &item, sizeof(item));
        if (len != sizeof(item)) {
            break;
        }
        u8 gpio = uuid2gpio(item.uuid);
        struct gpio_config config = {
            /* .gpio   = uuid2gpio(item.uuid), */
            .pin   = BIT(gpio % 16),
            .mode   = item.mode,
            .hd     = item.hd,
        };
        /* gpio_set_config(&config); */
        gpio_init(gpio / 16, &config);
        PORT_PROTECT(gpio);
    }
#endif
    for (u8 i = 0; i < sizeof(rckey_row); i++) 
        PORT_PROTECT(rckey_row[i]);

#if TCFG_CODE_SWITCH
    // PORT_PROTECT(IO_PORTB_03);
    PORT_PROTECT(IO_PORTB_02);
#endif
    // PORT_PROTECT(IR_TX_PIN);
#if TESTBOX_UART_UPDATE_EN
    PORT_PROTECT(TCFG_UART_UPDATE_PORT);
#endif

    __mask_io_cfg();

    __port_init((u32)gpio_config);
}

//platform_uninitcall(__gpio_uninit);
