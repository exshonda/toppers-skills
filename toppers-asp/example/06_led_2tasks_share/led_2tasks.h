#define DEFAULT_PRIORITY    8
#define STACK_SIZE          4096

#ifndef TOPPERS_MACRO_ONLY
extern void led_init(intptr_t exinf);
extern void led_task(intptr_t exinf);
#endif /* TOPPERS_MACRO_ONLY */
