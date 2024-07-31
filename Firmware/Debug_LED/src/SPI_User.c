/*
 * SPI_User.c
 *
 ******************************************************************************
 * @file    SPI_User.c
 * @author  akj56
 * @version V1.0.0
 * @date    21 - 05 - 2024
 * @brief   Contains all user-defined SPI operations
 *
 ******************************************************************************
 */

#include "SPI_User.h"

#include <stdint.h>
/*
 * @brief Initializes SPI to all the desired settings in one command, reducing clutter
 * @param none
 * @retval none
 */
void SPI_User_Init(void){

	/*
	 * Assign proper SPI pins, following similar pattern to demo/evaluation board
     */
	GPIO_InitType GPIO_InitStructure;

	GPIO_StructInit(&GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = SPI_CLK;
	GPIO_InitStructure.GPIO_Mode = MODE_SPI_I2C;
	GPIO_InitStructure.GPIO_Pull = ENABLE;
	GPIO_InitStructure.GPIO_HighPwr = DISABLE;
	GPIO_Init(&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = SPI_OUT;
	GPIO_InitStructure.GPIO_Mode = MODE_SPI_I2C;
	GPIO_Init(&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = SPI_IN;
	GPIO_InitStructure.GPIO_Pull = DISABLE;
	GPIO_InitStructure.GPIO_Mode = MODE_SPI_I2C;
	GPIO_Init(&GPIO_InitStructure);

	// Assign chip select pin
	GPIO_InitStructure.GPIO_Pin = SPI_CS;
	GPIO_InitStructure.GPIO_Mode = MODE_GPIO;
	GPIO_InitStructure.GPIO_Pull = ENABLE;
	GPIO_InitStructure.GPIO_HighPwr = DISABLE;
	GPIO_Init(&GPIO_InitStructure);

	// Set CS bit high since not using accelerometer
	//GPIO_SetBits(SPI_CS);

	// Enable pin 10 as the error LED pin
	GPIO_InitStructure.GPIO_Pin = ERROR_LED;
	GPIO_InitStructure.GPIO_Mode = MODE_GPIO;
	GPIO_InitStructure.GPIO_Pull = DISABLE;
	GPIO_InitStructure.GPIO_HighPwr = ENABLE;
	GPIO_Init(&GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = TOGGLE_SWITCH;
	GPIO_InitStructure.GPIO_Mode = GPIO_Input;
	GPIO_InitStructure.GPIO_HighPwr = DISABLE;
	GPIO_InitStructure.GPIO_Pull = ENABLE;
	GPIO_Init(&GPIO_InitStructure);

	GPIO_ResetBits(ERROR_LED);

	/*
	 * Build out the SPI functionality, assigning internal register values
	 */
	SPI_InitType SPI_InitStruct;
	SPI_DeInit();
	// Set baudrate, CPOL, and likewise
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_16b;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStruct.SPI_BaudRate = 1000000;
	SPI_Init(&SPI_InitStruct);

	//SPI_FrameFormatConfig(SPI_FrmFrmt_Motorola);

	// Clear out the FIFO just in case something went wrong
	SPI_ClearRXFIFO();
	SPI_ClearTXFIFO();

	// Set correct communications mode
	SPI_SetMasterCommunicationMode(SPI_FULL_DUPLEX_MODE);

	// Accelerometer FIFO outputs packets of 16 bytes, so set correct number of frames
	//SPI_SetNumFramesToReceive(RECEIVE_FIFO_SIZE);
	//SPI_SetNumFramesToTransmit(2);
	SPI_SetNumFramesToReceive(1);

	// Most significant bit, but least significant byte are received over SPI
	SPI_EndianFormatReception(SPI_ENDIAN_MSByte_MSBit);
	SPI_EndianFormatTransmission(SPI_ENDIAN_MSByte_MSBit);

	// In operational state, SPI FIFO should generate an interrupt when it has data to read
	SPI_RxFifoInterruptLevelConfig(SPI_FIFO_LEV_8);
	SPI_ITConfig(SPI_IT_RX, DISABLE);

	// Raise CS high
	//SPI_SlaveSwSelection(DISABLE);
	GPIO_SetBits(SPI_CS);

	// Enable SPI
	SPI_Cmd(ENABLE);


}

extern uint8_t packet_data[60];
extern uint8_t packet_counter;
/*
 * @brief Sends a request for to the accelerometer to receive a data packet
 */
void Request_Accel_Data(void){
	uint32_t buf;
	SPI_Read(0x3E, &buf);
	if(buf == 0){
		Error(3);
	}
	GPIO_ResetBits(SPI_CS);
	//SPI_SetNumFramesToTransmit(1);
	SPI_SetNumFramesToReceive(RECEIVE_FIFO_SIZE/2+1);
	SPI_SendData(FIFO_DATA_ADDR<<8);
	for(int i = 0; i < RECEIVE_FIFO_SIZE/2; i++){
		SPI_SendData(0);
	}
	// Leaving SPI_CS low as data is sent in
	while(SPI_GetFlagStatus(SPI_FLAG_BSY) == SET);
	GPIO_SetBits(SPI_CS);

	Process_Packet(packet_data+12*packet_counter);
	packet_counter++;

    // If packet is full, send it
    if(packet_counter == 5){
    	packet_counter = 0;
    }
}
/*
 * @brief	Manually gets the accelerometer data from each register, as an alternative to FIFO
 * 			While the code is simpler, not all measurements are at the same timestamp
 */
void Manual_Accel_Data(void){
	uint32_t temp;
	for(int i = 0; i < 12; i++){
		SPI_Read(FIRST_DATA_ADDR+i, &temp);
		packet_data[12*packet_counter + i] = temp;
	}
	packet_counter++;
	if(packet_counter == 5){ // Change to POINTS_PER_PACKET
		packet_counter = 0;
	}
}
// Reworked function
///**
//  * @brief Loads Accelerometer data into data
//  * @param uint8_t* data
//  */
//void Get_Accel_Data(uint8_t* data){
//	uint32_t temp;
//	// We have received all the bits, disable connection to accelerometer
//	GPIO_ResetBits(SPI_CS);
//	// Start reading and parsing into data array
//	for(int i = 0; i < RECEIVE_FIFO_SIZE; i++){
//		data[i] = (uint8_t)SPI_ReceiveData();
//	}
//	GPIO_SetBits(SPI_CS);
//
//}

/*
 * @brief Writes along SPI assuming chip is already selected
 * @param uint8_t address, uint8_t data
 * @ret returns true if operation failed
 */
void SPI_Write(uint32_t address, uint8_t data){
	//SPI_SetNumFramesToTransmit(2);
	//SPI_SetNumFramesToReceive(0);
	GPIO_ResetBits(SPI_CS);
	//Clock_Wait(1);
	SPI_SendData((address<<8) | data);
	while(SPI_GetFlagStatus(SPI_FLAG_BSY) == SET);
	GPIO_SetBits(SPI_CS);
	Clock_Wait(1);
	//SPI_SendData(data);

}


/*
 * @brief Reads along SPI assuming chip is already selected
 * @param uint8_t address, uint8_t* data
 */
void SPI_Read(uint32_t address, uint32_t* data){
	//SPI_SetNumFramesToTransmit(1);
	SPI_ClearRXFIFO();
	SPI_SetNumFramesToReceive(1);
	GPIO_ResetBits(SPI_CS);
	//Clock_Wait(1);
	SPI_SendData((address | ACCEL_READ_BIT)<<8);
	//SPI_SendData(0);
	while(SPI_GetFlagStatus(SPI_FLAG_BSY) == SET);
	//while(SPI_GetFlagStatus(SPI_FLAG_RNE) != SET);
	*data = SPI_ReceiveData();
	GPIO_SetBits(SPI_CS);
	Clock_Wait(1);
}

/**
  * @brief Initializes the desired accelerometer settings
  * @retval int 0 if no error, else error code
  */
void Accel_Init(void){
	uint32_t buf = 0;
	// Configure writing
	//Turn on permanent clock
	SPI_Write(PWR_MGMT0_ADDR, 0x10);
	Clock_Wait(1); // Requires 200 us delay before issuing another write commands
	//Wait for clock to start
	while(buf != 0){
		SPI_Read(MCLK_RDY_ADDR, &buf);
		Clock_Wait(1);
	}
	// Begin writing deep memory FIFO configuration
	SPI_Write(BLK_SEL_W_ADDR, 0x00);
	Clock_Wait(1);
	SPI_Write(MADDR_W_ADDR, 0x01);
	Clock_Wait(1);
	SPI_Write(M_W_ADDR, 0x03);
	Clock_Wait(1);
	// Now write deep memory timer configuration
	SPI_Write(BLK_SEL_W_ADDR, 0x00);
	Clock_Wait(1);
	SPI_Write(MADDR_W_ADDR, 0x00);
	Clock_Wait(1);
	SPI_Write(M_W_ADDR, 0x11);
	Clock_Wait(1);
	SPI_Write(BLK_SEL_W_ADDR, 0x00);
	Clock_Wait(1);
	// Flush FIFO
	SPI_Write(SIGNAL_PATH_RESET_ADDR, 0x04);
	// Make sure it flushed
	buf = 0;
	Clock_Wait(1);
	while(buf != 0){
		SPI_Read(SIGNAL_PATH_RESET_ADDR, &buf);
		Clock_Wait(1);
	}
	// Enable FIFO using stream
	SPI_Write(FIFO_CONFIG1_ADDR, 0x00);

	// Set output data rate for accel and gyro to same as sample rate, and configure accelerometer averaging
	SPI_Write(ACCEL_CONFIG0_ADDR, 0x09);
	SPI_Write(GYRO_CONFIG0_ADDR, 0x09);
	SPI_Write(ACCEL_CONFIG1_ADDR, 0x51);

	// Put accelerometer back into sleep mode
	SPI_Write(PWR_MGMT0_ADDR, 0x00);
	// Setup Complete!
}

/*
 * @brief	Turns on or off the accelerometer and flushes FIFO
 * @param	ENABLE to enable the accelerometer, DISABLE to turn it off
 */
void Enable_Accel(FunctionalState state){
	if(state != DISABLE){
		// Turn on the accelerometer
		SPI_Write(PWR_MGMT0_ADDR, 0x9E);
		Clock_Wait(1);

		// Flush the FIFO
		SPI_Write(SIGNAL_PATH_RESET_ADDR, 0x04);
		uint32_t buf = 0;
		Clock_Wait(1);
		while(buf != 0){
			SPI_Read(SIGNAL_PATH_RESET_ADDR, &buf);
		}
	}
	else{
		// Turn off the accerlerometer
		SPI_Write(PWR_MGMT0_ADDR, 0x00);
		Clock_Wait(1);
	}
}

/*
 * @brief Parses through the packet data and formats important info for bluetooth
 * @param (out) packet to send over bluetooth
 */
void Process_Packet(uint8_t* data){
	int data_index = 0;
	int i;
	uint8_t temp_data;
	// Read packet header
	temp_data = (uint8_t)SPI_ReceiveData();
	// If packet is empty, something is wrong, send error packet
	if((temp_data & 0x80) == 0x80){
		temp_data = 0xFF;
		for(i = 0; i < 12; i++){
			data[i] = temp_data;
		}
	}
	//else packet is good
	else{
		// Next twelve bytes will be the error packet
		for(i = 0; i < 12; i++){
			data[i] = (uint8_t)SPI_ReceiveData();
			if(data[i] != 0x00){
				Error(0);
			}
		}
	}
	// Clear rest of packet
	SPI_ClearRXFIFO();
}

// Display error
void Error(uint8_t ERR_code){
	if(!(ERR_code)){
		// If no error code, output a success blink pattern (3 quick blinks followed by a longer blink)
		for(int i = 0; i < 3; i++){
			GPIO_ResetBits(ERROR_LED);
			Clock_Wait(50);
			GPIO_SetBits(ERROR_LED);
			Clock_Wait(50);
		}
		GPIO_ResetBits(ERROR_LED);
		Clock_Wait(200);
		GPIO_SetBits(ERROR_LED);
		Clock_Wait(200);
		GPIO_ResetBits(ERROR_LED);
		return;
	}
	else{
		// If error, output that error code
		uint8_t frontHex, backHex;
		for(frontHex = ERR_code/16; frontHex > 0; frontHex--){
			GPIO_SetBits(ERROR_LED);
			Clock_Wait(150);
			GPIO_ResetBits(ERROR_LED);
			Clock_Wait(150);
		}
		Clock_Wait(500);
		for(backHex = ERR_code % 16; backHex > 0; backHex--){
			GPIO_SetBits(ERROR_LED);
			Clock_Wait(150);
			GPIO_ResetBits(ERROR_LED);
			Clock_Wait(150);
		}
		return;
	}
}
