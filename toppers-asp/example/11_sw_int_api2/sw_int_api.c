/*
 *  Call API from Interrupt Handler (Wakeup)
 *  PUSH1 で LED1_TASK を起床（slp_tsk 中のタスクを iwup_tsk で起こす）
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include <t_stdlib.h>
#include "device.h"
#include "sw_int_api.h"

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)    svc_perror(__FILE__, __LINE__, #expr, (expr))


unsigned int led_data = 0;

#define LED1_INTERVAL 500

/*
 * LED1点滅タスク
 * slp_tsk → 起床されたら4回点滅 → slp_tsk のループ
 */
void
led1_task(intptr_t exinf){
    int i;

    for (;;) {
        syslog(LOG_NOTICE, "led1_task : sleep!");
        slp_tsk();                                 /* 起床待ち */
        syslog(LOG_NOTICE, "led1_task : wakeup!");

        for (i = 0; i < 4; i++) {
            if ((led_data & LED1) == LED1) {
                led_data &= ~LED1;
            } else {
                led_data |= LED1;
            }
            led_out(led_data);
            dly_tsk(LED1_INTERVAL);
        }
    }
}

/*
 * EXTI9_5(PUSH1) 割込みハンドラ
 */
void
push1_handler(void){
    push1_int_clear();
    syslog(LOG_NOTICE, "push1_handler : start!");
    SVC_PERROR(iwup_tsk(LED1_TASK));
}
