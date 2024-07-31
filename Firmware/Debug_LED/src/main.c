/*
 * main.c
 *
 *  Created on: Jul 3, 2024
 *      Author: akj56
 */
#include "main.h"

uint8_t packet_counter;
uint8_t packet_data[60];

int main(void){
	SystemInit();
	SdkEvalIdentification();
	SdkEvalComUartInit(UART_BAUDRATE);
	Clock_Init();
	SPI_User_Init();
	ADC_User_Init();
	Accel_Init();
	packet_counter = 0;
	uint8_t debugState = 0x00, counter = 0;
	uint32_t buf = 1;

	Error(0);

	Enable_Accel(ENABLE);

	while(1){
		switch(debugState){
		case(0x00):
				// Debug 1, Check LED
			GPIO_SetBits(ERROR_LED);
			Clock_Wait(1000);
			GPIO_ResetBits(ERROR_LED);
			Clock_Wait(1000);
			nextState(&debugState);
			break;
		case(0x01):
				// Debug 2, Check SPI
			buf = 0x00;
			SPI_Write(PWR_MGMT0_ADDR, 0x1F);
			SPI_Read(MCLK_RDY_ADDR, &buf);
			if(buf == 9){
				Error(1);
			}
			if(buf != 9){
				Error(3);
			}
			//SPI_Write(PWR_MGMT0_ADDR, 0x00);
			Clock_Wait(100);
			nextState(&debugState);
			break;
		case(0x02):
				// Debug 3, Check ADC
			buf = (uint8_t)ADC_RequestBattery();
			if(buf >= 1){
				Error(0);
			}
			else{
				Error(1);
			}
			buf = 0;
			Clock_Wait(1000);
			GPIO_SetBits(ERROR_LED);
			Clock_Wait(1000);
			GPIO_ResetBits(ERROR_LED);
			nextState(&debugState);
			Clock_Wait(1000);
			break;
		case(0x03):
					// Debug 4, Check SPI Output
			GPIO_ResetBits(SPI_CS);
			//SPI_SetNumFramesToTransmit(4);
			//SPI_SetNumFramesToReceive(4);
			SPI_Cmd(DISABLE);
			SPI_Cmd(ENABLE);
			SPI_SendData(0xA0C0A0C1);
			GPIO_SetBits(ERROR_LED);
			Clock_Wait(500);
			GPIO_SetBits(SPI_CS);
			GPIO_ResetBits(ERROR_LED);
			Clock_Wait(500);
			nextState(&debugState);
			break;
		case (0x04):
					// Debug 5, read accel data function
			GPIO_ResetBits(SPI_CS);
			Request_Accel_Data();
			GPIO_SetBits(SPI_CS);
			Clock_Wait(500);
			nextState(&debugState);
			break;
		default:
			// Loop back to beggining on default
			debugState = 0x00;
			break;
		}
	}
	return 0;
}
/*
 * @brief 	Checks to advance to next state
 * @param	State to advance
 */
void nextState(uint8_t* debugState){
	if(GPIO_ReadBit(TOGGLE_SWITCH) == Bit_SET){
		// Make sure actually set
		Clock_Wait(100);
		if(GPIO_ReadBit(TOGGLE_SWITCH) == Bit_SET){
			(*debugState)++;

			while(GPIO_ReadBit(TOGGLE_SWITCH == Bit_SET)){
				// Wait to undo switch
				GPIO_SetBits(ERROR_LED);
				Clock_Wait(100);
				if(GPIO_ReadBit(TOGGLE_SWITCH != Bit_SET)){
					break;
				}
				GPIO_ResetBits(ERROR_LED);
				Clock_Wait(100);
			}

		}
	}
}
