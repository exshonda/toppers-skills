/*
 *  2タスクによるLED点滅プログラム(コード共有版)
 *  exinf でタスクごとの情報（LEDビット・周期）を切り替える
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "led_2tasks.h"

unsigned int led_data = 0;

#define LED1_INTERVAL   1000
#define LED3_INTERVAL    250

typedef struct {
    unsigned int  led_bit;
    int           interval;
} LED_TASK_INFO;

/* exinf をインデックスとして参照 */
LED_TASK_INFO task_info[] = {
    { LED1, LED1_INTERVAL },     /* exinf = 0 */
    { LED3, LED3_INTERVAL }      /* exinf = 1 */
};

/*
 * LED点滅タスク（共有コード）
 */
void
led_task(intptr_t exinf){
    unsigned int led_bit  = task_info[(int)exinf].led_bit;
    int          interval = task_info[(int)exinf].interval;

    syslog(LOG_NOTICE, "led_task : exinf = %d, led_bit = 0x%x, interval = %d msec",
           exinf, led_bit, interval);

    for (;;) {
        led_data |= led_bit;
        led_out(led_data);
        dly_tsk(interval);

        led_data &= ~led_bit;
        led_out(led_data);
        dly_tsk(interval);
    }
}
