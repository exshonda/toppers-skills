#define DEFAULT_PRIORITY    8
#define STACK_SIZE          4096

#define ROT_INTERVAL        1   /* タスク切替周期 [ms] */

#ifndef TOPPERS_MACRO_ONLY
extern void task(intptr_t exinf);
extern void rot_cyc_handler(intptr_t exinf);
#endif /* TOPPERS_MACRO_ONLY */
