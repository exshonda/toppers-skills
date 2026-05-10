/*
 *  1タスクによるLED点滅プログラム + syslogデバッグ
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "led_1task.h"

#define DELAY_LOOP 0x800000

void
busy_wait(void){
    volatile int i;
    for(i = 0; i < DELAY_LOOP; i++);
}

/*
 * LED1点滅タスク（点灯・消灯のたびに syslog 出力）
 */
void
led1_task(intptr_t exinf){
    led_init(0);

    for (;;) {
        syslog(LOG_NOTICE, "led1_task : out_put 0x%x", LED1);
        led_out(LED1);
        busy_wait();
        syslog(LOG_NOTICE, "led1_task : out_put 0x%x", 0x00);
        led_out(0x00);
        busy_wait();
    }
}
