#define DEFAULT_PRIORITY    8
#define STACK_SIZE          4096

#define SW_SCAN_INTERVAL    10  /* スイッチスキャン周期 [ms] */

#ifndef TOPPERS_MACRO_ONLY
extern void led_init(intptr_t exinf);
extern void switch_slide_init(intptr_t exinf);
extern void led_task(intptr_t exinf);
extern void sw_cyc_handler(intptr_t exinf);
extern void state_init(intptr_t exinf);
#endif /* TOPPERS_MACRO_ONLY */
