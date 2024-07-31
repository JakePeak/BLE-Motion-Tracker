/*
 *   ******************************************************************************
 * @file    SPI_User.h
 * @author  akj56
 * @version V1.0.0
 * @date    21-05-2024
 * @brief   This file contains the prototypes for user-defined SPI operations
 ******************************************************************************
 *
 */

#ifndef INC_SPI_USER_H_
#define INC_SPI_USER_H_

#include "BlueNRG1_spi.h"
#include "BlueNRG1_gpio.h"
#include "clock.h"
#include "user.h"
#include "BLE_User_main.h"

// Pin operation modes
#define MODE_SPI_I2C		Serial0_Mode
#define MODE_GPIO			GPIO_Output
#define MODE_UART			Serial1_Mode

// Redefine pin names
#define SPI_CS				GPIO_Pin_1
#define SPI_OUT				GPIO_Pin_2
#define SPI_IN				GPIO_Pin_3
#define SPI_CLK				GPIO_Pin_0
#define ERROR_LED			GPIO_Pin_10
#define TOGGLE_SWITCH		GPIO_Pin_7

#define RECEIVE_FIFO_SIZE 	((uint16_t)0x0010)

#define CALIBRATION_COMPLETE 0x02

// Initializes all desired SPI parameters
void SPI_User_Init();

/*
 * Acclerometer Specifications
 */
// For all values, if 0x80 bit is written it means read, if not it means write
#define ACCEL_READ_BIT			((uint32_t)0x080)

static const uint32_t PWR_MGMT0_ADDR = 0x1F;   //Turns on accelerometer and gyroscope
											   //with 0x1F (yes, same)
											   // Alternatively use 0x9E for low power/
											   //sample averaging

static const uint32_t MCLK_RDY_ADDR = 0x00;		 //If 0x04, internal clock is running
											 //Check before altering MREG (below instructions)

static const uint32_t BLK_SEL_W_ADDR = 0x79;   //For FIFO initialization, set value to 0x00
static const uint32_t MADDR_W_ADDR = 0x7A;     //Then set value to 0x01 to access FIFO_CONFIG5,
											 //set to 0x00 to access TMST_CONFIG1

static const uint32_t M_W_ADDR = 0x7B;         //set FIFO_CONFIG5 to 0x03 and wait for 10 uS
											 //set TMST_CONFIG1 to 0x11 and wait for 10 uS

static const uint32_t SIGNAL_PATH_RESET_ADDR = 0x02;  //To flush FIFO, set to 0x04 and wait 1.5uS
											        //to confirm flush then it should read 0x00

static const uint32_t FIFO_CONFIG1_ADDR = 0x28;//To enable FIFO using stream, set to 0x00

static uint32_t FIFO_DATA_ADDR = 0xBF;        //Instruction to read FIFO data (reg. 0x3F)

static const uint32_t ACCEL_CONFIG0_ADDR = 0x21; // Configure output data rate, 0x09 for 100hz
static const uint32_t GYRO_CONFIG0_ADDR = 0x20;  // Configure output data rate, 0x09 for 100hz

static const uint32_t ACCEL_CONFIG1_ADDR = 0x24; // Configure accelerometer LP averaging, set to 0x51

static const uint32_t FIRST_DATA_ADDR = 0x0B;  // Location of first data register in accelerometer

static const uint32_t DATA_READY_ADDR = 0x39;  // Will be 0x01 if there is new data
/*
static const uint32_t ACCEL_ADDR = 0xD0;       //0xD0 for read, 0xD1 for write
*/
// Used to reference accelerometer using I2C

void Manual_Accel_Data(void);
void SPI_Write(uint32_t address, uint8_t data);
void SPI_Read(uint32_t address, uint32_t* data);
void Accel_Init(void);
void Enable_Accel(FunctionalState state);
void Read_Accel(long long int* runningSum, int* numReads);
void Average_Accel(long long int* runningSum, int* numReads, uint8_t* output);
void Error(uint8_t ERR_code);


#endif /* INC_SPI_USER_H_ */
