/*
 * ADC_User.c
 *
 *******************************************************************************
 * @file    ADC_User.c
 * @author  akj56
 * @version V1.0.0
 * @date    03 - 06 - 2024
 * @brief   Contains all user-defined ADC operations for reading battery level
 *
 ******************************************************************************
 *
 */

#include "ADC_User.h"
/*
 * @brief Initializes ADC to all the desired settings in one command, reducing clutter
 * @param none
 */
void ADC_User_Init(void){
	// Initialize ADC settings as internal battery voltage sensor
	ADC_InitType ADC_InitStruct;
	ADC_InitStruct.ADC_OSR = ADC_OSR_200;
	ADC_InitStruct.ADC_ReferenceVoltage = ADC_ReferenceVoltage_0V6;
	ADC_InitStruct.ADC_ConversionMode = ADC_ConversionMode_Single;
	ADC_InitStruct.ADC_Input = ADC_Input_BattSensor;
	ADC_InitStruct.ADC_Attenuation = ADC_Attenuation_9dB54;
	ADC_Init(&ADC_InitStruct);

	// Enable automatic calibration
	ADC_AutoOffsetUpdate(ENABLE);
	ADC_Calibration(ENABLE);

	// Disable threshold
	ADC_ThresholdCheck(DISABLE);

	// Enable automatic compensation for voltage measurement
	ADC_Filter(ENABLE);

	return;
}
/*
 * @brief	Polls ADC peripheral to get the current battery voltage
 * @param	None
 * @retval	Battery voltage
 */

float ADC_RequestBattery(void){
	int16_t rawVoltage;
	// Get raw data
	rawVoltage = (int16_t)ADC_GetRawData();
	// Convert to battery voltage
	return ADC_ConvertBatterySensor(rawVoltage, ADC_ReferenceVoltage_0V6);
}
