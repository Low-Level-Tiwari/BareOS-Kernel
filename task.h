#include<stdint.h>
#include<stdio.h>

#define STACK_SIZE 512

typedef void (*TaskFunction_t)(void*);
typedef  uint32_t StackType_t;
typedef struct{
	uint32_t* sp;
	TaskFunction_t fp;
	uint32_t stack[STACK_SIZE];
}Task;

void xTaskCreate(TaskFunction_t fp,void* params);
void vTaskSwitchContext( void ) ;
void vTaskDelay( const uint32_t xTicksToDelay );
void vTaskDelete( void* xTaskToDelete ) ;
void vTaskStartScheduler( void ) ;
void init_systick( void );
uint32_t  xTaskIncrementTick( void ) ;
uint32_t xPortStartScheduler( void ) ;
StackType_t * initStack( StackType_t * pxTopOfStack, TaskFunction_t pxCode, void * pvParameters ) ;
