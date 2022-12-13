/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include <cstring>

#include "FreeRTOS.h"
#include "heap_lock_monitor.h"
#include "task.h"
#include "queue.h"
#include "DigitalIoPin.h"
#include "ITM_write.h"
#include "Fmutex.h"
#include "LpcUart.h"
#include <cstring>
#include "modbusConfig.h"

#if 1

static void prvHardwareSetup(void) {
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);
	Board_LED_Set(2, true);
	/* Initialize interrupt hardware */
	DigitalIoPin::GPIO_interrupt_init();
	ITM_init();
	ITM_write("ITM ok!\n");
}

Fmutex sysMutex;
QueueHandle_t hq;
SemaphoreHandle_t xSem;
modbusConfig modbus;

/* variables to read from MODBUS sensors */
static int temp = 0;
static int rh = 0;
static int co2  = 0;

struct BtnEvent {

};
/* Interrupt handlers must be wrapped with extern "C" */

extern "C"{
/* ISR for encode rotator A */
void PIN_INT0_IRQHandler(void)
{
	Board_LED_Toggle(2);
	/* this must be set to true so that the context switch
	 * gets back to the ongoing task
	 * */
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(0));
	/* create an event and send to the queue upon an interrupt */

	/* switch back to the previous context */
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
#if 0
/* ISR for encode rotator B */
void PIN_INT1_IRQHandler(void){
	portBASE_TYPE xHigherPriorityTaskWoken = pdTRUE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(1));
	BtnEvent e { 2, xTaskGetTickCountFromISR() };
	xQueueSendFromISR(hq, &e, &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
#endif
/* ISR for button 3 */
void PIN_INT2_IRQHandler(void){
	Board_LED_Toggle(2);
	portBASE_TYPE xHigherPriorityTaskWoken = pdTRUE;
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH(2));
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

/* enable runtime statistics */
void vConfigureTimerForRunTimeStats( void ) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
/* end runtime statictics collection */
}


/* @brief task controls MQTT interface */
static void vTaskMQTT(void *pvParameters){
	//implementation
}

/* task to wait on the queue event from ISR and print it */
static void vTaskLCD(void *pvParams){

	// 1. display values to LCD UI
	for( ;; ){
		// 2. take semaphore and update
	}
}

static void vTaskMODBUS(void *pvParams){

	char buff[20];

	while(1)  {
		temp = modbus.get_temp();
		rh = modbus.get_rh();
		co2 = modbus.get_co2();

		sprintf(buff, "\n\rtemp: %d\n\rrh: %d\n\rco2: %d", temp, rh, co2);
		sysMutex.lock();
		ITM_write(buff);
		sysMutex.unlock();
		//give semaphore to LCD task here

		vTaskDelay(500); //this is not required if semaphore is used
	}
}


int main(void) {
	prvHardwareSetup();
	heap_monitor_setup();

	/* UART port config */
	LpcPinMap none = {-1, -1}; // unused pin has negative values in it
	LpcPinMap txpin = { 0, 18 }; // transmit pin that goes to debugger's UART->USB converter
	LpcPinMap rxpin = { 0, 13 }; // receive pin that goes to debugger's UART->USB converter
	LpcUartConfig cfg = { LPC_USART0, 115200, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1, false, txpin, rxpin, none, none };

	LpcUart *dbgu = new LpcUart(cfg);

	/* Configure pins and ports for Rotary Encoder as input, pullup, and inverted */
	DigitalIoPin encoder_A(0, 5, DigitalIoPin::pullup, true);
	DigitalIoPin encoder_B(0, 6, DigitalIoPin::pullup, true);
	DigitalIoPin encoder_Button(1, 8, DigitalIoPin::pullup, true);

	DigitalIoPin solenoid_valve(0, 27, DigitalIoPin::pullup, true);

	/* configure interrupts for those buttons */
	encoder_A.enable_interrupt(0, 0, 0, 5);
	encoder_Button.enable_interrupt(2, 0, 1, 9);

	/* create a counting sempahore of max 5 events */
	xSem = xSemaphoreCreateCounting(10, 0);

	/* create a queue of max 10 events */
	hq = xQueueCreate(10, sizeof(int));

	/* task MQTT */


	/* task LCD */


	/* task co2 monitor */


	/* task measurement modbus */
	xTaskCreate(vTaskMODBUS, "Measuring",
			((configMINIMAL_STACK_SIZE)+128), NULL, tskIDLE_PRIORITY + 3UL,
						(TaskHandle_t *) NULL);

	vTaskStartScheduler();

	/* never arrive here */
    return 1 ;
}
#endif /* task 1 ends */
