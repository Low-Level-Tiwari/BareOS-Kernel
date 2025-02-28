#include "task.h"
#include "hardware/clocks.h"
#include "hardware/exception.h"

extern uint32_t debug;
#define portDISABLE_INTERRUPTS()            __asm volatile ( " cpsid i " ::: "memory" )
#define portENABLE_INTERRUPTS()             __asm volatile ( " cpsie i " ::: "memory" )
#define portNVIC_SYSTICK_CTRL_REG             ( *( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG             ( *( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG    ( *( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_INT_CTRL_REG                 ( *( ( volatile uint32_t * ) 0xe000ed04 ) )
#define portNVIC_SHPR3_REG                    ( *( ( volatile uint32_t * ) 0xe000ed20 ) )
#define portNVIC_SYSTICK_CLK_BIT              ( 1UL << 2UL )
#define portNVIC_SYSTICK_INT_BIT              ( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT           ( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT       ( 1UL << 16UL )
#define portNVIC_PENDSVSET_BIT                ( 1UL << 28UL )
#define portMIN_INTERRUPT_PRIORITY            ( 255UL )
#define portNVIC_PENDSV_PRI                   ( portMIN_INTERRUPT_PRIORITY << 16UL )
#define portNVIC_SYSTICK_PRI                  ( portMIN_INTERRUPT_PRIORITY << 24UL )
#define configTICK_RATE_HZ  		      ( ( uint32_t ) 1000 )
#define portINITIAL_XPSR                      ( 0x01000000 )
#define portMAX_24_BIT_NUMBER                 ( 0xffffffUL )
void vPortSetupTimerInterrupt( void );
void xPortPendSVHandler( void ) __attribute__( ( naked ) );
void xPortSysTickHandler( void );
void vPortSVCHandler( void );
static void vPortStartFirstTask( void ) __attribute__( ( naked ) );
static void prvTaskExitError( void );
static uint32_t uxCriticalNesting;
#define INVALID_PRIMARY_CORE_NUM    0xffu

StackType_t * initStack( StackType_t * pxTopOfStack, TaskFunction_t pxCode, void * pvParameters ) {
    pxTopOfStack--;                                          
    *pxTopOfStack = portINITIAL_XPSR;                        /* xPSR */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) pxCode;                  /* PC */
    pxTopOfStack--;
    *pxTopOfStack = ( StackType_t ) prvTaskExitError; 	     /* LR */
    pxTopOfStack -= 5;                                       /* R12, R3, R2 and R1. */
    *pxTopOfStack = ( StackType_t ) pvParameters;            /* R0 */
    pxTopOfStack -= 8;                                       /* R11..R4. */

    return pxTopOfStack;
}

static void prvTaskExitError( void ) {
	vTaskDelete(NULL);
}

void vPortSVCHandler( void ) {
}

void vPortStartFirstTask( void ) {
         __asm volatile (
             "   .syntax unified             \n"
             "   ldr  r2, pxCurrentTCBConst1 \n" /* Obtain location of pxCurrentTCB. */
             "   ldr  r3, [r2]               \n"
             "   ldr  r0, [r3]               \n" /* The first item in pxCurrentTCB is the task top of stack. */
             "   adds r0, #32                \n" /* Discard everything up to r0. */
             "   msr  psp, r0                \n" /* This is now the new top of stack to use in the task. */
             "   movs r0, #2                 \n" /* Switch to the psp stack. */
             "   msr  CONTROL, r0            \n"
             "   isb                         \n"
             "   pop  {r0-r5}                \n" /* Pop the registers that are saved automatically. */
             "   mov  lr, r5                 \n" /* lr is now in r5. */
             "   pop  {r3}                   \n" /* Return address is now in r3. */
             "   pop  {r2}                   \n" /* Pop and discard XPSR. */
             "   cpsie i                     \n" /* The first task has its context and interrupts can be enabled. */
             "   bx   r3                     \n" /* Finally, jump to the user defined task code. */
             "   .align 4                       \n"
             "pxCurrentTCBConst1: .word pxCurrentTCB\n"
             );
}

uint32_t xPortStartScheduler( void ) {
        portNVIC_SHPR3_REG |= portNVIC_PENDSV_PRI;
        portNVIC_SHPR3_REG |= portNVIC_SYSTICK_PRI;

        exception_set_exclusive_handler( PENDSV_EXCEPTION, xPortPendSVHandler );
        exception_set_exclusive_handler( SYSTICK_EXCEPTION, xPortSysTickHandler );
        exception_set_exclusive_handler( SVCALL_EXCEPTION, vPortSVCHandler );
        
	init_systick();
        vPortStartFirstTask();
        vTaskSwitchContext();
        prvTaskExitError();
        return 0;
    }

void vPortYield( void ) {
    portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
    __asm volatile ( "dsb" ::: "memory" );
    __asm volatile ( "isb" );
}

void vPortEnterCritical( void ) {
        portDISABLE_INTERRUPTS();
        uxCriticalNesting++;
        __asm volatile ( "dsb" ::: "memory" );
        __asm volatile ( "isb" );
}
    
void vPortExitCritical( void ) {
        uxCriticalNesting--;

        if( uxCriticalNesting == 0 )
        {
            portENABLE_INTERRUPTS();
        }
}

uint32_t ulSetInterruptMaskFromISR( void ) {
    __asm volatile (
        " mrs r0, PRIMASK    \n"
        " cpsid i            \n"
        " bx lr                "
        ::: "memory"
        );
}

void vClearInterruptMaskFromISR( __attribute__( ( unused ) ) uint32_t ulMask ) {
    __asm volatile (
        " msr PRIMASK, r0    \n"
        " bx lr                "
        ::: "memory"
        );
}

void vYieldCore( int xCoreID ) {
    ( void ) xCoreID;

}


void xPortPendSVHandler( void ) {
        __asm volatile
        (
            "   .syntax unified                     \n"
            "   mrs r0, psp                         \n"
            "                                       \n"
            "   ldr r3, pxCurrentTCBConst2          \n" /* Get the location of the current TCB. */
            "   ldr r2, [r3]                        \n"
            "                                       \n"
            "   subs r0, r0, #32                    \n" /* Make space for the remaining low registers. */
            "   str r0, [r2]                        \n" /* Save the new top of stack. */
            "   stmia r0!, {r4-r7}                  \n" /* Store the low registers that are not saved automatically. */
            "   mov r4, r8                          \n" /* Store the high registers. */
            "   mov r5, r9                          \n"
            "   mov r6, r10                         \n"
            "   mov r7, r11                         \n"
            "   stmia r0!, {r4-r7}                  \n"
            "   push {r3, r14}                      \n"
            "   cpsid i                             \n"
            "   bl vTaskSwitchContext               \n"
            "   cpsie i                             \n"
            "   pop {r2, r3}                        \n" /* lr goes in r3. r2 now holds tcb pointer. */
            "                                       \n"
            "   ldr r1, [r2]                        \n"
            "   ldr r0, [r1]                        \n" /* The first item in pxCurrentTCB is the task top of stack. */
            "   adds r0, r0, #16                    \n" /* Move to the high registers. */
            "   ldmia r0!, {r4-r7}                  \n" /* Pop the high registers. */
            "   mov r8, r4                          \n"
            "   mov r9, r5                          \n"
            "   mov r10, r6                         \n"
            "   mov r11, r7                         \n"
            "                                       \n"
            "   msr psp, r0                         \n" /* Remember the new top of stack for the task. */
            "                                       \n"
                "   subs r0, r0, #32                    \n" /* Go back for the low registers that are not automatically restored. */
            "   ldmia r0!, {r4-r7}                  \n"     /* Pop low registers.  */
            "                                       \n"
            "   bx r3                               \n"
            "   .align 4                            \n"
            "pxCurrentTCBConst2: .word pxCurrentTCB \n"
        );
}

void xPortSysTickHandler( void ){
    vPortEnterCritical();
          if( xTaskIncrementTick() != 0){
              portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
          }
    vPortExitCritical( );
}
__attribute__( ( weak ) ) void init_systick( void ){
    portNVIC_SYSTICK_CTRL_REG = 0UL;
    portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
    portNVIC_SYSTICK_LOAD_REG = ( clock_get_hz( clk_sys ) / configTICK_RATE_HZ ) - 1UL;
    portNVIC_SYSTICK_CTRL_REG = portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT;
}
