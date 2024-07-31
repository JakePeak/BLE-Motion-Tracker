/*
  ******************************************************************************
  * @file    user.c 
  * @author  AMG - RF Application Team
  * @version V1.0.0
  * @date    12 - 10 - 2017
  * @brief   Application functions
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2017 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 
  
/** \cond DOXYGEN_SHOULD_SKIP_THIS
 */ 
 
/* Includes-----------------------------------------------------------------*/
#include "../src/user.h"

#include <stdio.h>
#include <string.h>
#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "gp_timer.h"
#include "SDK_EVAL_Config.h"
#include "OTA_btl.h"
#include "osal.h"
#include "SPI_User.h"

#ifndef DEBUG
#define DEBUG 0
#endif

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

extern GATT_Handles accel_gatt_handles;

/******************************************************************************
 * Function Name  : device_initialization.
 * Description    : Init a BlueNRG device.
 * Input          : None.
 * Output         : None.
 * Return         : True if error occured.
******************************************************************************/
uint8_t device_initialization(tBleStatus* out)
{
  uint16_t service_handle;
  uint16_t dev_name_char_handle;
  uint16_t appearance_char_handle;
  uint8_t status;
  uint8_t name[] = {0x4D, 0x6F, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x54, 0x72, 0x61,
		  0x63, 0x6B, 0x65, 0x72};
  // Suspected address: 123456789AAA = {0xAA,0x9A,0x78,0x56,0x34,0x12}
  // Chosen Address: C526AD36BCC2 = {0xC2,0xBC,0x36,0xAD,0x26,0xC5}
  uint8_t value_2[] = {0xC2,0xBC,0x36,0xAD,0x26,0xC5};

  //aci_hal_write_config_data
  //status = aci_hal_write_config_data(offset,length,value);
  status = aci_hal_write_config_data(CONFIG_DATA_STATIC_RANDOM_ADDRESS,CONFIG_DATA_STATIC_RANDOM_ADDRESS_LEN,value_2);
  if (status != BLE_STATUS_SUCCESS) {
	*out = status;
    return 2;
	//PRINTF("aci_hal_write_config_data() failed:0x%02x\r\n", status);
  }else{
    //PRINTF("aci_hal_write_config_data --> SUCCESS\r\n");
  }


  //aci_gatt_init
  //status = aci_gatt_init();
  status = aci_gatt_init();
  if (status != BLE_STATUS_SUCCESS) {
	*out = status;
    return 3;
	//PRINTF("aci_gatt_init() failed:0x%02x\r\n", status);
  }else{
    //PRINTF("aci_gatt_init --> SUCCESS\r\n");
  }

  //aci_gap_init
  //status = aci_gap_init(role,privacy_enabled,device_name_char_len, &service_handle, &dev_name_char_handle, &appearance_char_handle);
  status = aci_gap_init(GAP_PERIPHERAL_ROLE,0x00,0x0F, &service_handle, &dev_name_char_handle, &appearance_char_handle);
  if (status != BLE_STATUS_SUCCESS && status != FLASH_WRITE_FAILED) {
	*out = status;
    return 4;
	//PRINTF("aci_gap_init() failed:0x%02x\r\n", status);
  }else if (status == FLASH_WRITE_FAILED){
	// Mark flash failure, but do nothing
	*out = (tBleStatus)1;
  }else{
    //PRINTF("aci_gap_init --> SUCCESS\r\n");
  }

  status = aci_gatt_update_char_value_ext(0, service_handle, dev_name_char_handle, 0, sizeof(name), 0, sizeof(name), name);
  if(status != BLE_STATUS_SUCCESS){
	  return 26;
  }

  //aci_hal_set_tx_power_level
  //status = aci_hal_set_tx_power_level(en_high_power,pa_level);
  status = aci_hal_set_tx_power_level(0x01,0x04);
  if (status != BLE_STATUS_SUCCESS) {
	*out = status;
    return 5;
	//PRINTF("aci_hal_set_tx_power_level() failed:0x%02x\r\n", status);
  }else{
    //PRINTF("aci_hal_set_tx_power_level --> SUCCESS\r\n");
  }

  // Additional GAP setup
  // Since this isn't a keyboard or a display, disabling those IOs
  /*status = aci_gap_set_io_capability(IO_CAP_NO_INPUT_NO_OUTPUT);
  if(status != BLE_STATUS_SUCCESS){
	*out = status;
	return 6;
  }*/

  // Giving the device a password of AUTH_PASSWORD, and remind it that we are using public pairing
  status = aci_gap_set_authentication_requirement(BONDING, MITM_PROTECTION_REQUIRED, SC_IS_NOT_SUPPORTED, KEYPRESS_IS_NOT_SUPPORTED,7,16, USE_FIXED_PIN_FOR_PAIRING, AUTH_PASSWORD, 0x01);
  if(status != BLE_STATUS_SUCCESS){
	*out = status;
	return 7;
  }
/*
  // Allow connection events to occur
  uint8_t eventMask[1] = {0x04};
  status = hci_set_event_mask(eventMask);
  if(status != BLE_STATUS_SUCCESS){
	  *out = status;
	  return 24;
  }

  // Set up the GATT bitmask to allow desired events to come through
  status = aci_gatt_set_event_mask(ACI_GATT_ATTRIBUTE_MODIFIED_EVENT | ACI_GATT_PROC_TIMEOUT_EVENT);
  if(status != BLE_STATUS_SUCCESS){
	  *out = status;
	  return 25;
  }*/
  return 0;
}

/******************************************************************************
 * Function Name  : device_GATT_profile.
 * Description    : Builds the GATT profile of the device
 * Input          : None.
 * Output         : Handles Struct containing all relevant handles.
 * Return         : Error code.
******************************************************************************/

uint8_t device_GATT_profile(tBleStatus* out){
	tBleStatus status;
	// Accelerometer service ID => d03ac8b6-59e9-4e59-846a-32aa3cb712bc

	Service_UUID_t accel_UUID;  // In Big-Endian order
	accel_UUID.Service_UUID_128[0] = 0xBC;
	accel_UUID.Service_UUID_128[1] = 0x12;
	accel_UUID.Service_UUID_128[2] = 0xB7;
	accel_UUID.Service_UUID_128[3] = 0x3C;
	accel_UUID.Service_UUID_128[4] = 0xAA;
	accel_UUID.Service_UUID_128[5] = 0x32;
	accel_UUID.Service_UUID_128[6] = 0x6A;
	accel_UUID.Service_UUID_128[7] = 0x84;
	accel_UUID.Service_UUID_128[8] = 0x59;
	accel_UUID.Service_UUID_128[9] = 0x4E;
	accel_UUID.Service_UUID_128[10] = 0xE9;
	accel_UUID.Service_UUID_128[11] = 0x59;
	accel_UUID.Service_UUID_128[12] = 0xB6;
	accel_UUID.Service_UUID_128[13] = 0xC8;
	accel_UUID.Service_UUID_128[14] = 0x3A;
	accel_UUID.Service_UUID_128[15] = 0xD0;

	// Custom notification characteristic => d03ac8b9-59e9-4e59-846a-32aa3cb712bc
	Char_UUID_t notif_UUID;  // In Big-Endian order
	notif_UUID.Char_UUID_128[0] = 0xBC;
	notif_UUID.Char_UUID_128[1] = 0x12;
	notif_UUID.Char_UUID_128[2] = 0xB7;
	notif_UUID.Char_UUID_128[3] = 0x3C;
	notif_UUID.Char_UUID_128[4] = 0xAA;
	notif_UUID.Char_UUID_128[5] = 0x32;
	notif_UUID.Char_UUID_128[6] = 0x6A;
	notif_UUID.Char_UUID_128[7] = 0x84;
	notif_UUID.Char_UUID_128[8] = 0x59;
	notif_UUID.Char_UUID_128[9] = 0x4E;
	notif_UUID.Char_UUID_128[10] = 0xE9;
	notif_UUID.Char_UUID_128[11] = 0x59;
	notif_UUID.Char_UUID_128[12] = 0xB9; // Key difference from service
	notif_UUID.Char_UUID_128[13] = 0xC8;
	notif_UUID.Char_UUID_128[14] = 0x3A;
	notif_UUID.Char_UUID_128[15] = 0xD0;

	// Battery service UUID for HID profile
	Service_UUID_t battServ_UUID, deviceInfo_UUID, HID_UUID;
	battServ_UUID.Service_UUID_16 = 0x180F;
	deviceInfo_UUID.Service_UUID_16 = 0x180A;
	HID_UUID.Service_UUID_16 = 0x1812;

	// Sensor location and speed UUID, used for accelerometer data char => 0x2A67
	Char_UUID_t accel_data_UUID;
	accel_data_UUID.Char_UUID_16 = 0x2A67;
	// Boolean UUID to control if in pause state => 0x2AE2
	Char_UUID_t pause_state;
	pause_state.Char_UUID_16 = 0x2AE2;
	// Battery level UUID, used for battery voltage
	Char_UUID_t battery_voltage_UUID;
	battery_voltage_UUID.Char_UUID_16 = 0x2A19;
	// Characterstic UUIDs for HID service
	Char_UUID_t report_map_UUID, HID_information_UUID, control_point_UUID, report_UUID;
	report_map_UUID.Char_UUID_16 = 0x2A4B;
	HID_information_UUID.Char_UUID_16 = 0x2A4A;
	control_point_UUID.Char_UUID_16 = 0x2A4C;
	report_UUID.Char_UUID_16 = 0x2A4D;

	// Descriptor UUID for HID
	Char_Desc_Uuid_t report_reference_UUID;
	report_reference_UUID.Char_UUID_16 = 0x2908;


	// Build the accelerometer data gatt service
	status = aci_gatt_add_service(0x02, &accel_UUID, 0x01, 10, &(accel_gatt_handles.service_handle));
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 8;
	}

	// Build the battery voltage gatt service for the HID profile
	status = aci_gatt_add_service(0x01, &battServ_UUID, 0x01, 4, &(accel_gatt_handles.battServ_handle));
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x1C;
	}

	// Build the battery voltage gatt characteristic
	status = aci_gatt_add_char(accel_gatt_handles.battServ_handle, 0x01, &battery_voltage_UUID, 1, CHAR_PROP_READ, 0x00, GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, 0x07, 0x00, &(accel_gatt_handles.battery_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x17;
	}

	// Build the device info gatt service (no required members)
	status = aci_gatt_add_service(0x01, &deviceInfo_UUID, 0x01, 3, &(accel_gatt_handles.devInfo_handle));
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x1D;
	}

	// Build the HID gatt service, requring report map, HID information, and HID control point
	status = aci_gatt_add_service(0x01, &HID_UUID, 0x01, 23, &(accel_gatt_handles.HIDServ_handle));
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x1E;
	}

	// Set up report map
	status = aci_gatt_add_char(accel_gatt_handles.HIDServ_handle, 0x01, &report_map_UUID, 19, CHAR_PROP_READ, 0x00, GATT_DONT_NOTIFY_EVENTS, 0x07, 0x00, &(accel_gatt_handles.repMap_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x1F;
	}

	// Set up HID information
	status = aci_gatt_add_char(accel_gatt_handles.HIDServ_handle, 0x01, &HID_information_UUID, 4, CHAR_PROP_READ, 0x00, GATT_DONT_NOTIFY_EVENTS, 0x07, 0x00, &(accel_gatt_handles.info_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x20;
	}

	// Set up HID control point
	status = aci_gatt_add_char(accel_gatt_handles.HIDServ_handle, 0x01, &control_point_UUID, 1, CHAR_PROP_WRITE_WITHOUT_RESP, 0x00, GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP, 0x07, 0x00, &(accel_gatt_handles.controlPt_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x21;
	}

	// Set up dummy HID feature report
	status = aci_gatt_add_char(accel_gatt_handles.HIDServ_handle, 0x01, &report_UUID, 1, CHAR_PROP_READ | CHAR_PROP_WRITE, 0x00, GATT_DONT_NOTIFY_EVENTS, 0x07, 0x00, &(accel_gatt_handles.HIDDummy_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x30;
	}
	uint8_t desc_val[2] = {0x00, 0x03};
	status = aci_gatt_add_char_desc(accel_gatt_handles.HIDServ_handle, accel_gatt_handles.HIDDummy_handle, 0x01, &report_reference_UUID, 2, 2, desc_val, 0, 0x01, GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, 0x07, 0x00, &(accel_gatt_handles.dummy_descriptor_handle));
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x23;
	}

	// Set up data ready notification characteristic to notify server when to perform long read from data characteristic
	status = aci_gatt_add_char(accel_gatt_handles.service_handle, 0x02, &notif_UUID, 1, CHAR_PROP_READ | CHAR_PROP_WRITE | CHAR_PROP_NOTIFY, 0x00, GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP | GATT_NOTIFY_ATTRIBUTE_WRITE, 0x07, 0x00, &(accel_gatt_handles.char_ready_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x22;
	}
	accel_gatt_handles.CCC_desc = accel_gatt_handles.char_ready_handle + 2;

	// Set up acceleration data gatt characteristic
	status = aci_gatt_add_char(accel_gatt_handles.service_handle, 0x01, &accel_data_UUID, POINTS_PER_PACKET*12, CHAR_PROP_READ, 0x00, GATT_DONT_NOTIFY_EVENTS, 0x07, 0x00, &(accel_gatt_handles.char_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 9;
	}

	// Device control characteristic, described by boolean
	// 0x01 => DEVICE_PAUSE => pause state if set
	// 0x00 => DEVICE_ACTIVE => not paused
	status = aci_gatt_add_char(accel_gatt_handles.service_handle, 0x01, &pause_state, 1, CHAR_PROP_READ | CHAR_PROP_WRITE, 0x00, GATT_NOTIFY_ATTRIBUTE_WRITE | GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP | GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP, 0x07, 0x00, &(accel_gatt_handles.char_desc_handle) );
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 10;
	}

	// Assign value to report map
	uint8_t map[] = {
		    0x05, 0x0c,                    // USAGE_PAGE (Consumer Devices)
		    0x09, 0x01,                    // USAGE (Consumer Control)
		    0xa1, 0x01,                    //   COLLECTION (Application)
		    0x09, 0x01,                    //   USAGE (Consumer Control)
		    0x25, 0x04,                    //   LOGICAL_MAXIMUM (4)
		    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
		    0x75, 0x08,                    //   REPORT_SIZE (8)
		    0x95, 0x01,                    //   REPORT_COUNT (1)
		    0xB1, 0x20,                    //   FEATURE (Data,Ary,Abs,NPrf)
		    0xc0                           // END_COLLECTION
		};

	status = aci_gatt_update_char_value_ext(0, accel_gatt_handles.HIDServ_handle, accel_gatt_handles.repMap_handle, 0, 19, 0, 19, map);
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x28;
	}

	// Assign value to HID information
	uint8_t HID_info[] = {0,11U,0x00,0x02};
	status = aci_gatt_update_char_value_ext(0, accel_gatt_handles.HIDServ_handle, accel_gatt_handles.info_handle, 0, 4, 0, 4, HID_info);
	if(status != BLE_STATUS_SUCCESS){
		*out = status;
		return 0x29;
	}

	return 0;
}

/******************************************************************************
 * Function Name  : set_device_discoverable.
 * Description    : Puts the device in connectable mode.
 * Input          : None.
 * Output         : None.
 * Return         : None.
******************************************************************************/
uint8_t set_device_discoverable(void)
{
  uint8_t status;

  //Assigning name to device "Motion Tracker"
  uint8_t name[] = {0x09, 0x4D, 0x6F, 0x74, 0x69, 0x6F, 0x6E, 0x20, 0x54, 0x72, 0x61,
		  0x63, 0x6B, 0x65, 0x72};
  uint8_t service_uuid_list[] = {0x02, 0xD0, 0x3A, 0xC8, 0xB6, 0x59, 0xE9, 0x4E, 0x59, 0x84, 0x6A, 0x32, 0xAA, 0x3C, 0xB7, 0x12, 0xBC};
  hci_le_set_scan_response_data(0, NULL);
  //aci_gap_set_discoverable
  //status = aci_gap_set_discoverable(advertising_type,advertising_interval_min,advertising_interval_max,own_address_type,advertising_filter_policy,local_name_length,local_name_length,service_uuid_length,service_uuid_length,slave_conn_interval_min,slave_conn_interval_max);
  status = aci_gap_set_discoverable(ADV_IND,0x0320,0x03C0,STATIC_RANDOM_ADDR,NO_WHITE_LIST_USE,sizeof(name),name,0,NULL,0,0);

  if (status != BLE_STATUS_SUCCESS) {
    return 11;
  }else{
    return 0;
  }



/*
 * This came with the code, however this device shouldn't need additional advertisement data since not broadcasting
 * uint8_t advertising_data_12[] = {0x02,0x01,0x06,0x1A,0xFF,0x30,0x00,0x02,0x15,0xE2,0x0A,0x39,0xF4,0x73,0xF5,0x4B,0xC4,0xA1,0x2F,0x17,0xD1,0xAD,0x07,0xA9,0x61,0x00,0x01,0x00,0x02,0xC8,0x00};
 */

/*
 *  Original code, changed from host-controller interface to application-command interface
 *  //hci_le_set_advertising_data
 *  //status = hci_le_set_advertising_data(advertising_data_length,advertising_data);
 *  status = hci_le_set_advertising_data(0x1E,advertising_data_12);
 */

/*
 * If this code is to be used, use this line instead of above line
 *
 * //Uses same parameters, just with different architecture
 *   status = aci_gap_update_adv_data(0x1E, advertising_data_12);
 * if (status != BLE_STATUS_SUCCESS) {
 *   PRINTF("hci_le_set_advertising_data() failed:0x%02x\r\n", status);
 * }else{
 *   PRINTF("hci_le_set_advertising_data --> SUCCESS\r\n");
 * }
 *
 */
}
/*
 * @brief	Disables all necessary components when exiting the active device state
 */
extern uint8_t packet_counter;
extern uint8_t calibrating;
void exit_active_state(void){
	// Disable SPI interrupts
	SPI_ITConfig(SPI_IT_RX, DISABLE);

	// Turn off the accelerometer
	SPI_Write(PWR_MGMT0_ADDR, 0x00);

	// Reset packet information
	packet_counter = 0;
	calibrating = 0x00;
}


/** \endcond 
*/
