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

#include "../src/ADC_User.h"
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
	ADC_InitStruct.ADC_Attenuation = ADC_Attenuation_0dB;
	ADC_Init(&ADC_InitStruct);

	// Enable the filter
	ADC_Filter(ENABLE);

	// Enable automatic calibration
	ADC_AutoOffsetUpdate(ENABLE);
	ADC_Calibration(ENABLE);

	// Disable threshold
	ADC_ThresholdCheck(DISABLE);

	return;
}
/*
 * @brief	Polls ADC peripheral to get the current battery voltage
 * @param	None
 * @retval	Battery level, as a percentage
 */

uint8_t ADC_RequestBattery(void){
	long double V, x, Q, rawVal;
	// Poll for ADC to collect data
	ADC_Cmd(ENABLE);
	while(ADC_GetFlagStatus(ADC_FLAG_EOC) != SET){
		;
	}
	// Convert to battery voltage
	rawVal = ADC_GetConvertedData(ADC_Input_BattSensor, ADC_ReferenceVoltage_0V6);
	if(!ADC_SwCalibration()){
		rawVal = ADC_CompensateOutputValue(rawVal);
	}
	/*
	 * Use cubic formula along with battery degradation model to get an approximation of battery level
	 * Model only ever has 1 real root, so solved using hyperbolic trigonometric functions
	 * sources:
	 * https://en.wikipedia.org/wiki/Cubic_equation    (mathematical formulas)
	 * https://data.energizer.com/pdfs/max-eu-aaa.pdf  (battery degradation model)
	 *
	 * Equations:
	 * rawVoltage/2 - .9 = V
	 * V = 0.59859981 - 4.44242639e-2 x + 3.23863752e-03 x^2 + -1.05101869e-04 x^3
	 * x / 23.17956 = battery percentage
	 */
	V = (rawVal / 2 )- .9;
	if (V >= BATTERY_CUBIC_D) {
		return 100;
	}
	else if (V <= 0) {
		return 0;
	}
	// If in bounds, perform battery estimation based on model equation
	Q = ((2*BATTERY_CUBIC_B*BATTERY_CUBIC_B*BATTERY_CUBIC_B) + (-1 * (9*BATTERY_CUBIC_A*BATTERY_CUBIC_B*BATTERY_CUBIC_C)) + (27*BATTERY_CUBIC_A*BATTERY_CUBIC_A*(BATTERY_CUBIC_D-V)))/(27*BATTERY_CUBIC_A*BATTERY_CUBIC_A*BATTERY_CUBIC_A);
	x = ((-2*(sqrtl(BATTERY_CUBIC_P/3))) * (sinhl((asinhl((3*Q/(2*BATTERY_CUBIC_P))*( sqrtl(3)/sqrtl(BATTERY_CUBIC_P))))/3))) + BATTERY_ROOT_OFFSET;
	return (uint8_t)(100*(1-(x / BATTERY_TIME_MAX)));
}
