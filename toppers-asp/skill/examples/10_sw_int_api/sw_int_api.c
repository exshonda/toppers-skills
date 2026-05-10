/*
 *  Call API from Interrupt Handler (Task Activation)
 *  PUSH1 で LED1_TASK を起動。タスクは4回点滅して終了。
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include <t_stdlib.h>
#include "device.h"
#include "sw_int_api.h"

/*
 *  サービスコールのエラーログ出力
 */
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
 * 起動されると4回点滅して終了（return で自動的に休止状態へ）
 */
void
led1_task(intptr_t exinf){
    int i;

    syslog(LOG_NOTICE, "led1_task : start!");

    for (i = 0; i < 4; i++) {
        if ((led_data & LED1) == LED1) {
            led_data &= ~LED1;
        } else {
            led_data |= LED1;
        }
        led_out(led_data);
        dly_tsk(LED1_INTERVAL);
    }

    syslog(LOG_NOTICE, "led1_task : done!");
    /* return すると ext_tsk() 同等の動作で休止状態へ戻る */
}

/*
 * EXTI9_5(PUSH1) 割込みハンドラ
 * iact_tsk で LED1_TASK を起動
 */
void
push1_handler(void){
    push1_int_clear();
    syslog(LOG_NOTICE, "push1_handler : start!");
    SVC_PERROR(iact_tsk(LED1_TASK));
}
