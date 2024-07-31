/******************** (C) COPYRIGHT 2018 STMicroelectronics ********************
* File Name          : SensorDemo_main.c
* Author             : AMS - RF Application team
* Version            : V1.2.0
* Date               : 03-December-2018
* Description        : BlueNRG-1-2 Sensor Demo main file 
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
/** @addtogroup BlueNRG1_demonstrations_applications
 * BlueNRG-1,2 SensorDemo \see SensorDemo_main.c for documentation.
 *
 *@{
 */

/** @} */
/** \cond DOXYGEN_SHOULD_SKIP_THIS
 */
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h" 
#include "bluenrg1_stack.h"
#include "SDK_EVAL_Config.h"
#include "sleep.h"
#include "sensor.h"
#include "SensorDemo_config.h"
#include "OTA_btl.h"  
#include "gatt_db.h"
#include "BLE_User_main.h"
#include "gp_timer.h"

#include "../WiSE-Studio/BlueNRG-2/src/SPI_User.h"
#include "../WiSE-Studio/BlueNRG-2/src/user.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define BLE_SENSOR_VERSION_STRING "1.0.0" 
#define HS_SPEED_XTAL				HS_SPEED_XTAL_32MHZ
#define LS_SOURCE					LS_SOURCE_EXTERNAL_32KHZ
#define SMPS_INDUCTOR				SMPS_INDUCTOR_10uH

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
   
/* Set the Application Service Max number of attributes records with init parameters coming from application *.config.h file */
uint8_t Services_Max_Attribute_Records[NUMBER_OF_APPLICATION_SERVICES] = {MAX_NUMBER_ATTRIBUTES_RECORDS_SERVICE_1, MAX_NUMBER_ATTRIBUTES_RECORDS_SERVICE_2};

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/*
 * Global variables are altered here, in BlueNRH1_it.c, and bluenrg1_events.c to
 * control the state of the function based on BLE events and interrupts
 */

/*
 * Global device state variable
 * 0x0x = Unconnected state, search for device to connect to
 * 0x1x = Operational state, active IMU and data transfer
 * 0x2x = Paused state, connected but not actively transferring data
 *
 * substates are defined by the x, where x = 0 is the base state
 */
uint8_t device_state;

/*
 * Contains connection information about host
 */
connection_info conn_info;

/*
 * contains all GATT handle information
 */
GATT_Handles accel_gatt_handles;

/*
 * Data packet build to send over set interval
 */
uint8_t packet_data[12*POINTS_PER_PACKET];
uint8_t packet_counter;
uint8_t calibrating;

int main(void) 
{
  int ret, averageCounter = 0;
  long long int averageData[12] = {};
  uint8_t temp_read;
  struct timer Timer;
  packet_counter = 0;
  tBleStatus commandStatus = 0;
  SystemInit();
  SysCtrl_PeripheralClockCmd(CLOCK_PERIPH_GPIO | CLOCK_PERIPH_PKA | CLOCK_PERIPH_SPI | CLOCK_PERIPH_ADC, ENABLE);

  /* Initialize the clock for time delays*/
  Clock_Init();

  /* Configure SPI & error LED*/
  SPI_User_Init();

  /* Configure ADC */
  ADC_User_Init();

  /* BlueNRG-1 stack init */
  ret = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);

  // Initialization success!
  Error(0);

  if (ret != BLE_STATUS_SUCCESS) {
	  // Error function flashes LED a number of times according to the corresponding error code
	  Error(1);
  }

  //device_initialization (codes 2-7 & 24-25)
  Error(device_initialization(&commandStatus));

  // GATT Construction (codes 8-10 & 23)
  Error(device_GATT_profile(&commandStatus));
  if(commandStatus != 0){
	  Clock_Wait(1000);
	  Error(commandStatus);
  }

  // Use SPI to initialize all accelerometer settings
  Accel_Init();
  Error(0);

  // Initialize device state
  device_state = 0x01;

  while(1) {
	// State machine
	switch(device_state){
	/*
	 * Unattached State, device slowly makes a connection to a master
	 */
	  // Begin the connection setup process (code 11)
	  case 0x01:
		Error(set_device_discoverable());
		device_state++;
		break;

	  // Only allow connection requests to be filtered out w/ master instructions (code 12)
	  case 0x02:
		if(aci_hal_set_radio_activity_mask(0x0034) == BLE_STATUS_SUCCESS){
			device_state = 0x00;
			Error(0); /*Successful transition into state 0*/}
		else {Error(12);}
		break;

	  // Heart of unattached, waiting for a connection request
	  case 0x00:
		;
		break;

	  // Stop trying to make a connection to a master (code 13), removed feature
	  case 0x0C:
		//if(aci_gap_set_non_discoverable() == BLE_STATUS_SUCCESS){
			device_state++;//}
   		  //else {Error(13);}
		break;

	  // Filter radio input to just instructions from the master (code 14)
	  case 0x0D:
	    if(aci_hal_set_radio_activity_mask(0x0024) == BLE_STATUS_SUCCESS){
	    	device_state++;}
		else {Error(14);}
	    break;

	  // Allow use without authorization (code 15)
	  case 0x0E:
		if(aci_gap_set_authorization_requirement(conn_info.connection_handle, 0x00) == BLE_STATUS_SUCCESS){
			device_state++;}
		else {Error(15);}
		break;

	  // Wait for master to write ATT notification descriptor to Client Characteristic
	  // configuration Descriptor
	  case 0x0F:
		  // Waiting for aci_gatt_attribute_modified_event
		  ;
		  // Upon event, transfer to pause state
		break;


	/*
	 * Active State, device is fully operational and transmitting/receiving data
	 */

	  // Enable clock/SPI interrupts, accelerometer, and clear the buffer (transition into active)
	  case 0x11:
		// Mark gatt state characteristic correctly
		temp_read = 0x00;
		aci_gatt_update_char_value_ext(0, accel_gatt_handles.service_handle, accel_gatt_handles.char_desc_handle, 0, 1, 0, 1, &temp_read);

		// Mark as needing callibration to start
		calibrating = 0x00;
		packet_counter = 0;

		//Take accelerometer out of sleep mode
		Enable_Accel(ENABLE);

		// Clearing local FIFO buffer
		SPI_ClearRXFIFO();

		// Enable SPI Interrupts
		SPI_ITConfig(SPI_IT_RX, ENABLE);

		// Start timer
		Timer_Set(&Timer, SAMPLE_INTERVAL);

		// Enter main active state
		device_state--;

		// Successful transition to state 1
		Error(0);
		break;

	  // Primary active state, waiting for various interrupts + timer
	  case 0x10:
		/*
		 * Timer Expired => if 10 ms have passed, request a packet from IMU
		 * SPI_Handler => if SPI receive FIFO has a packet, parse and prepare/send it (code 1-2)
		 * BLE sleep mode => if user requested sleep mode, head to 0x1D
		 * BLE connection lost => if device disconnected, head to 0x1F
		 */
		if(Timer_Expired(&Timer)){
			Timer_Reset(&Timer);
			Average_Accel(averageData, &averageCounter, packet_data + 12*packet_counter);
			Manual_Accel_Data();
		}
		else{
			Read_Accel(averageData, &averageCounter);
		}
		break;

	  // Notifications Unsubscribed
	  case 0x1B:
		exit_active_state();

		// Change state
		device_state = 0x0F;
		break;

	  // Return to pause mode command occurred
	  case 0x1D:
	    exit_active_state();
	    // Transfer over to pause
	    device_state = 0x21;
	    break;

	  // Bluetooth connection lost
	  case 0x1F:
		exit_active_state();

		// Terminate connection after waiting necessary delay (code 16)
		Clock_Wait(100);
		if(aci_gap_terminate(conn_info.connection_handle, 0x15) != BLE_STATUS_SUCCESS){
			Error(16);
		}
		conn_info.connection_handle = 0x0000;

		device_state = 0x01;
		break;
	/*
	 * Pause state, device is connected, but master has not initiated data collection
	 */
	  // Entering pause state, turn off the Error LED and other non-necessary GPIOs if on
	  case 0x21:
		GPIO_ResetBits(ERROR_LED);
		device_state = 0x20;
		// Mark gatt state characteristic correctly
		temp_read = 0x01;
		aci_gatt_update_char_value_ext(0, accel_gatt_handles.service_handle, accel_gatt_handles.char_desc_handle, 0, 1, 0, 1, &temp_read);
		// Successful transition to state 2
		Error(0);
		break;
	  // General sleep state, just loop and wait for bluetooth command
	  case 0x20:
		;
	    break;
	  // Bluetooth disconnection state
	  case 0x2F:
		// Wait appropriate time and then disconnect from device (code 17)
		Clock_Wait(100);
		if(aci_gap_terminate(conn_info.connection_handle, 0x15) != BLE_STATUS_SUCCESS){
			Error(17);
		}
		device_state = 0x01;
		break;

	  /*
	   * Entered incorrect state (device broke)
	   */
	  case 0xFF:
		GPIO_ToggleBits(ERROR_LED);
		Clock_Wait(1000);
		break;
	  default:
		//Oh No! shut down device as politely as possible
		device_state = 0xFF;
		break;
	}
    /* BlueNRG-1 stack tick */
    BTLE_StackTick();
  }
}
int temp(void){
  uint8_t ret;

  /* System Init */
  SystemInit();
  
  /* Identify BlueNRG1 platform */
  SdkEvalIdentification();

  /* Configure I/O communication channel */
  SdkEvalComUartInit(UART_BAUDRATE);

  /* BlueNRG-1,2 stack init */
  ret = BlueNRG_Stack_Initialization(&BlueNRG_Stack_Init_params);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("Error in BlueNRG_Stack_Initialization() 0x%02x\r\n", ret);
    while(1);
  }
  
  /* Application demo Led Init */
  SdkEvalLedInit(LED1); //Activity led
  SdkEvalLedInit(LED3); //Error led
  SdkEvalLedOn(LED1);
  SdkEvalLedOff(LED3);

  PRINTF("BlueNRG-1,2 BLE Sensor Demo Application (version: %s)\r\n", BLE_SENSOR_VERSION_STRING);
  
#if ST_USE_OTA_SERVICE_MANAGER_APPLICATION
  /* Initialize the button: to be done before Sensor_DeviceInit for avoiding to 
     overwrite pressure/temperature sensor IO configuration when using BUTTON_2 (IO5) */
  SdkEvalPushButtonInit(USER_BUTTON);
#endif /* ST_USE_OTA_SERVICE_MANAGER_APPLICATION */
  
  /* Sensor Device Init */
  ret = Sensor_DeviceInit();
  if (ret != BLE_STATUS_SUCCESS) {
    SdkEvalLedOn(LED3);
    while(1);
  }


  while(1)
  {
    /* BLE Stack Tick */
    BTLE_StackTick();

    /* Application Tick */
    APP_Tick();
    
    /* Power Save management */
    BlueNRG_Sleep(SLEEPMODE_NOTIMER, 0, 0); 
    
#if ST_OTA_FIRMWARE_UPGRADE_SUPPORT
    /* Check if the OTA firmware upgrade session has been completed */
    if (OTA_Tick() == 1)
    {
      /* Jump to the new application */
      OTA_Jump_To_New_Application();
    }
#endif /* ST_OTA_FIRMWARE_UPGRADE_SUPPORT */

#if ST_USE_OTA_SERVICE_MANAGER_APPLICATION
    if (SdkEvalPushButtonGetState(USER_BUTTON) == RESET)
    {
      OTA_Jump_To_Service_Manager_Application();
    }
#endif /* ST_USE_OTA_SERVICE_MANAGER_APPLICATION */
  }/* while (1) */
}

/* Hardware Error event. 
   This event is used to notify the Host that a hardware failure has occurred in the Controller. 
   Hardware_Code Values:
   - 0x01: Radio state error
   - 0x02: Timer overrun error
   - 0x03: Internal queue overflow error
   After this event is recommended to force device reset. */

void hci_hardware_error_event(uint8_t Hardware_Code)
{
   NVIC_SystemReset();
}

/**
  * This event is generated to report firmware error informations.
  * FW_Error_Type possible values: 
  * Values:
  - 0x01: L2CAP recombination failure
  - 0x02: GATT unexpected response
  - 0x03: GATT unexpected request
    After this event with error type (0x01, 0x02, 0x3) it is recommended to disconnect. 
*/
void aci_hal_fw_error_event(uint8_t FW_Error_Type,
                            uint8_t Data_Length,
                            uint8_t Data[])
{
  if (FW_Error_Type <= 0x03)
  {
    uint16_t connHandle;
    
    /* Data field is the connection handle where error has occurred */
    connHandle = LE_TO_HOST_16(Data);
    
    aci_gap_terminate(connHandle, BLE_ERROR_TERMINATED_REMOTE_USER); 
  }
}

/****************** BlueNRG-1,2 Sleep Management Callback ********************************/

SleepModes App_SleepMode_Check(SleepModes sleepMode)
{
  if(request_free_fall_notify || SdkEvalComIOTxFifoNotEmpty() || SdkEvalComUARTBusy())
    return SLEEPMODE_RUNNING;
  
  return SLEEPMODE_NOTIMER;
}

/***************************************************************************************/

#ifdef USE_FULL_ASSERT
/*******************************************************************************
* Function Name  : assert_failed
* Description    : Reports the name of the source file and the source line number
*                  where the assert_param error has occurred.
* Input          : - file: pointer to the source file name
*                  - line: assert_param error line source number
* Output         : None
* Return         : None
*******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    
    /* Infinite loop */
    while (1)
    {}
}
#endif

/******************* (C) COPYRIGHT 2015 STMicroelectronics *****END OF FILE****/
/** \endcond
 */
