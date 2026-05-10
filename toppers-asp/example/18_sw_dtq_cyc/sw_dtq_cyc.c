/*
 *  DataQueue Notify Program (Cyclic Handler → Task)
 *  周期ハンドラがスイッチ変化を検出してデータキューでタスクに通知
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include <t_stdlib.h>
#include "device.h"
#include "sw_dtq_cyc.h"

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)    svc_perror(__FILE__, __LINE__, #expr, (expr))

/* キューに送るイベントコード */
#define SW1_ON      0x01
#define SW1_OFF     0x02
#define SW2_ON      0x04
#define SW2_OFF     0x08
#define SW3_ON      0x10
#define SW3_OFF     0x20
#define SW4_ON      0x40
#define SW4_OFF     0x80

unsigned int led_data;
unsigned int sw_state;

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
 * LED表示タスク（データキュー受信）
 */
void
led_task(intptr_t exinf){
    intptr_t sw_data;

    for (;;) {
        rcv_dtq(SW_DTQ, &sw_data);
        syslog(LOG_NOTICE, "led_task : recive data = 0x%x", sw_data);

        if ((sw_data & SW1_ON)  == SW1_ON)  led_data |= LED1;
        if ((sw_data & SW1_OFF) == SW1_OFF) led_data &= ~LED1;
        if ((sw_data & SW2_ON)  == SW2_ON)  led_data |= LED2;
        if ((sw_data & SW2_OFF) == SW2_OFF) led_data &= ~LED2;
        if ((sw_data & SW3_ON)  == SW3_ON)  led_data |= LED3;
        if ((sw_data & SW3_OFF) == SW3_OFF) led_data &= ~LED3;
        if ((sw_data & SW4_ON)  == SW4_ON)  led_data |= LED4;
        if ((sw_data & SW4_OFF) == SW4_OFF) led_data &= ~LED4;

        led_out(led_data);
    }
}

/*
 * スイッチスキャン周期ハンドラ
 * 変化があればデータキューにイベントコードを送信
 */
void
sw_cyc_handler(intptr_t exinf){
    unsigned char tmp;

    tmp = switch_slide_sense();

    if ((sw_state & SW1) != (tmp & SW1)) {
        SVC_PERROR(ipsnd_dtq(SW_DTQ, ((tmp & SW1) == SW1) ? SW1_ON : SW1_OFF));
    }
    if ((sw_state & SW2) != (tmp & SW2)) {
        SVC_PERROR(ipsnd_dtq(SW_DTQ, ((tmp & SW2) == SW2) ? SW2_ON : SW2_OFF));
    }
    if ((sw_state & SW3) != (tmp & SW3)) {
        SVC_PERROR(ipsnd_dtq(SW_DTQ, ((tmp & SW3) == SW3) ? SW3_ON : SW3_OFF));
    }
    if ((sw_state & SW4) != (tmp & SW4)) {
        SVC_PERROR(ipsnd_dtq(SW_DTQ, ((tmp & SW4) == SW4) ? SW4_ON : SW4_OFF));
    }

    sw_state = tmp;
}
