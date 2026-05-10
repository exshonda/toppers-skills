/*
 *  CPU Lock Program
 *  SW1 OFF→ON で loc_cpu(), ON→OFF で unl_cpu()
 *  CPUロック中は PUSH1割込みハンドラが実行されない
 */
#include <kernel.h>
#include "kernel_cfg.h"
#include <t_syslog.h>
#include "device.h"
#include "cpu_lock.h"

/*
 * スライドスイッチ監視タスク
 * CPUロック中は syslog 等のAPIを呼ばないように注意
 */
void
switch_task(intptr_t exinf){
    unsigned char data;
    unsigned char pdata;

    pdata = switch_slide_sense();

    for(;;){
        data = switch_slide_sense();
        if (((data & SW1) == SW1) && (pdata & SW1) != SW1) {
            loc_cpu();              /* SW1 OFF→ON: CPUロックへ */
        }
        if (((data & SW1) != SW1) && (pdata & SW1) == SW1) {
            unl_cpu();              /* SW1 ON→OFF: CPUロック解除 */
        }
        pdata = data;
    }
}

/*
 * EXTI9_5(PUSH1) 割込みハンドラ
 * CPUロック中は実行されない（CPUロック解除後にまとめて処理）
 */
void
push1_handler(void){
    push1_int_clear();
    syslog(LOG_NOTICE, "push1_int : start!");
}
