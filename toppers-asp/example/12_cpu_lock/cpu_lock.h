#define DEFAULT_PRIORITY    8
#define STACK_SIZE          4096

#define PUSH1_INT_LVL   -5

#ifndef TOPPERS_MACRO_ONLY
extern void switch_task(intptr_t exinf);
extern void switch_slide_init(intptr_t exinf);
extern void push1_int_init(intptr_t exinf);
extern void push1_handler(void);
#endif /* TOPPERS_MACRO_ONLY */
