/*
 *   ******************************************************************************
 * @file    ADC_User.h
 * @author  akj56
 * @version V1.0.0
 * @date    03-06-2024
 * @brief   This file contains the prototypes for user-defined ADC operations
 ******************************************************************************
 *
 */

#ifndef INC_ADC_USER_H_
#define INC_ADC_USER_H_

#include <stdint.h>
#include "BLueNRG1_adc.h"
#include "clock.h"
#include "math.h"

// Battery voltage math Macros
#define BATTERY_CUBIC_A (long double)(-.000105101869)
#define BATTERY_CUBIC_B (long double)(0.00323863752)
#define BATTERY_CUBIC_C (long double)(-0.0444242639)
#define BATTERY_CUBIC_D (long double)(0.59859981)
#define BATTERY_CUBIC_P (long double)(((3*BATTERY_CUBIC_A*BATTERY_CUBIC_C) - (BATTERY_CUBIC_B * BATTERY_CUBIC_B))/(3*BATTERY_CUBIC_A*BATTERY_CUBIC_A))
#define BATTERY_ROOT_OFFSET	(long double)((-1)*(BATTERY_CUBIC_B/(3*BATTERY_CUBIC_A)))
#define BATTERY_TIME_MAX (long double)(23.17956)

void ADC_User_Init(void);
uint8_t ADC_RequestBattery(void);

#endif /* INC_ADC_USER_H_ */
