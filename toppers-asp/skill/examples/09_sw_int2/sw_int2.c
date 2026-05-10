/*
 *  LED Blink Program with Two Interrupt Handlers
 *  PUSH1 → LED3 反転, PUSH2 → LED4 反転
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "sw_int2.h"

unsigned char led_data = 0;

#define LED1_INTERVAL 1000

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
 */
void
push1_handler(void){
    if ((led_data & LED3) == LED3) {
        led_data &= ~LED3;
    } else {
        led_data |= LED3;
    }
    push1_int_clear();
    led_out(led_data);
    syslog(LOG_NOTICE, "push1_int : led_data 0x%x", led_data);
}

/*
 * EXTI15_10(PUSH2) 割込みハンドラ
 */
void
push2_handler(void){
    if ((led_data & LED4) == LED4) {
        led_data &= ~LED4;
    } else {
        led_data |= LED4;
    }
    push2_int_clear();
    led_out(led_data);
    syslog(LOG_NOTICE, "push2_int : led_data 0x%x", led_data);
}
