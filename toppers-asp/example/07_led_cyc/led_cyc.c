/*
 *  LED Blink Program with Cyclic Handler
 *  LED1 はタスク，LED3 は周期ハンドラで点滅
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "led_cyc.h"

unsigned int led_data = 0;

#define LED1_INTERVAL 1000

/*
 * LED1点滅タスク
 */
void
led1_task(intptr_t exinf){
    for (;;) {
        led_data &= ~LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);

        led_data |= LED1;
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
    }
}

/*
 * LED3点滅周期ハンドラ
 * 非タスクコンテキストで実行される（待ちAPI禁止）
 */
void
led3_cyc_handler(intptr_t exinf){
    if ((led_data & LED3) == LED3) {
        led_data &= ~LED3;
    } else {
        led_data |= LED3;
    }
    led_out(led_data);
}
