/*
 *  LED Blink Program with Interrupt (PUSH1)
 *  PUSH1 ボタン押下で LED3 を反転
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "sw_int.h"

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
 * EXTI9_5(PUSH1) 割込みハンドラ
 * 注意: シグネチャは void(void) で exinf を持たない
 */
void
push1_handler(void){
    if ((led_data & LED3) == LED3) {
        led_data &= ~LED3;
    } else {
        led_data |= LED3;
    }
    push1_int_clear();    /* デバイスの割込みフラグクリア（必須） */
    led_out(led_data);
    syslog(LOG_NOTICE, "push1_int : led_data 0x%x", led_data);
}
