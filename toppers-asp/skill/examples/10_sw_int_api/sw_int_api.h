#define DEFAULT_PRIORITY    8
#define STACK_SIZE          4096

#define PUSH1_INT_LVL   -5      /* PUSH1 割込み優先度 */

#ifndef TOPPERS_MACRO_ONLY
extern void led1_task(intptr_t exinf);
extern void led_init(intptr_t exinf);
extern void push1_int_init(intptr_t exinf);
extern void push1_handler(void);
#endif /* TOPPERS_MACRO_ONLY */
