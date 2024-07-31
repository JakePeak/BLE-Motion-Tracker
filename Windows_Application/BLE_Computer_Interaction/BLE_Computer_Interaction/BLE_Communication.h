#pragma once
// Mark as included
#include "framework.h"

// Bluetooth GATT/GAP information
#define DEVICE_NAME {L'M', L'o', L't', L'i', L'o', L'n', L' ', L'T', L'r', L'a', L'c', L'k', L'e', L'r', L'\0'}
#define DEVICE_ADDRESS { (char)0xC2,(char)0xBC,(char)0x36,(char)0xAD,(char)0x26,(char)0xC5,(char)0x00 }
#define ACCEL_UUID GUID {0xD03AC8B6, 0x59E9, 0x4E59, {0x84, 0x6A, 0x32, 0xAA, 0x3C, 0xB7, 0x12, 0xBC} }
#define BATTERY_SERVICE_UUID (USHORT)0x180F
#define HID_UUID (USHORT)0x1812
#define BATTERY_UUID (USHORT)0x2A19
#define DATA_UUID (USHORT)0x2A67
#define PAUSE_STATE_UUID (USHORT)0x2AE2
#define READY_UUID GUID {0xD03AC8B9, 0x59E9, 0x4E59, {0x84, 0x6A, 0x32, 0xAA, 0x3C, 0xB7, 0x12, 0xBC} }
//#define REPORT_UUID (USHORT)0x2A4D
#define CCC_UUID (USHORT)0x2902
#define ACCEL_UUID_STRING_LEN 36

// Time it takes for device to have a successful callibration, in seconds
#define CALLIBRATION_TIME 15
// Tolerance of callibration
#define TOLERANCE .05
// Default output CSV name
#define DEFAULT_NAME "BLE_output_file"
#define DEFAULT_NAME_WIDE L"BLE_output_file"
#define EXECUTABLE_NAME L"BLE_Computer_Interaction.exe"
// Optional caps for graphing data to keep accelerometer data reasonable
#define VELOCITY_CAP 10
#define POSITION_CAP 50

// Data Parsing Macros
#define POINTS_PER_PACKET 10
#define SAMPLE_RATE .025
#define CALLIBRATION_PACKETS  ((CALLIBRATION_TIME/SAMPLE_RATE)/POINTS_PER_PACKET)
#define MAX_INT (double)32768
#define GYRO_SCALE (double)2000
#define ACCEL_SCALE (double)16
#define GS_TO_MPS2 (double)9.80665
#define CALLIBRATION_COMPLETE (0x02)


typedef enum _PAUSE_STATE {PAUSE, UNPAUSE} PAUSE_STATE;
typedef enum _FILE_MODE {NONE, PATH, NAME} FILE_MODE;

/*
 * @brief	Struct containing information to be given as context to the callback function.  It
 *			provides information on the previous point, as well as if the last packet was read.
 */
typedef struct {
	bool Callibrated;
	int CallibrationCounter;
	double x;
	double y;
	double z;
	double Vx;
	double Vy;
	double Vz;
	int16_t anglexComp;
	int16_t angleyComp;
	int16_t anglezComp;
	double anglex;
	double angley;
	double anglez;
	double scale;
	USHORT AttributeHandle;
	HANDLE* ServPtr;
	PBTH_LE_GATT_CHARACTERISTIC DataPtr;
	PBTH_LE_GATT_CHARACTERISTIC NotifPtr;
	HANDLE fileHandle;
}BLE_CALLBACK_CONTEXT;

/* 
 * @brief	Primary BLE device class, representing an individual BLE device.  Methods are sepecified
 *			in BLE_Communication.cpp, as well as their documentation and error codes
 */
class BLE_Device{
private:
	HANDLE DevHandle;
	HANDLE ServHandle;
	HANDLE BattServHandle;
	BTH_LE_GATT_SERVICE IMU;
	BTH_LE_GATT_SERVICE BatteryService;
	BTH_LE_GATT_CHARACTERISTIC Battery;
	BTH_LE_GATT_CHARACTERISTIC Data;
	BTH_LE_GATT_CHARACTERISTIC Pause;
	BTH_LE_GATT_CHARACTERISTIC Ready;
	BTH_LE_GATT_DESCRIPTOR CCC;
	BLUETOOTH_GATT_EVENT_HANDLE NotifyHandle;
	BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION* NotifyHandleRegistration;
	BLE_CALLBACK_CONTEXT* CallbackContext;
	std::string* FilePath;

	static void ParseAccelData(int16_t* x, int16_t* y, int16_t* z, UCHAR* data, int offset);
	static int GetExecPath(WCHAR** out);
public:
	int ErrorCode;

	// Standard Member Functions
	BLE_Device(HWND hWnd);
	~BLE_Device();
	int EnterState(PAUSE_STATE State);
	void ClearCSV();
	uint8_t BatteryPercentage();

	// Setter/getter functions
	void SetFilePath(std::string& input, FILE_MODE mode);
	const std::string* GetFilePath() { return FilePath; }		// Outputs current filepath

	// Function callbacks
	static void ReadNotifcationData(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context);
	static void FileIOCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	// copy, default, and copy assignment methods removed
	BLE_Device() = delete;
	BLE_Device(BLE_Device const &device2) = delete;
	void operator=(BLE_Device const &device2) = delete;
};


