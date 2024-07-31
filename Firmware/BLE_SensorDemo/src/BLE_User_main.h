/*
 *   ******************************************************************************
 * @file    BLE_User_main.h
 * @author  akj56
 * @version V1.0.0
 * @date    22-05-2024
 * @brief   This file acts as a header file for the main function
 ******************************************************************************
 *
 */

#ifndef INC_BLE_USER_MAIN_H_
#define INC_BLE_USER_MAIN_H_

#define BLE_STACK_CONFIGURAITION 	BLE_STACK_FULL_CONFIGURATION
#define HS_SPEED_XTAL				HS_SPEED_XTAL_32MHZ
#define LS_SOURCE					LS_SOURCE_EXTERNAL_32KHZ
#define SMPS_INDUCTOR				SMPS_INDUCTOR_10uH

/* Includes-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdint.h>
#include "BlueNRG1_it.h"
#include "BlueNRG1_conf.h"
#include "ble_const.h"
#include "bluenrg1_stack.h"
#include "SDK_EVAL_Config.h"
#include "sleep.h"
#include "gp_timer.h"
#include "user.h"
#include "osal.h"
#include "SPI_User.h"
#include "ADC_User.h"
#include "stack_user_cfg.h"

#define SAMPLE_INTERVAL (tClockTime)25

int main(void);
void hci_hardware_error_event(uint8_t Hardware_Code);
void aci_hal_fw_error_event(uint8_t FW_Error_Type, uint8_t Data_Length, uint8_t Data[]);

// Connection information structure
typedef struct {
	uint16_t connection_handle;
	uint8_t  peer_address_type;
	uint8_t  peer_address[6];
	uint16_t conn_interval;
	uint16_t slave_latency;
	uint16_t supervision_timeout;
	uint8_t  master_clock_accuracy;
}connection_info;


#endif /* INC_BLE_USER_MAIN_H_ */
