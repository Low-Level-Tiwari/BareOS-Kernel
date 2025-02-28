#include"task.h" 
#include"pico/stdlib.h"

#define TASK_LIMIT 3
Task TaskList[3];
__attribute__( ( used ) ) Task * volatile pxCurrentTCB = NULL;
uint32_t task_count=0,current_task=0;
uint32_t tick=0,debug=0;
void xTaskCreate(TaskFunction_t fp,void *params){
		TaskList[task_count].fp = fp;
		TaskList[task_count].sp = (uint32_t*)(TaskList[task_count].stack + STACK_SIZE);	
		TaskList[task_count].sp =(uint32_t*)(((uintptr_t)(TaskList[task_count].sp)) & (~0x7)); 
		TaskList[task_count].sp = initStack(TaskList[task_count].sp,fp,params); 
		task_count++;
}
void vTaskSwitchContext( void ) {
 	current_task = (current_task+1)%TASK_LIMIT;
 	pxCurrentTCB = &TaskList[current_task];
}
void vTaskDelay( const uint32_t xTicksToDelay ){
	sleep_ms(xTicksToDelay);
}
void vTaskDelete( void* xTaskToDelete ) {
	while(1);
}
void vTaskStartScheduler( void ) {
	pxCurrentTCB = &TaskList[current_task];
 	__asm volatile ( " cpsid i " ::: "memory" );
	xPortStartScheduler();
}

uint32_t xTaskIncrementTick( void ) {
  	tick += 1;
  	if(tick==1000){
  		tick =0;
		return 1;
  	}else{
  		return 0;
  	}
}
