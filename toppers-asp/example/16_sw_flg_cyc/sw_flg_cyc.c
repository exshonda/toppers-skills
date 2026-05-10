/*
 *  Event Flag Notify Program (Cyclic Handler → Task)
 *  周期ハンドラがスイッチをスキャンしてイベントフラグでタスクに通知
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include <t_stdlib.h>
#include "device.h"
#include "sw_flg_cyc.h"

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)    svc_perror(__FILE__, __LINE__, #expr, (expr))

/* イベントビット割付け */
#define SW1_ON      0x01
#define SW1_OFF     0x02
#define SW2_ON      0x04
#define SW2_OFF     0x08
#define SW3_ON      0x10
#define SW3_OFF     0x20
#define SW4_ON      0x40
#define SW4_OFF     0x80
#define SW_EV_MASK  0xff

unsigned int led_data;
unsigned int sw_state;

/*
 * 初期化ルーチン: 起動時のSW状態をLEDに反映
 */
void
state_init(intptr_t exinf){
    led_data = 0;
    sw_state = switch_slide_sense();

    if((sw_state & SW1) == SW1) led_data |= LED1;
    if((sw_state & SW2) == SW2) led_data |= LED2;
    if((sw_state & SW3) == SW3) led_data |= LED3;
    if((sw_state & SW4) == SW4) led_data |= LED4;

    led_out(led_data);
}

/*
 * LED表示タスク（イベントフラグ待ち）
 */
void
led_task(intptr_t exinf){
    FLGPTN flgptn;

    for (;;) {
        wai_flg(SW_FLG, SW_EV_MASK, TWF_ORW, &flgptn);
        syslog(LOG_NOTICE, "led_task : eventflag pattern = 0x%x", flgptn);

        if ((flgptn & SW1_ON)  == SW1_ON)  led_data |= LED1;
        if ((flgptn & SW1_OFF) == SW1_OFF) led_data &= ~LED1;
        if ((flgptn & SW2_ON)  == SW2_ON)  led_data |= LED2;
        if ((flgptn & SW2_OFF) == SW2_OFF) led_data &= ~LED2;
        if ((flgptn & SW3_ON)  == SW3_ON)  led_data |= LED3;
        if ((flgptn & SW3_OFF) == SW3_OFF) led_data &= ~LED3;
        if ((flgptn & SW4_ON)  == SW4_ON)  led_data |= LED4;
        if ((flgptn & SW4_OFF) == SW4_OFF) led_data &= ~LED4;

        led_out(led_data);
    }
}

/*
 * スイッチスキャン周期ハンドラ
 * 状態変化があったらイベントフラグをセットして通知
 */
void
sw_cyc_handler(intptr_t exinf){
    unsigned char tmp;
    FLGPTN flgptn = 0;

    tmp = switch_slide_sense();

    if ((sw_state & SW1) != (tmp & SW1)) {
        flgptn |= ((tmp & SW1) == SW1) ? SW1_ON : SW1_OFF;
    }
    if ((sw_state & SW2) != (tmp & SW2)) {
        flgptn |= ((tmp & SW2) == SW2) ? SW2_ON : SW2_OFF;
    }
    if ((sw_state & SW3) != (tmp & SW3)) {
        flgptn |= ((tmp & SW3) == SW3) ? SW3_ON : SW3_OFF;
    }
    if ((sw_state & SW4) != (tmp & SW4)) {
        flgptn |= ((tmp & SW4) == SW4) ? SW4_ON : SW4_OFF;
    }

    if (flgptn != 0) {
        SVC_PERROR(iset_flg(SW_FLG, flgptn));
        sw_state = tmp;
    }
}
