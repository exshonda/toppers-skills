#define DEFAULT_PRIORITY    8
#define STACK_SIZE          4096

#define LED3_INTERVAL    500    /* LED3 周期ハンドラ起動周期 [ms] */

#ifndef TOPPERS_MACRO_ONLY
extern void led_init(intptr_t exinf);
extern void led1_task(intptr_t exinf);
extern void led3_cyc_handler(intptr_t exinf);
#endif /* TOPPERS_MACRO_ONLY */
