/*
 *  2タスクによるLED点滅プログラム(dly_tsk版)
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "led_2tasks.h"

unsigned int led_data = 0;

#define LED1_INTERVAL 1000    /* LED1点滅間隔 [ms] */
#define LED3_INTERVAL  250    /* LED3点滅間隔 [ms] */

/*
 * LED1点滅タスク（1秒間隔）
 */
void
led1_task(intptr_t exinf){
    led_init(0);
    for (;;) {
        led_data |= LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);

        led_data &= ~LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
    }
}

/*
 * LED3点滅タスク（0.25秒間隔）
 */
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
