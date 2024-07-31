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

void ADC_User_Init(void);
float ADC_RequestBattery(void);

#endif /* INC_ADC_USER_H_ */
