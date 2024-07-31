/*
 * main.h
 *
 *  Created on: Jul 3, 2024
 *      Author: akj56
 */

#include "BlueNRG2.h"
#include "system_bluenrg1.h"
#include "SDK_EVAL_Config.h"
#include "SPI_User.h"
#include "ADC_User.h"
#include "BlueNRG1_it.h"
#include "misc.h"

#define UART_BAUDRATE 115200

int main(void);
void nextState(uint8_t* debugState);
