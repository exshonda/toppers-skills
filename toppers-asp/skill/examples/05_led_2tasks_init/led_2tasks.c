/*
 *  2タスクによるLED点滅プログラム(初期化ルーチン版)
 *  led_init() は ATT_INI でカーネル起動時に呼ばれるためタスク内では呼ばない
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "led_2tasks.h"

unsigned int led_data = 0;

#define LED1_INTERVAL 1000
#define LED3_INTERVAL  250

void
led1_task(intptr_t exinf){
    for (;;) {
        led_data |= LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);

        led_data &= ~LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
    }
}

void
led3_task(intptr_t exinf){
    for (;;) {
        led_data |= LED3;
        led_out(led_data);
        dly_tsk(LED3_INTERVAL);

        led_data &= ~LED3;
        led_out(led_data);
        dly_tsk(LED3_INTERVAL);
    }
}
