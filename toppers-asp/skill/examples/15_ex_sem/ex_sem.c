/*
 *  2 Tasks Output Program WITH Mutual Exclusion (Semaphore)
 *  共有変数 count の更新を SEM1 セマフォで排他制御
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include <t_stdlib.h>
#include "device.h"
#include "ex_sem.h"

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)    svc_perror(__FILE__, __LINE__, #expr, (expr))


#define DELAY_LOOP 0x40000L

int
busy_wait_inc(int e){
    volatile int i;
    e++;
    for(i = 0; i < DELAY_LOOP; i++);
    return e;
}

int count = 0;

/*
 * 出力タスク（セマフォによる排他制御あり）
 */
void
task(intptr_t exinf){
    int local_count = 0;

    for (;;) {
        wai_sem(SEM1);                            /* セマフォ獲得 */
        count = busy_wait_inc(count);             /* クリティカルセクション */
        sig_sem(SEM1);                            /* セマフォ返却 */
        local_count = busy_wait_inc(local_count); /* 排他外 */
        syslog(LOG_NOTICE, "Task %d : count = %d, local_count = %d",
               exinf, count, local_count);
    }
}

void
rot_cyc_handler(intptr_t exinf){
    SVC_PERROR(irot_rdq(DEFAULT_PRIORITY));
}
