/*
 *  1タスクによるLED点滅プログラム
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include "device.h"
#include "led_1task.h"

#define DELAY_LOOP 0x800000

/*
 * ビジーループ関数
 */
void
busy_wait(void){
    volatile int i;
    for(i = 0; i < DELAY_LOOP; i++);
}

/*
 * LED1点滅タスク
 */
void
led1_task(intptr_t exinf){
    led_init(0);

    for (;;) {
        led_out(LED1);
        busy_wait();
        led_out(0x00);
        busy_wait();
    }
}
