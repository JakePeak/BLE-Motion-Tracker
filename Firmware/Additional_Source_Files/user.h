/*
  ******************************************************************************
  * @file    user.h 
  * @author  AMG - RF Application Team
  * @version V1.0.0
  * @date    12 - 10 - 2017
  * @brief   Application Header functions
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
 
#ifndef _user_H_
#define _user_H_

#define AUTH_PASSWORD		111111
#define DEVICE_ACTIVE		(uint8_t)0x00
#define DEVICE_PAUSE		(uint8_t)0x01
#define POINTS_PER_PACKET 	10
#include <stdint.h>
#include "ble_status.h"
// GATT event mask definitions (added by user)
#define ACI_GATT_ATTRIBUTE_MODIFIED_EVENT			0x00000001
#define ACI_GATT_PROC_TIMEOUT_EVENT  				0x00000002
#define ACI_ATT_EXCHANGE_MTU_RESP_EVENT 			0x00000004
#define ACI_ATT_FIND_INFO_RESP_EVENT 				0x00000008
#define ACI_ATT_FIND_BY_TYPE_VALUE_RESP_EVENT 		0x00000010
#define ACI_ATT_READ_BY_TYPE_RESP_EVENT 			0x00000020
#define ACI_ATT_READ_RESP_EVENT 					0x00000040
#define ACI_ATT_READ_BLOB_RESP_EVENT 				0x00000080
#define ACI_ATT_READ_MULTIPLE_RESP_EVENT 			0x00000100
#define ACI_ATT_READ_BY_GROUP_TYPE_RESP_EVENT 		0x00000200
#define ACI_ATT_PREPARE_WRITE_RESP_EVENT 			0x00000800
#define ACI_ATT_EXEC_WRITE_RESP_EVENT 				0x00001000
#define ACI_GATT_INDICATION_EVENT 					0x00002000
#define ACI_GATT_NOTIFICATION_EVENT 				0x00004000
#define ACI_GATT_ERROR_RESP_EVENT 					0x00008000
#define ACI_GATT_PROC_COMPLETE_EVENT 				0x00010000
#define ACI_GATT_DISC_READ_CHAR_BY_UUID_RESP_EVENT 	0x00020000
#define ACI_GATT_TX_POOL_AVAILABLE_EVENT 			0x00040000

// Struct to contain various handles to pass to main function
typedef struct {
	uint16_t service_handle;
	uint16_t char_handle;
	uint16_t char_desc_handle;
	uint16_t CCC_desc;
	uint16_t battery_handle;
	uint16_t battServ_handle;
	uint16_t devInfo_handle;
	uint16_t HIDServ_handle;
	uint16_t repMap_handle;
	uint16_t info_handle;
	uint16_t HIDDummy_handle;
	uint16_t controlPt_handle;
	uint16_t char_ready_handle;
	uint16_t dummy_descriptor_handle;
}GATT_Handles;

/**
  * @brief  This function initializes the BLE GATT & GAP layers and it sets the TX power level 
  * @param  None
  * @retval Error if applicable
  */
uint8_t device_initialization(tBleStatus* out);


/**
  * @brief  This function handles the BLE advertising mode 
  * @param  GATT_Handles containing service UUID(s)
  * @retval None
  */
uint8_t set_device_discoverable(void);


/*
 * @brief This function initializes the GATT profile of the device
 * @param (out) handle object containing all relevant handles
 * @retval Int true if error
 */
uint8_t device_GATT_profile(tBleStatus* out);

/*
 * @param Disables all necessary components when exiting the active device state
 */
void exit_active_state(void);


#endif /* _user_H_ */
/** \endcond 
*/
