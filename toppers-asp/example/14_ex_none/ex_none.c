/*
 *  2 Tasks Output Program WITHOUT Mutual Exclusion
 *  共有変数 count に競合状態が発生する（意図的）
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include <t_stdlib.h>
#include "device.h"
#include "ex_none.h"

Inline void
svc_perror(const char *file, int_t line, const char *expr, ER ercd)
{
    if (ercd < 0) {
        t_perror(LOG_ERROR, file, line, expr, ercd);
    }
}

#define SVC_PERROR(expr)    svc_perror(__FILE__, __LINE__, #expr, (expr))


#define DELAY_LOOP 0x40000L

/*
 * 時間のかかるインクリメント関数（競合を顕在化させるため）
 */
int
busy_wait_inc(int e){
    volatile int i;
    e++;
    for(i = 0; i < DELAY_LOOP; i++);
    return e;
}

/* 共有変数 */
int count = 0;

/*
 * 出力タスク（2タスクで共有）
 * exinf でタスクID（1 or 2）を識別
 */
void
task(intptr_t exinf){
    int local_count = 0;

    for (;;) {
        count = busy_wait_inc(count);              /* 共有変数（競合あり） */
        local_count = busy_wait_inc(local_count);  /* ローカル変数（競合なし） */
        syslog(LOG_NOTICE, "Task %d : count = %d, local_count = %d",
               exinf, count, local_count);
    }
}

/*
 * 同一優先度のタスクをラウンドロビン的に切り替えるための周期ハンドラ
 */
void
rot_cyc_handler(intptr_t exinf){
    SVC_PERROR(irot_rdq(DEFAULT_PRIORITY));
}
