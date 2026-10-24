// #include "power_manage.h" 
#include "rc_cswitch.h"
#include "app_config.h"
#include "stdlib.h"
#include "app_reg_include.h"
#include "gpio.h"
#include "msg.h"
#include "jiffies.h"

#if TCFG_CODE_SWITCH

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[code_swtich]"
#include "log.h"

static u8 code_switch_active = 0;
u16 code_switch_timer = 0;
s8 cswitch_report_value = 0;

static void code_switch_event_to_usr(s8 sw_val);
static void get_code_value_handler(void *priv);
/* static void get_code_value_handler(void); */
static u8 code_switch_idle_query(void);

static void code_switch_event_to_usr(s8 sw_val)
{
	log_info("code_switch_event_to_usr\n");
    cswitch_report_value = sw_val;
    int ret = post_msg(1, MSG_CODE_SWITCH);
    if (ret != 0) {
        log_info("error !!! msg pool full \n");
    }
}

u8 code_postive_list[] = {
    ((0x00 << 4) | (0x02 << 2) | (0x03 << 0)), \
    ((0x00 << 4) | (0x02 << 2) | (0x01 << 0)), \
    ((0x00 << 4) | (0x03 << 2) | (0x01 << 0)), \

    ((0x02 << 4) | (0x03 << 2) | (0x01 << 0)), \
    ((0x02 << 4) | (0x03 << 2) | (0x00 << 0)), \
    ((0x02 << 4) | (0x01 << 2) | (0x00 << 0)), \

    ((0x03 << 4) | (0x01 << 2) | (0x00 << 0)), \
    ((0x03 << 4) | (0x01 << 2) | (0x02 << 0)), \
    ((0x03 << 4) | (0x00 << 2) | (0x02 << 0)), \

    ((0x01 << 4) | (0x00 << 2) | (0x02 << 0)), \
    ((0x01 << 4) | (0x00 << 2) | (0x03 << 0)), \
    ((0x01 << 4) | (0x02 << 2) | (0x03 << 0))
};

u8 code_negative_list[] = {
    ((0x00 << 4) | (0x01 << 2) | (0x03 << 0)), \
    ((0x00 << 4) | (0x01 << 2) | (0x02 << 0)), \
    ((0x00 << 4) | (0x03 << 2) | (0x02 << 0)), \

    ((0x01 << 4) | (0x03 << 2) | (0x02 << 0)), \
    ((0x01 << 4) | (0x03 << 2) | (0x00 << 0)), \
    ((0x01 << 4) | (0x02 << 2) | (0x00 << 0)), \

    ((0x03 << 4) | (0x02 << 2) | (0x00 << 0)), \
    ((0x03 << 4) | (0x02 << 2) | (0x01 << 0)), \
    ((0x03 << 4) | (0x00 << 2) | (0x01 << 0)), \

    ((0x02 << 4) | (0x00 << 2) | (0x01 << 0)), \
    ((0x02 << 4) | (0x00 << 2) | (0x03 << 0)), \
    ((0x02 << 4) | (0x01 << 2) | (0x03 << 0))
};

//功能：相位纠正
static void correct_code(u8 *code_data, u8 *list_data, u8 idx)
{
    code_data[2] = (list_data[(idx / 3) * 3] & (0x03 << 4)) >> 4;
    code_data[1] = (list_data[(idx / 3) * 3] & (0x03 << 2)) >> 2;
    code_data[0] = (list_data[(idx / 3) * 3] & (0x03 << 0)) >> 0;

    log_debug("lost_phase_recover: %d  %d  %d\n", code_data[2], code_data[1], code_data[0]);
}

//功能：相位遍历
static u8 traverse_phase_list(u8 *code_data, u8 *list_data, u8 size)
{
    u8 code_num = 0, idx = 0;

    code_num = code_data[2] << 4 | code_data[1] << 2 | code_data[0] << 0;

    for (idx = 0; idx < size; idx++) {
        if (code_num == list_data[idx]) {
            if (idx % 3) {
                /* correct_code(code_data, list_data, idx); */
            }
            break;
        }
    }

    return idx;
}

//功能：消抖
static bool code_phase_debounce(u8 code_val)
{
    static u8 filter_value = 0, filter_cnt = 0;
    const u8 filter_time = 2;

    //当前值与上一次值如果不相等, 重新消抖处理, 注意filter_time != 0;
    if (code_val != filter_value && filter_time) {
        filter_cnt = 0; 		//消抖次数清0, 重新开始消抖
        filter_value = code_val;//记录上一次的值
        return false; 		    //第一次检测, 返回不做处理
    }

    //当前值与上一次值相等, filter_cnt开始累加;
    if (filter_cnt < filter_time) {
        filter_cnt++;
        return false;
    }

    return true;
}

//功能：获取相位
static bool get_code_phase(u8 *code_data)
{
    static u8 code_now = 0, code_last = 0;
    static u8 debug_count = 0;

    //获取相位值
    code_last = code_now;
    code_now = (gpio_read(A_PHASE_PORT) ? 0x02 : 0x00) + (gpio_read(B_PHASE_PORT) ? 0x01 : 0x00);
	// printf("code_now=%x",code_now);
    //相位状态出现变化，将相位值存入队列
    if (code_last != code_now && code_now != code_data[0]) {
        code_data[2] = code_data[1];
        code_data[1] = code_data[0];
        code_data[0] = code_now;

        code_switch_active = 50;//编码开关工作状态标志位

        /* log_debug(">>>%d<<<%d  %d  %d\n", debug_count, code_data[2], code_data[1], code_data[0]); */
        debug_count++;
        return true;
    }

    //相位状态无变化，编码开关工作状态标志位递减
    else {
        if (code_switch_active) {
            code_switch_active--;
        }
        // else log_debug("enter power_down!!!!");
    }

    return false;
}

//功能：获取编码值
u8 get_code_value(u8 *code_data)
{
    u8 idx = 0;   //相位序列号
    static u8 code_val = 0;
    static u8 move_now = 0, move_last = 0;
    static u8 idx_pos_last = 0, idx_neg_last = 0;
    enum {
        MOVE_POS,
        MOVE_NEG,
    };

    //正相位遍历，获取相位序列号
    idx = traverse_phase_list(code_data, &code_postive_list[0], ARRAY_SIZE(code_postive_list));

    //相位序列号有效，则出现正向滚动动作
    if (idx < ARRAY_SIZE(code_postive_list)) {
        move_last = move_now;
        move_now = MOVE_POS;

        //相位区域限制，避免重复检测
        if ((idx >= 0 && idx <= 2) || (idx >= 6 && idx <= 8) || (move_now != move_last))
            /* if (abs(idx_pos_last/3-idx/3)>=2 || (move_now != move_last)) */
        {
            code_val--; //正向滚动，编码值+1
            idx_pos_last = idx;
            /* log_debug(">>>>>>>>>>>>>>>>>>>>>>>idx = %d  R\n", idx); */
            /* log_debug("\n"); */
            return code_val;
        }
    }


    //负相位遍历，获取相位序列号
    idx = traverse_phase_list(code_data, &code_negative_list[0], ARRAY_SIZE(code_negative_list));

    //相位序列号有效，则出现负向滚动
    if (idx < ARRAY_SIZE(code_negative_list)) {
        move_last = move_now;
        move_now = MOVE_NEG;

        //相位区域限制，避免重复检测
        if ((idx >= 3 && idx <= 5) || (idx >= 9 && idx <= 11) || (move_now != move_last))
            /* if (abs(idx_neg_last/3-idx/3)>=2 || (move_now != move_last)) */
        {
            code_val++; //负向滚动，编码值-1
            idx_neg_last = idx;
            /* log_debug(">>>>>>>>>>>>>>>>>>>>>>>idx = %d  L\n", idx); */
            /* log_debug("\n"); */

            return code_val;
        }
    }

    return code_val;
}

//功能：旋转编码开关检测
static u8 code_switch_detector(void)
{
    static u8 code_table[3] = {0}; //相位值存储队列
    static u8 code_val = 0;

    //相位有变化，进行相位遍历，获取编码值
    if (get_code_phase(&code_table[0])) {
        code_val = get_code_value(&code_table[0]);
    }

    //相位无变化，编码值保持不变
    else {
        code_val = code_val;
    }

    return code_val;
}


//功能：旋转编码开关数据采集
void code_switch_handler(void *priv)
{
    static u8 code_val_last = 0;
    static u8 code_val_now  = 0;
    static u8 int_codes_doing_ct_p =0,int_codes_doing_ct_n =0;
	u8 code_switch_value;
    // extern bool fcode_switch;

    // if(!code_switch_active)
    // {
    //     // if(!gpio_read(IO_PORTB_03) || !gpio_read(IO_PORTB_02))
    //     //     code_switch_active = 200;
    //     return ;
    // }
    code_switch_init();
	delay(50);
    code_val_last = code_val_now;
    code_val_now = code_switch_detector();
	// log_info("code_val_now=%x",code_val_now);
    if (code_val_last != code_val_now) {
    	code_switch_value = code_val_now - code_val_last;
    	// log_info("code_switch_value=%x",code_switch_value);
    	if(code_switch_value ==1)
    	{
	    	if(++ int_codes_doing_ct_p <1)//4ms
				int_codes_doing_ct_p ++;
			else {
                int_codes_doing_ct_p = 2;
                fkey_doing = 1;
		        code_switch_event_to_usr(code_switch_value);
		        }
	    }
    	else int_codes_doing_ct_p =0;
    	if(code_switch_value == 0xff)
    	{
	    	if(++ int_codes_doing_ct_n <1)
				int_codes_doing_ct_n ++;
			else {
                int_codes_doing_ct_n = 2;
                fkey_doing = 1;
		        code_switch_event_to_usr(code_switch_value);
		        }
	    }
    	else int_codes_doing_ct_n =0;
    }
}

void code_switch_init(void)
{
    gpio_set_mode(IO_PORT_SPILT(A_PHASE_PORT), PORT_INPUT_PULLUP_1M);
    gpio_hw_set_direction(IO_PORT_SPILT(A_PHASE_PORT), 1);
    gpio_hw_set_pull_up(IO_PORT_SPILT(A_PHASE_PORT), 1);
    gpio_hw_set_pull_down(IO_PORT_SPILT(A_PHASE_PORT), 0);
    gpio_hw_set_die(IO_PORT_SPILT(A_PHASE_PORT), 1);
    gpio_hw_set_dieh(IO_PORT_SPILT(A_PHASE_PORT), 1);

    // gpio_set_mode(IO_PORT_SPILT(sw_pdata->b_phase_io), PORT_INPUT_PULLUP_100K);
    gpio_hw_set_direction(IO_PORT_SPILT(B_PHASE_PORT), 1);
    gpio_hw_set_pull_up(IO_PORT_SPILT(B_PHASE_PORT), 0);
    gpio_hw_set_pull_down(IO_PORT_SPILT(B_PHASE_PORT), 0);
    gpio_hw_set_die(IO_PORT_SPILT(B_PHASE_PORT), 1);
    gpio_hw_set_dieh(IO_PORT_SPILT(B_PHASE_PORT), 1);

    // if(!code_switch_timer)
    //     code_switch_timer = sys_timer_add(NULL, code_switch_handler, 2); //2ms
}

#include "power_interface.h"
void rc_cswitch_active_set(P33_IO_WKUP_EDGE edge)
{
    code_switch_active = 50;
	code_switch_init();
}
void code_switch_ioClose(void)
{
    gpio_hw_set_direction(IO_PORT_SPILT(A_PHASE_PORT), 1);
    gpio_hw_set_pull_up(IO_PORT_SPILT(A_PHASE_PORT), 0);
    gpio_hw_set_pull_down(IO_PORT_SPILT(A_PHASE_PORT), 0);
    gpio_hw_set_die(IO_PORT_SPILT(A_PHASE_PORT), 0);
    gpio_hw_set_dieh(IO_PORT_SPILT(A_PHASE_PORT), 0);

    gpio_hw_set_direction(IO_PORT_SPILT(B_PHASE_PORT), 1);
    gpio_hw_set_pull_up(IO_PORT_SPILT(B_PHASE_PORT), 0);
    gpio_hw_set_pull_down(IO_PORT_SPILT(B_PHASE_PORT), 0);
    gpio_hw_set_die(IO_PORT_SPILT(B_PHASE_PORT), 1);
    gpio_hw_set_dieh(IO_PORT_SPILT(B_PHASE_PORT), 1);
}

static u8 code_switch_idle_query(void)
{
    return (!code_switch_active);
}

REGISTER_LP_TARGET(code_switch_lp_target) = {
    .name = "code_switch",
    .is_idle = code_switch_idle_query,
};

#endif 
