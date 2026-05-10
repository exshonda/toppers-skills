/*
 *  Interrupt Priority Mask Program
 *  SW1: CPUロック制御 / SW2,SW3: 割込み優先度マスク制御
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "int_mask.h"

unsigned int led_data = 0;

/*
 * スイッチ監視タスク
 */
void
switch_task(intptr_t exinf){
    unsigned char data;
    unsigned char pdata;

    pdata = switch_slide_sense();

    for(;;){
        data = switch_slide_sense();

        /* SW1 で CPUロック制御 */
        if (((data & SW1) == SW1) && (pdata & SW1) != SW1) {
            loc_cpu();
        }
        if (((data & SW1) != SW1) && (pdata & SW1) == SW1) {
            unl_cpu();
        }

        /* CPUロック中でないときだけ chg_ipm 呼出可 */
        if (sns_loc() != true) {
            if (((data & SW2) == SW2) && ((data & SW3) != SW3)) {
                chg_ipm(-5);                 /* SW2=ON,SW3=OFF: -5でマスク */
            } else if (((data & SW2) != SW2) && ((data & SW3) == SW3)) {
                chg_ipm(-6);                 /* SW2=OFF,SW3=ON: -6でマスク */
            } else {
                chg_ipm(0);                  /* マスク解除 */
            }
        }
        pdata = data;
    }
}

/*
 * EXTI9_5(PUSH1) 割込みハンドラ（優先度 -5）
 */
void
push1_handler(void){
    syslog(LOG_NOTICE, "push1_int : start!");
    if ((led_data & LED1) == LED1) {
        led_data &= ~LED1;
    } else {
        led_data |= LED1;
    }
    led_out(led_data);
    push1_int_clear();
}

/*
 * EXTI15_10(PUSH2) 割込みハンドラ（優先度 -6）
 */
void
push2_handler(void){
    syslog(LOG_NOTICE, "push2_int : start!");
    if ((led_data & LED2) == LED2) {
        led_data &= ~LED2;
    } else {
        led_data |= LED2;
    }
    led_out(led_data);
    push2_int_clear();
}
