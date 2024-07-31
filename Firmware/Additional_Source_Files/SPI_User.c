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
	SPI_InitStruct.SPI_BaudRate = 2000000;
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

extern uint8_t packet_data[12*POINTS_PER_PACKET];
extern uint8_t packet_counter;
extern uint8_t calibrating;
extern connection_info conn_info;
extern GATT_Handles accel_gatt_handles;

/*
 * @brief	Manually gets the accelerometer data from each register, as an alternative to FIFO
 * 			While the code is simpler, not all measurements are at the same timestamp
 */
void Manual_Accel_Data(void){
	packet_counter++;
	if(packet_counter == POINTS_PER_PACKET){
		if(aci_gatt_update_char_value_ext(conn_info.connection_handle, accel_gatt_handles.service_handle, accel_gatt_handles.char_handle, 0x00, 12*POINTS_PER_PACKET, 0, 12*POINTS_PER_PACKET, packet_data) != BLE_STATUS_SUCCESS){
			Error(2);
		}
		// Notify peripheral device
		uint8_t update = 1;
		if(aci_gatt_update_char_value_ext(conn_info.connection_handle, accel_gatt_handles.service_handle, accel_gatt_handles.char_ready_handle, 0x01, 1, 0, 1, &update)){
			Error(3);
		}
		packet_counter = 0;
		// If previous packet hasn't been read, report error through holding LED high
		if(calibrating == 0x00){
			GPIO_SetBits(ERROR_LED);
		}
		else if (calibrating == CALIBRATION_COMPLETE){
		// Once a packet has been read, stop with the LED
			GPIO_ResetBits(ERROR_LED);
			calibrating = 0x01;
		}
		// After first calibration call, do nothing
	}
}

/*
 * @brief Writes along SPI assuming chip is already selected
 * @param uint8_t address, uint8_t data
 * @ret returns true if operation failed
 */
void SPI_Write(uint32_t address, uint8_t data){
	GPIO_ResetBits(SPI_CS);
	SPI_SendData((address<<8) | data);
	while(SPI_GetFlagStatus(SPI_FLAG_BSY) == SET);
	GPIO_SetBits(SPI_CS);


}


/*
 * @brief Reads along SPI assuming chip is already selected
 * @param uint8_t address, uint8_t* data
 */
void SPI_Read(uint32_t address, uint32_t* data){
	SPI_ClearRXFIFO();
	SPI_SetNumFramesToReceive(1);
	GPIO_ResetBits(SPI_CS);
	SPI_SendData((address | ACCEL_READ_BIT)<<8);
	while(SPI_GetFlagStatus(SPI_FLAG_BSY) == SET);
	*data = SPI_ReceiveData();
	GPIO_SetBits(SPI_CS);
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
		SPI_Write(PWR_MGMT0_ADDR, 0x10);
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
	Clock_Wait(1);

	// Set output data rate for accel and gyro to same as sample rate, and configure accelerometer averaging
	SPI_Write(ACCEL_CONFIG0_ADDR, 0x05);
	Clock_Wait(1);
	SPI_Write(GYRO_CONFIG0_ADDR, 0x05);
	Clock_Wait(1);
	SPI_Write(ACCEL_CONFIG1_ADDR, 0x51);
	Clock_Wait(1);

	// Put accelerometer back into sleep mode
	SPI_Write(PWR_MGMT0_ADDR, 0x00);
	Clock_Wait(1);
	// Setup Complete!
}

/*
 * @brief	Turns on or off the accelerometer and flushes FIFO
 * @param	ENABLE to enable the accelerometer, DISABLE to turn it off
 */
void Enable_Accel(FunctionalState state){
	if(state != DISABLE){
		// Turn on the accelerometer
		SPI_Write(PWR_MGMT0_ADDR, 0x1F);
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
		// Turn off the accelerometer
		SPI_Write(PWR_MGMT0_ADDR, 0x00);
		Clock_Wait(1);
	}
}

/*
 * @brief	Reads the accelerometer if new data is available
 * @param	int* runningSum => Integer array containing 12 integers, one for each data output
 * @param	int* numReads   => External counter used to count number of packets for averaging
 */
void Read_Accel(long long int* runningSum, int* numReads){
	uint32_t temp;
	SPI_Read(DATA_READY_ADDR, &temp);
	// If there is new data to read, read it and update the averages
	if((temp & 0x01) == 0x01){
		*numReads += 1;
		for(int i = 0; i < 12; i++){
			SPI_Read(FIRST_DATA_ADDR+i, &temp);
			runningSum[i] += temp;
		}
	}
}

/*
 * @brief	Averages all of the collected accelerometer data
 * @param	runningSum	=> Array of register values summed together
 * @param	numReads	=> number of times acclerometer has been read
 * @param	output		=> Byte output array to send over GATT
 */
void Average_Accel(long long int* runningSum, int* numReads, uint8_t* output){
	long long int temp;
	// Go through all 6 axis of motion
	for(int i = 0; i < 12; i += 2){
		// merge both lower and upper byte sums
		temp = runningSum[i];
		temp <<= 8;
		temp += runningSum[i+1];
		// Average them
		temp /= *numReads;
		// Load lower and upper bytes into output matrix
		if((temp & 0x080) == 0){
			output[i+1] = temp & 0x0FF;
		}
		else{
			output[i+1] = (temp & 0x0FF) | 0xFF00;  // Reduces typecast ambiguity
		}
		temp >>= 8;
		output[i] = temp & 0x0FF;
		// Clear out the running sum
		runningSum[i] = 0;
		runningSum[i+1] = 0;
	}
	*numReads = 0;
}

/*
 * @brief	Error function, takes an error code and flashes the led in the pattern of it's two
 * 			digit hex code.  If no error, flashes 3 quick flashes followed by a longer flash
 */
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
