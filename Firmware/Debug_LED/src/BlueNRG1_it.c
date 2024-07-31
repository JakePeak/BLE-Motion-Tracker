
/* Includes----------------------------------------------------------------------*/
#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "SDK_EVAL_Com.h"
#include "clock.h"
#include "main.h"
/******************************************************************************/
/********************Cortex-M0 Processor Exceptions Handlers********************/
/******************************************************************************/


/******************************************************************************
 * Function Name  : nmi_handler.
 * Description    : This function handles NMI exception.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void NMI_Handler(void)
{
}


/******************************************************************************
 * Function Name  : hardfault_handler.
 * Description    : This function handles Hard Fault exception.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {}
}


/******************************************************************************
 * Function Name  : svc_handler.
 * Description    : This function handles SVCall exception.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void SVC_Handler(void)
{
}


/******************************************************************************
 * Function Name  : pendsv_handler.
 * Description    : This function handles PendSV_Handler exception.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void PendSV_Handler(void)
{
}


/******************************************************************************
 * Function Name  : systick_handler.
 * Description    : This function handles SysTick Handler.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void SysTick_Handler(void)
{
  SysCount_Handler();
}


/******************************************************************************
 * Function Name  : gpio_handler.
 * Description    : This function handles GPIO Handler.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void GPIO_Handler(void)
{
}
/******************************************************************************
*                 BlueNRG1LP Peripherals Interrupt Handlers                   *
*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the   *
*  available peripheral interrupt handler/'s name please refer to the startup *
* file (startup_BlueNRG1lp.s).
******************************************************************************/


/******************************************************************************
 * Function Name  : uart_handler.
 * Description    : This function handles UART interrupt request.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void UART_Handler(void)
{
}


/******************************************************************************
 * Function Name  : blue_handler.
 * Description    : This function handles Blue Handlers.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
void Blue_Handler(void)
{
//  Call RAL_Isr
   RAL_Isr();
}


extern uint8_t packet_data[60];
extern uint8_t packet_counter;

void SPI_Handler(void)
{
	Error(1);
		// This means received FIFO buffer is full, so load data to packet state
		Process_Packet(packet_data+12*packet_counter);
		packet_counter++;

	    // If packet is full, send it
	    if(packet_counter == 5){
	    	Error(0);
	    	packet_counter = 0;
	    }
}
