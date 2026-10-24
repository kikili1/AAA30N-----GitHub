#ifndef _RC_LED_H_
#define _RC_LED_H_

typedef enum {
    BT_PAIR_LED_CONNECT = 0,
    BT_PAIR_LED_DISCON = 1,
    BT_PAIR_LED_PAIRING = 2,
    BT_PAIR_LED_FCC     = 3,
} bt_pair_led_enum;

extern bt_pair_led_enum 	bt_led_mode;

u8 Connect_led_set(u8 en);
void ir_led_set(u8 en);
void MOT_led_set(u8 en);
void blt_led_set(void);
void sys_write_sleep(void);
void sys_power_led_pmu(void);
void cover_led_high_low(void);
int app_power_led_handler(void);
void sys_read_vm_powerup(void);

#endif
