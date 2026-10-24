#ifndef _SW_H_
#define _SW_H_

#include "cpu.h"

extern s8 cswitch_report_value;
typedef struct {
    u8 a_phase_io;
    u8 b_phase_io;
    // u8 gnd_phase_io;
} SW_PLATFORM_DATA;

extern u16 code_switch_timer;
void code_switch_init(void);
void code_switch_handler(void *priv);
void code_switch_ioClose(void);
#endif
