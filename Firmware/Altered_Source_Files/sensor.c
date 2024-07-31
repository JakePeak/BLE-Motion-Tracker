/******************** (C) COPYRIGHT 2015 STMicroelectronics ********************
 * File Name          : sensor.c
 * Author             : AMS - VMA RF Application team
 * Version            : V1.0.0
 * Date               : 23-November-2015
 * Description        : Sensor init and sensor state machines
 ********************************************************************************
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h" 
#include "bluenrg1_stack.h"
#include "gp_timer.h"
#include "SDK_EVAL_Config.h"
#include "gatt_db.h"
#include "BLE_User_main.h"
#include "bluenrg1_api.h"
#ifndef SENSOR_EMULATION
#include "LPS25HB.h"
#include "lsm6ds3.h"
#include "LSM6DS3_hal.h"
#endif
#include "OTA_btl.h"   
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

#define UPDATE_CONN_PARAM 0 
#define  ADV_INTERVAL_MIN_MS  500
#define  ADV_INTERVAL_MAX_MS  600

#define BLE_SENSOR_VERSION_STRING "1.1.0" 

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
volatile uint8_t set_connectable = 1;
uint16_t connection_handle = 0;
uint8_t connInfo[20];

BOOL sensor_board = FALSE; // It is True if sensor boad has been detected

int connected = FALSE;
#if UPDATE_CONN_PARAM
#define UPDATE_TIMER 2 //TBR
int l2cap_request_sent = FALSE;
static uint8_t l2cap_req_timer_expired = FALSE; 
#endif

#define SENSOR_TIMER 1
static uint16_t acceleration_update_rate = 200;
static uint8_t sensorTimer_expired = FALSE;

#ifndef SENSOR_EMULATION
PRESSURE_DrvTypeDef* xLPS25HBDrv = &LPS25HBDrv;  
IMU_6AXES_DrvTypeDef *Imu6AxesDrv = NULL;
LSM6DS3_DrvExtTypeDef *Imu6AxesDrvExt = NULL;
static AxesRaw_t acceleration_data; 
#endif 

volatile uint8_t request_free_fall_notify = FALSE; 
  
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

#ifndef SENSOR_EMULATION

static IMU_6AXES_InitTypeDef Acc_InitStructure; 
/*******************************************************************************
 * Function Name  : Init_Accelerometer.
 * Description    : Init LIS331DLH accelerometer.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Init_Accelerometer(void)
{
  /* LSM6DS3 library setting */
  uint8_t tmp1 = 0x00;
  
  Imu6AxesDrv = &LSM6DS3Drv;
  Imu6AxesDrvExt = &LSM6DS3Drv_ext_internal;
  Acc_InitStructure.G_FullScale      = 125.0f;
  Acc_InitStructure.G_OutputDataRate = 13.0f;
  Acc_InitStructure.G_X_Axis         = 0; //1;
  Acc_InitStructure.G_Y_Axis         = 0;//1;
  Acc_InitStructure.G_Z_Axis         = 0; //1;
  Acc_InitStructure.X_FullScale      = 2.0f;
  Acc_InitStructure.X_OutputDataRate = 13.0f;
  Acc_InitStructure.X_X_Axis         = 1;
  Acc_InitStructure.X_Y_Axis         = 1;
  Acc_InitStructure.X_Z_Axis         = 1;  
  
  /* LSM6DS3 initiliazation */
  Imu6AxesDrv->Init(&Acc_InitStructure);
    
  /* Disable all mems IRQs in order to enable free fall detection */ //TBR
  LSM6DS3_IO_Write(&tmp1, LSM6DS3_XG_MEMS_ADDRESS, LSM6DS3_XG_INT1_CTRL, 1);
  
  /* Clear first previous data */
  Imu6AxesDrv->Get_X_Axes((int32_t *)&acceleration_data);
  
  /* Enable Free fall detection */
  Imu6AxesDrvExt->Enable_Free_Fall_Detection(); 
}

static PRESSURE_InitTypeDef Pressure_InitStructure;

/*******************************************************************************
 * Function Name  : Init_Pressure_Temperature_Sensor.
 * Description    : Init LPS25HB pressure and temperature sensor.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Init_Pressure_Temperature_Sensor(void)
{  
  /* LPS25HB initialization */

  Pressure_InitStructure.OutputDataRate = LPS25HB_ODR_1Hz;
  Pressure_InitStructure.BlockDataUpdate = LPS25HB_BDU_READ; //LPS25HB_BDU_READ LPS25HB_BDU_CONT
  Pressure_InitStructure.DiffEnable = LPS25HB_DIFF_ENABLE;  // LPS25HB_DIFF_ENABLE
  Pressure_InitStructure.SPIMode = LPS25HB_SPI_SIM_3W;  // LPS25HB_SPI_SIM_3W
  Pressure_InitStructure.PressureResolution = LPS25HB_P_RES_AVG_32;
  Pressure_InitStructure.TemperatureResolution = LPS25HB_T_RES_AVG_16;  
  xLPS25HBDrv->Init(&Pressure_InitStructure);
}

/*******************************************************************************
 * Function Name  : Init_Humidity_Sensor.
 * Description    : Init HTS221 temperature sensor.
 * Input          : None.
 * Output         : None.
 * Return         : 0 if success, 1 if error.
 *******************************************************************************/
int Init_Humidity_Sensor(void)
{
  /* No humidity sensor available on BlueNRG-1 Application Board*/
  return 0;
}
#endif

/*******************************************************************************
 * Function Name  : Sensor_DeviceInit.
 * Description    : Init the device sensors.
 * Input          : None.
 * Return         : Status.
 *******************************************************************************/
uint8_t Sensor_DeviceInit()
{
  uint8_t bdaddr[] = {0x12, 0x34, 0x00, 0xE1, 0x80, 0x02};
  uint8_t ret;
  uint16_t service_handle, dev_name_char_handle, appearance_char_handle;
  uint8_t device_name[] = {'B', 'l', 'u', 'e', 'N', 'R', 'G'};

  /* Set the device public address */
  ret = aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN,
                                  bdaddr);  
  if(ret != BLE_STATUS_SUCCESS) {
    PRINTF("aci_hal_write_config_data() failed: 0x%02x\r\n", ret);
    return ret;
  }
  
  /* Set the TX power -2 dBm */
  aci_hal_set_tx_power_level(1, 4);
  
  /* GATT Init */
  ret = aci_gatt_init();
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_init() failed: 0x%02x\r\n", ret);
    return ret;
  }
  
  /* GAP Init */
  ret = aci_gap_init(GAP_PERIPHERAL_ROLE, 0, 0x07, &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gap_init() failed: 0x%02x\r\n", ret);
    return ret;
  }
 
  /* Update device name */
  ret = aci_gatt_update_char_value_ext(0, service_handle, dev_name_char_handle, 0,sizeof(device_name), 0, sizeof(device_name), device_name);
  if(ret != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gatt_update_char_value_ext() failed: 0x%02x\r\n", ret);
    return ret;
  }
  
  ret = aci_gap_set_authentication_requirement(BONDING,
                                               MITM_PROTECTION_REQUIRED,
                                               SC_IS_NOT_SUPPORTED,
                                               KEYPRESS_IS_NOT_SUPPORTED,
                                               7, 
                                               16,
                                               USE_FIXED_PIN_FOR_PAIRING,
                                               123456,
                                               0x00);
  if(ret != BLE_STATUS_SUCCESS) {
    PRINTF("aci_gap_set_authentication_requirement()failed: 0x%02x\r\n", ret);
    return ret;
  } 
  
  PRINTF("BLE Stack Initialized with SUCCESS\n");

#ifndef SENSOR_EMULATION /* User Real sensors */
  Init_Accelerometer();
  Init_Pressure_Temperature_Sensor();
#endif   


  /* Add ACC service and Characteristics */
  ret = Add_Acc_Service();
  if(ret == BLE_STATUS_SUCCESS) {
    PRINTF("Acceleration service added successfully.\n");
  } else {
    PRINTF("Error while adding Acceleration service: 0x%02x\r\n", ret);
    return ret;
  }

  /* Add Environmental Sensor Service */
  ret = Add_Environmental_Sensor_Service();
  if(ret == BLE_STATUS_SUCCESS) {
    PRINTF("Environmental service added successfully.\n");
  } else {
    PRINTF("Error while adding Environmental service: 0x%04x\r\n", ret);
    return ret;
  }

#if ST_OTA_FIRMWARE_UPGRADE_SUPPORT     
  ret = OTA_Add_Btl_Service();
  if(ret == BLE_STATUS_SUCCESS)
    PRINTF("OTA service added successfully.\n");
  else
    PRINTF("Error while adding OTA service.\n");
#endif /* ST_OTA_FIRMWARE_UPGRADE_SUPPORT */ 
  
  /* Start the Sensor Timer */
  ret = HAL_VTimerStart_ms(SENSOR_TIMER, acceleration_update_rate);
  if (ret != BLE_STATUS_SUCCESS) {
    PRINTF("HAL_VTimerStart_ms() failed; 0x%02x\r\n", ret);
    return ret;
  } else {
    sensorTimer_expired = FALSE;
  }

  return BLE_STATUS_SUCCESS;
}

/*******************************************************************************
 * Function Name  : Set_DeviceConnectable.
 * Description    : Puts the device in connectable mode.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void Set_DeviceConnectable(void)
{  
  uint8_t ret;
  uint8_t local_name[] = {AD_TYPE_COMPLETE_LOCAL_NAME,'B','l','u','e','N','R','G'}; 

#if ST_OTA_FIRMWARE_UPGRADE_SUPPORT
  hci_le_set_scan_response_data(18,BTLServiceUUID4Scan); 
#else
  hci_le_set_scan_response_data(0,NULL);
#endif /* ST_OTA_FIRMWARE_UPGRADE_SUPPORT */ 
  PRINTF("Set General Discoverable Mode.\n");
  
  ret = aci_gap_set_discoverable(ADV_IND,
                                (ADV_INTERVAL_MIN_MS*1000)/625,(ADV_INTERVAL_MAX_MS*1000)/625,
                                 STATIC_RANDOM_ADDR, NO_WHITE_LIST_USE,
                                 sizeof(local_name), local_name, 0, NULL, 0, 0); 
  if(ret != BLE_STATUS_SUCCESS)
  {
    PRINTF("aci_gap_set_discoverable() failed: 0x%02x\r\n",ret);
    SdkEvalLedOn(LED3);  
  }
  else
    PRINTF("aci_gap_set_discoverable() --> SUCCESS\r\n");
}

/*******************************************************************************
 * Function Name  : APP_Tick.
 * Description    : Sensor Demo state machine.
 * Input          : None.
 * Output         : None.
 * Return         : None.
 *******************************************************************************/
void APP_Tick(void)
{
  /* Make the device discoverable */
  if(set_connectable) {
    Set_DeviceConnectable();
    set_connectable = FALSE;
  }

#if UPDATE_CONN_PARAM      
  /* Connection parameter update request */
  if(connected && !l2cap_request_sent && l2cap_req_timer_expired){
    aci_l2cap_connection_parameter_update_req(connection_handle, 9, 20, 0, 600); //24, 24
    l2cap_request_sent = TRUE;
  }
#endif
    
  /*  Update sensor value */
  if (sensorTimer_expired) {
    sensorTimer_expired = FALSE;
    if (HAL_VTimerStart_ms(SENSOR_TIMER, acceleration_update_rate) != BLE_STATUS_SUCCESS)
      sensorTimer_expired = TRUE;
    if(connected) {
      AxesRaw_t acc_data;
      
      /* Activity Led */
      SdkEvalLedToggle(LED1);  

      /* Get Acceleration data */
      if (GetAccAxesRaw(&acc_data) == IMU_6AXES_OK) {
        Acc_Update(&acc_data);
      }
        
      /* Get free fall status */
      GetFreeFallStatus();
    }
  }

  /* Free fall notification */
  if(request_free_fall_notify == TRUE) {
    request_free_fall_notify = FALSE;
    Free_Fall_Notify();
  }
}

extern connection_info conn_info;
extern uint8_t device_state;
extern GATT_Handles accel_gatt_handles;
extern uint8_t calibrating;

int IsValidHandle(uint16_t Attribute_Handle){
	return ((Attribute_Handle == accel_gatt_handles.CCC_desc)
			|| (Attribute_Handle == accel_gatt_handles.char_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.char_handle)
			|| (Attribute_Handle == accel_gatt_handles.char_desc_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.char_desc_handle)
			|| (Attribute_Handle == accel_gatt_handles.battery_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.battery_handle)
			|| (Attribute_Handle == accel_gatt_handles.repMap_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.repMap_handle)
			|| (Attribute_Handle == accel_gatt_handles.char_ready_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.char_ready_handle)
			|| (Attribute_Handle == accel_gatt_handles.controlPt_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.controlPt_handle)
			|| (Attribute_Handle == accel_gatt_handles.info_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.info_handle)
			|| (Attribute_Handle == accel_gatt_handles.HIDDummy_handle+1)
			|| (Attribute_Handle == accel_gatt_handles.HIDDummy_handle)
			|| (Attribute_Handle == accel_gatt_handles.dummy_descriptor_handle));
}

/* ***************** BlueNRG-1 Stack Callbacks ********************************/

/*******************************************************************************
 * Function Name  : hci_le_connection_complete_event.
 * Description    : This event indicates that a new connection has been created.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void hci_le_connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Role,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Conn_Interval,
                                      uint16_t Conn_Latency,
                                      uint16_t Supervision_Timeout,
                                      uint8_t Master_Clock_Accuracy)
{
  
  connected = TRUE;
  connection_handle = Connection_Handle;
	// If connection failed, do nothing
		if(Status != BLE_STATUS_SUCCESS){
			Error(18);
			return;
		}
		// If we are not the slave, something is wrong, so disconnect
		if(Role != 0x01){
			aci_gap_terminate(Connection_Handle, 0x3B);
			Error(19);
		}
		// Else, start saving important data, especially connection handle
		conn_info.connection_handle = Connection_Handle;
		conn_info.peer_address_type = Peer_Address_Type;
		conn_info.conn_interval = Conn_Interval;
		conn_info.master_clock_accuracy = Master_Clock_Accuracy;
		conn_info.slave_latency = Conn_Latency;
		conn_info.supervision_timeout = Supervision_Timeout;
		for(int i_ = 0; i_ < 6; i_++){
			conn_info.peer_address[i_] = Peer_Address[i_];
		}
		// If connection interval is too large, attemp to reduce it
		if(Conn_Interval*1.25 > 70){
			aci_l2cap_connection_parameter_update_req(Connection_Handle, 0x08, 0x038, Conn_Latency, Supervision_Timeout);
		}
		// Finally, transfer out of the waiting for connection state!
		device_state = 0x0C;
#if UPDATE_CONN_PARAM    
  l2cap_request_sent = FALSE;
  HAL_VTimerStart_ms(UPDATE_TIMER, CLOCK_SECOND*2);
  {
    l2cap_req_timer_expired = FALSE;
  }
#endif
    
}/* end hci_le_connection_complete_event() */

/*******************************************************************************
 * Function Name  : hci_disconnection_complete_event.
 * Description    : This event occurs when a connection is terminated.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void hci_disconnection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Reason)
{
  Error(Reason);
  connected = FALSE;
  /* Make the device connectable again. */
  set_connectable = TRUE;
  connection_handle =0;
    // If in a disconnect state, move to new state
    if((device_state & 0x0F) == 0x0F){
    	device_state = 0x01;
    }
    // If in operational state, go to active mode disconnect state
  	if((device_state & 0xF0) == 0x10){
  		device_state = 0x1F;
  	}
  	// If in pause state, go to sleep mode disconnect state
  	else if((device_state & 0xF0) == 0x20){
  		device_state = 0x2F;
  	}
  	// If in unattached state, re-detach and start again
  	else if((device_state & 0xF0) == 0x00){
  		Clock_Wait(100);
  		device_state = 0x01;
  	}
  	// If in an error state, do nothing
  	else{

  	}
  
  //SdkEvalLedOn(LED1);//activity led
#if ST_OTA_FIRMWARE_UPGRADE_SUPPORT
  OTA_terminate_connection();
#endif 
}/* end hci_disconnection_complete_event() */


/*******************************************************************************
 * Function Name  : aci_gatt_read_permit_req_event.
 * Description    : This event is given when a read request is received
 *                  by the server from the client.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void aci_gatt_read_permit_req_event(uint16_t Connection_Handle,
                                    uint16_t Attribute_Handle,
                                    uint16_t Offset)
{
  //Read_Request_CB(Attribute_Handle);
	// Always enable read if valid handle
	if(IsValidHandle(Attribute_Handle)){
		// If it is a battery voltage read request, update the battery voltage
		if(Attribute_Handle == (accel_gatt_handles.battery_handle+1) || Attribute_Handle == (accel_gatt_handles.battery_handle)){
			uint8_t batteryPercent = ADC_RequestBattery();
			if(aci_gatt_update_char_value(accel_gatt_handles.battServ_handle, accel_gatt_handles.battery_handle, 0, 0x01, &batteryPercent) != BLE_STATUS_SUCCESS){
				Error(26);
			}

		}
		// Else just allow read
		aci_gatt_allow_read(Connection_Handle);
	}
	else{
		Error(0x31);
		aci_gatt_deny_read(Connection_Handle, 0x80);
	}
}

/*******************************************************************************
 * Function Name  : HAL_VTimerTimeoutCallback.
 * Description    : This function will be called on the expiry of 
 *                  a one-shot virtual timer.
 * Input          : See file bluenrg1_stack.h
 * Output         : See file bluenrg1_stack.h
 * Return         : See file bluenrg1_stack.h
 *******************************************************************************/
void HAL_VTimerTimeoutCallback(uint8_t timerNum)
{
  if (timerNum == SENSOR_TIMER) {
    sensorTimer_expired = TRUE;
  }
#if UPDATE_CONN_PARAM
  else if (timerNum == UPDATE_TIMER) {
    l2cap_req_timer_expired = TRUE;
  }
#endif
}

/*******************************************************************************
 * Function Name  : aci_gatt_attribute_modified_event.
 * Description    : This event occurs when an attribute is modified.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void aci_gatt_attribute_modified_event(uint16_t Connection_Handle,
                                       uint16_t Attr_Handle,
                                       uint16_t Offset,
                                       uint16_t Attr_Data_Length,
                                       uint8_t Attr_Data[])
{
	uint8_t New_Attr_Data;
	// If they have subscribed to notifications
	if(Attr_Handle == accel_gatt_handles.CCC_desc){
		// If they are subscribing
		if(Attr_Data[0] & 0x01){
			if(device_state == 0x0F){
				// Enter paused state
				device_state = 0x21;
				// Set device current state to paused state
				New_Attr_Data = DEVICE_PAUSE;
				aci_gatt_update_char_value_ext(Connection_Handle, accel_gatt_handles.service_handle, accel_gatt_handles.char_desc_handle, 0, 1, 0, 1, &New_Attr_Data);
			}
			// If not in valid state, undo it
			else if(device_state & 0xF0 == 0x00){
				New_Attr_Data = DEVICE_ACTIVE;
				aci_gatt_set_desc_value(accel_gatt_handles.service_handle, accel_gatt_handles.char_ready_handle, accel_gatt_handles.CCC_desc, 0, 1, &New_Attr_Data);
				Error(20);
			}
			// Else just allow it to happen
		}
		// If they are unsubscribing
		else{
			// If in the active state, move accordingly
			if(device_state & 0x10 == 0x10){
				device_state = 0x1B;
			}
			// If in pause state, move directly
			else if (device_state & 0x20 == 0x20){
				device_state = 0x0F;
			}
			// If in connection state, ignore
		}
	}
	else if(Attr_Handle == (accel_gatt_handles.char_desc_handle+1)){
		// If it is a pause command
		if((Attr_Data[0] & 0x01) == DEVICE_PAUSE){
			// If in any variant of the active state move to the transfer state
			if((device_state & 0xF0) == 0x10){
				device_state = 0x1D;
			}
			// Cannot pause from any other state, so else undo
			else{
				New_Attr_Data = DEVICE_ACTIVE;
				aci_gatt_update_char_value_ext(0x0000, accel_gatt_handles.service_handle, accel_gatt_handles.char_desc_handle, 0x00, 1, 0, 1, &New_Attr_Data);
				Error(21);
			}
		}
		// If it is a activate command
		else{
			// Only can activate from general pause state
			if(device_state == 0x20){
				// move to initialize operational state
				device_state = 0x11;
			}
			// Ignore if invalid
			else{
				New_Attr_Data = DEVICE_PAUSE;
				aci_gatt_update_char_value_ext(0x0000, accel_gatt_handles.service_handle, accel_gatt_handles.char_desc_handle, 0x00, 1, 0, 1, &New_Attr_Data);
				Error(22);
			}
		}
	}
	else if(Attr_Handle == (accel_gatt_handles.char_ready_handle+1)){
		// Update calibrating global
		calibrating = Attr_Data[0];
	}
#if ST_OTA_FIRMWARE_UPGRADE_SUPPORT
  OTA_Write_Request_CB(Connection_Handle, Attr_Handle, Attr_Data_Length, Attr_Data);
#endif /* ST_OTA_FIRMWARE_UPGRADE_SUPPORT */ 
}


void aci_hal_end_of_radio_activity_event(uint8_t Last_State,
                                         uint8_t Next_State,
                                         uint32_t Next_State_SysTime)
{
#if ST_OTA_FIRMWARE_UPGRADE_SUPPORT
  if (Next_State == 0x02) /* 0x02: Connection event slave */
  {
    OTA_Radio_Activity(Next_State_SysTime);  
  }
#endif 
}
/**
 * @brief This event is generated by the client/server to the application on a
 *        GATT timeout (30 seconds). This is a critical event that should not
 *        happen during normal operating conditions. It is an indication of
 *        either a major disruption in the communication link or a mistake in
 *        the application which does not provide a reply to GATT procedures.
 *        After this event, the GATT channel is closed and no more GATT
 *        communication can be performed. The applications is exptected to issue
 *        an @ref aci_gap_terminate to disconnect from the peer device. It is
 *        important to leave an 100 ms blank window before sending the @ref
 *        aci_gap_terminate, since immediately after this event, system could
 *        save important information in non volatile memory.
 * @param Connection_Handle Connection handle on which the GATT procedure has
 *        timed out
 * @retval None
 */
void aci_gatt_proc_timeout_event(uint16_t Connection_Handle){
	// If in operational state, go to active mode disconnect state
	if((device_state & 0xF0) == 0x10){
		device_state = 0x1F;
	}
	// If in pause state, go to sleep mode disconnect state
	else if((device_state & 0xF0) == 0x20){
		device_state = 0x2F;
	}
	// If in unattached state, re-detach and start again
	else if((device_state & 0xF0) == 0x00){
		Clock_Wait(100);
		aci_gap_terminate(conn_info.connection_handle,0x15);
		device_state = 0x01;
	}
	// If in an error state, do nothing
	else{

	}
}
/*********************************************************************************
 * Pairing events
 *
 * Pairing is added as an optional feature to allow for better connection with
 * windows devices
 *********************************************************************************/

/*
 * @brief	During pairing, allow access
 */
void aci_gap_authorization_req_event(uint16_t Connection_Handle){
	aci_gap_authorization_resp(Connection_Handle, 0x01);
}

/*
 * @brief	When given a Yes/No for matching pins, always say yes
 */
void aci_gap_numeric_comparison_value_event(uint16_t Connection_Handle,
                                            uint32_t Numeric_Value){
	aci_gap_numeric_comparison_value_confirm_yesno(Connection_Handle, 0x01);
}

/*
 * @brief	Pairing complete event, used for debugging
 */
void aci_gap_pairing_complete_event(uint16_t Connection_Handle,
                                    uint8_t Status,
                                    uint8_t Reason){
	Error(Status);
	Clock_Wait(300);
	Error(Reason);
}

/*********************************************************************************
 * Additional events added for completion
 *
 * These are not intended to be used with normal function, but are included to
 * be safe and make the program more versatile
 *********************************************************************************/
/**
 * @brief This event is given to the application when a write request, write
 *        command or signed write command is received by the server from the
 *        client. This event will be given to the application only if the event
 *        bit for this event generation is set when the characteristic was
 *        added. When this event is received, the application has to check
 *        whether the value being requested for write can be allowed to be
 *        written and respond with the command @ref aci_gatt_write_resp. The
 *        details of the parameters of the command can be found. Based on the
 *        response from the application, the attribute value will be modified by
 *        the stack. If the write is rejected by the application, then the value
 *        of the attribute will not be modified. In case of a write REQ, an
 *        error response will be sent to the client, with the error code as
 *        specified by the application. In case of write/signed write commands,
 *        no response is sent to the client but the attribute is not modified.
 * @param Connection_Handle Handle of the connection on which there was the
 *        request to write the attribute
 * @param Attribute_Handle The handle of the attribute
 * @param Data_Length Length of Data field
 * @param Data The data that the client has requested to write
 * @retval None
 */
void aci_gatt_write_permit_req_event(uint16_t Connection_Handle,
                                     uint16_t Attribute_Handle,
                                     uint8_t Data_Length,
                                     uint8_t Data[]){
	if(Attribute_Handle == (accel_gatt_handles.char_handle+1)){
		// This is read-only and used for data transmission, report error
		aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x01, 0x80, Data_Length, Data);
	}
	else if(Attribute_Handle == (accel_gatt_handles.char_desc_handle+1)|| Attribute_Handle == accel_gatt_handles.CCC_desc || Attribute_Handle == accel_gatt_handles.char_ready_handle+1){
		// Should always be written if correct length and no offset for both cases
		if(Data_Length == 1){
			aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x00, 0xE0, Data_Length, Data);
		}
		else{
			aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x01, 0x80, Data_Length, Data);
		}
	}
	else if(IsValidHandle(Attribute_Handle)){
		// Just let it happen
		aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x00, 0xE0, Data_Length, Data);
	}

	else{
		// Attribute doesn't exist
		aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x01, 0xE0, Data_Length, Data);
	}
}

/**
 * @brief This event is given to the application when a read multiple request or
 *        read by type request is received by the server from the client. This
 *        event will be given to the application only if the event bit for this
 *        event generation is set when the characteristic was added. On
 *        receiving this event, the application can update the values of the
 *        handles if it desires and when done, it has to send the @ref
 *        aci_gatt_allow_read command to indicate to the stack that it can send
 *        the response to the client.
 * @param Connection_Handle Handle of the connection which requested to read the
 *        attribute
 * @param Number_of_Handles
 * @param Handle_Item See @ref Handle_Item_t
 * @retval None
 */
void aci_gatt_read_multi_permit_req_event(uint16_t Connection_Handle,
                                          uint8_t Number_of_Handles,
                                          Handle_Item_t Handle_Item[]){
	// Check if all handles are valid
	for(uint8_t i = 0; i < Number_of_Handles; i++){
		if(!IsValidHandle(Handle_Item[i].Handle)){
				aci_gatt_deny_read(Connection_Handle, 0x80);
		}
		// Else if the handle is a battery handle, update the battery level
		else{
			if(Handle_Item[i].Handle == accel_gatt_handles.battery_handle+1 || Handle_Item[i].Handle == accel_gatt_handles.battery_handle){
				uint8_t batteryPercent = ADC_RequestBattery();
				if(aci_gatt_update_char_value(accel_gatt_handles.battServ_handle, accel_gatt_handles.battery_handle, 0, 0x01, &batteryPercent) != BLE_STATUS_SUCCESS){
					Error(27);
				}
			}
		}
	}
	aci_gatt_allow_read(Connection_Handle);
}

/**
 * @brief This event is generated when the client has sent the confirmation to a
 *        previously sent indication
 * @param Connection_Handle Connection handle related to the event
 * @retval None
 */
void aci_gatt_server_confirmation_event(uint16_t Connection_Handle){
	// Do nothing
	;
}

/**
 * @brief This event is given to the application when a prepare write request is
 *        received by the server from the client. This event will be given to
 *        the application only if the event bit for this event generation is set
 *        when the characteristic was added. When this event is received, the
 *        application has to check whether the value being requested for write
 *        can be allowed to be written and respond with the command @ref
 *        aci_gatt_write_resp. Based on the response from the application, the
 *        attribute value will be modified by the stack. If the write is
 *        rejected by the application, then the value of the attribute will not
 *        be modified and an error response will be sent to the client, with the
 *        error code as specified by the application.
 * @param Connection_Handle Handle of the connection on which there was the
 *        request to write the attribute
 * @param Attribute_Handle The handle of the attribute
 * @param Offset The offset from which the prepare write has been requested
 * @param Data_Length Length of Data field
 * @param Data The data that the client has requested to write
 * @retval None
 */
void aci_gatt_prepare_write_permit_req_event(uint16_t Connection_Handle,
                                             uint16_t Attribute_Handle,
                                             uint16_t Offset,
                                             uint8_t Data_Length,
                                             uint8_t Data[]){
	if(Attribute_Handle == accel_gatt_handles.char_handle){
		// This is read-only and used for data transmission, report error
		aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x01, 0x80, Data_Length, Data);
	}
	else if(Attribute_Handle == (accel_gatt_handles.char_desc_handle+1) || Attribute_Handle == accel_gatt_handles.CCC_desc){
		// Should always be written if correct length and no offset for both cases
		if(Data_Length == 1 && Offset == 0){
			aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x00, 0xE0, Data_Length, Data);
		}
		else{
			aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x01, 0x80, Data_Length, Data);
		}
	}
	else if (IsValidHandle(Attribute_Handle)){
		// Else just let it happen
		aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x00, 0xE0, Data_Length, Data);
	}
	else{
		// Attribute doesn't exist
		aci_gatt_write_resp(Connection_Handle, Attribute_Handle, 0x01, 0xE0, Data_Length, Data);
	}

}
