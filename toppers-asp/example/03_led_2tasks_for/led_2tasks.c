/*
 *  2タスクによるLED点滅プログラム(forループ版)
 *  注意: 片方のLEDしか点滅しない（FCFSスケジューリングの結果）
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "led_2tasks.h"

#define DELAY_LOOP 0x800000

void
busy_wait(void){
    volatile int i;
    for(i = 0; i < DELAY_LOOP; i++);
}

unsigned int led_data = 0;     /* 共有変数（2タスクが書き換える） */

/*
 * LED1点滅タスク
 */
void
led1_task(intptr_t exinf){
    led_init(0);
    for (;;) {
        led_data |= LED1;
        led_out(led_data);
        busy_wait();

        led_data &= ~LED1;
        led_out(led_data);
        busy_wait();
    }
}

/*
 * LED3点滅タスク（このタスクは実行されない）
 */
void
led3_task(intptr_t exinf){
    for (;;) {
        led_data |= LED3;
        led_out(led_data);
        busy_wait();

        led_data &= ~LED3;
        led_out(led_data);
        busy_wait();
    }
}
