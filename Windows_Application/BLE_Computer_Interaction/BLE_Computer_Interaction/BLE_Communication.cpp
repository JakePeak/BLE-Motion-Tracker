#include "framework.h"
#include "BLE_Communication.h"

static const WCHAR ServiceUUID[] = L"d03ac8b6-59e9-4e59-846a-32aa3cb712bc";
static const WCHAR BattServUUID[] = L"0000180f-0000-1000-8000-00805f9b34fb";

/*
 * @brief Gets all necessary handles for device communication, initializing the device
 * @retval bit map signifying what errors occured in ErrorCode
 *		0x0001 => SetupDiGetClassDevsW error, likely no connected BLE devices
 *		0x0002 => SetupDiGetDeviceInterfaceDetail error
 *		0x0004 => If any device has an address larger than 6 bytes will return error
 *		0x0008 => Handle creation error occured
 *		0x0010 => Error destroying device information set, either already destroyed or memleak
 *		Further bitmaps are explained at location
 */
BLE_Device::BLE_Device(HWND hWnd) {

	ErrorCode = 0;
	IMU = {};
	BatteryService = {};
	Battery = {};
	Data = {};
	Pause = {};
	Ready = {};
	CCC = {};
	NotifyHandle = {};
	NotifyHandleRegistration = NULL;
	DevHandle = NULL;
	ServHandle = NULL;
	BattServHandle = NULL;
	FilePath = NULL;
	HDEVINFO BLEDevInterfaces = NULL;
	HDEVINFO BLEServInterfaces = NULL;
	SP_DEVINFO_DATA DeviceData = {};
	SP_DEVICE_INTERFACE_DATA InterfaceInfo = {};
	SP_DEVICE_INTERFACE_DETAIL_DATA_W *DetailedInfo = NULL;
	WCHAR Name[15] = {};
	WCHAR Test_Name[15] = DEVICE_NAME;
	SECURITY_ATTRIBUTES NoSecurity = { sizeof(SECURITY_ATTRIBUTES), NULL, true };
	CallbackContext = NULL;

	// Gather list of all connected BLE devices
	BLEDevInterfaces = SetupDiGetClassDevsW(&GUID_BLUETOOTHLE_DEVICE_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (BLEDevInterfaces == INVALID_HANDLE_VALUE) {
		// Handle error, perhaps no connected devices?
		ErrorCode |= 0x0001;
		DevHandle = NULL;
		return;
	}
	// Gather list of all connected BLE services (for later use)
	BLEServInterfaces = SetupDiGetClassDevsW(&GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (BLEServInterfaces == INVALID_HANDLE_VALUE) {
		// Handle error, perhaps no connected devices?
		ErrorCode |= 0x0001;
		ServHandle = NULL;
		return;
	}

	DeviceData.cbSize = sizeof(SP_DEVINFO_DATA);
	InterfaceInfo.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	DWORD tempSize = 0;
	// Enumerate over all of the BLE device interfaces, looking for ours
	OutputDebugStringW(L"\nEnum\n");
	for (int i = 0; SetupDiEnumDeviceInterfaces(BLEDevInterfaces, NULL, &GUID_BLUETOOTHLE_DEVICE_INTERFACE, i, &InterfaceInfo); i++) {
		// Get the hardware path to the interface and get the data of the device
		SetupDiGetDeviceInterfaceDetailW(BLEDevInterfaces, &InterfaceInfo, NULL, 0, &tempSize, NULL);
		DetailedInfo = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)malloc(tempSize);
		if (DetailedInfo == NULL) {
			SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
			ErrorCode |= 0x010000;
			return;
		}
		DetailedInfo->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

		if (!(SetupDiGetDeviceInterfaceDetailW(BLEDevInterfaces, &InterfaceInfo, DetailedInfo, tempSize, NULL, &DeviceData))) {
			// Error getting information on the interface
			ErrorCode |= 0x0002;
		}
		// Test the device data to see if it is our device via device friendly name "Motion Tracker"
		if (!(SetupDiGetDeviceRegistryPropertyW(BLEDevInterfaces, &DeviceData, SPDRP_FRIENDLYNAME, NULL, (PBYTE)Name, 15 * sizeof(WCHAR), NULL))) {
			// Error, could be wrong address size
			ErrorCode |= 0x0004;
		}
		OutputDebugStringW(L"\nDevice\n");
		OutputDebugStringW(Name);
		OutputDebugStringW(L"\n");
		// Expecting 14 bytes, clearing the fifthteenth to allow for strcmp
		Name[14] = 0x00;
		// If device has been found, we are at the correct interface.  Else keep looking
		if (wcscmp(Name, Test_Name) == 0) {
			// If a device has already been found, do nothing
			if (DevHandle != NULL) {
				;
			}
			// Create and export the file handle for use with BLE IO
			else if ((DevHandle = CreateFileW(DetailedInfo->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &NoSecurity, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
				ErrorCode |= 0x0008;
				DevHandle = NULL;
				SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
				return;
			}
			else {
				break;
			}
		}
		free(DetailedInfo);
		DetailedInfo = NULL;

	}
	free(DetailedInfo);
	DetailedInfo = NULL;
	if (DevHandle == NULL) {
		SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
		if (ErrorCode == 0) {
			ErrorCode |= 0x20000;  // Error no connected devices
		}
		return;
	}
	else {
		ErrorCode = 0;
	}
	OutputDebugStringW(L"\nPost_Enum\n");
	BTH_LE_GATT_SERVICE *ServiceBuf;
	USHORT Real_Buf_Size;
	HRESULT FuncReturn;
	// Now that you have the handle, enable set up notifications and get all various handles

	//*****************************************************
	// Now Find Services
	//*****************************************************

	// Create correct buffer for all device services
	BluetoothGATTGetServices(DevHandle, 0, NULL, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE);
	ServiceBuf = (PBTH_LE_GATT_SERVICE)calloc(Real_Buf_Size, sizeof(BTH_LE_GATT_SERVICE));
	if (ServiceBuf == NULL) {
		ErrorCode |= 0x10000;
		SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
		return;
	}
	tempSize = Real_Buf_Size;

	// Get the correct services
	if( (FuncReturn = BluetoothGATTGetServices(DevHandle, tempSize, ServiceBuf, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
		// Handle error
		if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
			ErrorCode |= 0x0020; // No access
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER) || FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_FUNCTION)) {
			ErrorCode |= 0x0040; // To little buffer space or no services
		}
		else {
			ErrorCode |= 0x0080; // Massive data error
		}
	}
	// Find the service(s) you want
	for (int i = 0; i < Real_Buf_Size; i++) {
		if (!ServiceBuf[i].ServiceUuid.IsShortUuid) {
			if (ServiceBuf[i].ServiceUuid.Value.LongUuid == ACCEL_UUID) {
				IMU = ServiceBuf[i];
			}
			// Add more long services to identify 
		}
		else {
			if (ServiceBuf[i].ServiceUuid.Value.ShortUuid == BATTERY_SERVICE_UUID) {
				BatteryService = ServiceBuf[i];
			}
			// Add more short services to identify
		}
	}

	//********************************************
	// Now find the characterstic(s)
	//********************************************
	

	// IMU Characteristics

	// Create buffer
	BTH_LE_GATT_CHARACTERISTIC* CharBuf;
	BluetoothGATTGetCharacteristics(DevHandle, &IMU, 0, NULL, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE);
	CharBuf = (PBTH_LE_GATT_CHARACTERISTIC)malloc(Real_Buf_Size * sizeof(BTH_LE_GATT_CHARACTERISTIC));
	if (CharBuf == NULL) {
		ErrorCode |= 0x10000;
		SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
		return;
	}
	tempSize = Real_Buf_Size;

	// Get all characteristics
	if ((FuncReturn = BluetoothGATTGetCharacteristics(DevHandle, &(IMU), tempSize, CharBuf, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
		// Error occured
		if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
			ErrorCode |= 0x0020; // No access
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER)) {
			ErrorCode |= 0x0100; // Buffer space error
		}
		else {
			ErrorCode |= 0x0200; // Memory error
		}
	}

	// Look for the correct characteristics
	for (int i = 0; i < Real_Buf_Size; i++) {
		if (!CharBuf[i].CharacteristicUuid.IsShortUuid) {
			if (CharBuf[i].CharacteristicUuid.Value.LongUuid == READY_UUID) {
				Ready = CharBuf[i];
			}
			// Add more long characterstics to identify here
		}
		else {
			if (CharBuf[i].CharacteristicUuid.Value.ShortUuid == DATA_UUID) {
				Data = CharBuf[i];
			}
			else if (CharBuf[i].CharacteristicUuid.Value.ShortUuid == PAUSE_STATE_UUID) {
				Pause = CharBuf[i];
			}
			// Add more short characteristics to identify here
		}
	}
	free(CharBuf);
	CharBuf = NULL;

	// Battery Characteristics

	// Create buffer
	BluetoothGATTGetCharacteristics(DevHandle, &BatteryService, 0, NULL, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE);
	CharBuf = (PBTH_LE_GATT_CHARACTERISTIC)malloc(Real_Buf_Size * sizeof(BTH_LE_GATT_CHARACTERISTIC));
	if (CharBuf == NULL) {
		ErrorCode |= 0x10000;
		SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
		return;
	}
	tempSize = Real_Buf_Size;

	// Get all characteristics
	if ((FuncReturn = BluetoothGATTGetCharacteristics(DevHandle, &BatteryService, tempSize, CharBuf, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
		// Error occured
		if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
			ErrorCode |= 0x0020; // No access
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER)) {
			ErrorCode |= 0x0100; // Buffer space error
		}
		else {
			ErrorCode |= 0x0200; // Memory error
		}
	}

	// Look for the correct characteristics
	for (int i = 0; i < Real_Buf_Size; i++) {
		if (CharBuf[i].CharacteristicUuid.IsShortUuid) {
			if (CharBuf[i].CharacteristicUuid.Value.ShortUuid == BATTERY_UUID) {
				Battery = CharBuf[i];
			}
		}
		// Add more characteristics to identify here
	}
	free(CharBuf);
	CharBuf = NULL;


	//*****************************************************************
	// Now find the necessary characteristic descriptions
	//*****************************************************************


	BTH_LE_GATT_DESCRIPTOR *DescBuf;
	BluetoothGATTGetDescriptors(DevHandle, &Ready, 0, NULL, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE);
	DescBuf = (PBTH_LE_GATT_DESCRIPTOR)malloc(Real_Buf_Size * sizeof(BTH_LE_GATT_DESCRIPTOR));
	if (DescBuf == NULL) {
		ErrorCode |= 0x10000;
		SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
		return;
	}
	tempSize = Real_Buf_Size;

	if ((FuncReturn = BluetoothGATTGetDescriptors(DevHandle, &Ready, tempSize, DescBuf, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
		// Error occured
		if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
			ErrorCode |= 0x0020; // No access
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER)) {
			ErrorCode |= 0x0400; // Buffer space error
		}
		else {
			ErrorCode |= 0x0800; // Memory error
		}
	}

	// And look for the correct ones
	for (int i = 0; i < Real_Buf_Size; i++) {
		if (DescBuf[i].DescriptorUuid.IsShortUuid) {
			if (DescBuf[i].DescriptorUuid.Value.ShortUuid == CCC_UUID) {
				CCC = DescBuf[i];
			}
		}
		// Add more descriptors to identify here
	}
	free(DescBuf);


	//**************************************
	// Now setup notifications
	//**************************************


	// Start by creating/initializing the struct to be passed as context to the callback function
	CallbackContext = (BLE_CALLBACK_CONTEXT*)malloc(sizeof(BLE_CALLBACK_CONTEXT));
	// Then register the notifications
	NotifyHandleRegistration = (BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION*)malloc(sizeof(BLUETOOTH_GATT_VALUE_CHANGED_EVENT_REGISTRATION));
	if ( (NotifyHandleRegistration != NULL) && (CallbackContext != NULL) ) {
		*CallbackContext = { false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, Ready.AttributeHandle, &ServHandle, &Data, &Ready, NULL };
		NotifyHandleRegistration->NumCharacteristics = 1;
		NotifyHandleRegistration->Characteristics[0] = Ready;

		//*********************************************************************************************************
		// Now must get service handle of the HID service through the BLE Enum system, like the device handle
		//*********************************************************************************************************

		WCHAR ServName[ACCEL_UUID_STRING_LEN + 1] ;
		WCHAR* NameLoc;
		OutputDebugStringW(L"\nEnum 2\n");
		for (int i = 0; SetupDiEnumDeviceInterfaces(BLEServInterfaces, NULL, &GUID_BLUETOOTH_GATT_SERVICE_DEVICE_INTERFACE, i, &InterfaceInfo); i++) {
			// Get the hardware path to the interface and get the data of the service
			SetupDiGetDeviceInterfaceDetailW(BLEServInterfaces, &InterfaceInfo, NULL, 0, &tempSize, NULL);
			DetailedInfo = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)malloc(tempSize);
			if (DetailedInfo == NULL) {
				SetupDiDestroyDeviceInfoList(BLEServInterfaces);
				SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
				ErrorCode |= 0x010000;
				return;
			}
			DetailedInfo->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
			
			if (!(SetupDiGetDeviceInterfaceDetailW(BLEServInterfaces, &InterfaceInfo, DetailedInfo, tempSize, NULL, &DeviceData))) {
				// Error getting information on the interface
				ErrorCode |= 0x0002;
			}

			// Get the UUID of the service
			if ((NameLoc = wcschr(DetailedInfo->DevicePath, L'{')) == NULL) {
				free(DetailedInfo);
				DetailedInfo = NULL;
				continue;
			}

			// Copy into buffer
			NameLoc++;
			for (int i = 0; i < ACCEL_UUID_STRING_LEN; i++) {
				ServName[i] = NameLoc[i];
			}
			ServName[ACCEL_UUID_STRING_LEN] = L'\0';


			OutputDebugStringW(L"\nService\n");
			OutputDebugStringW(ServName);
			OutputDebugStringW(L"\n");
			// See if is custom service
			if (wcscmp(ServName, ServiceUUID) == 0 && ServHandle == NULL) { 

				// Create and export the file handle for use with BLE IO
				if ((ServHandle = CreateFileW(DetailedInfo->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &NoSecurity, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
					ErrorCode |= 0x0008;
					ServHandle = NULL;
					SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
					SetupDiDestroyDeviceInfoList(BLEServInterfaces);
					free(DetailedInfo);
					return;
				}
			}
			// See if is battery service
			if (wcscmp(ServName, BattServUUID) == 0 && BattServHandle == NULL) {

				// Create and export file handle
				if ((BattServHandle = CreateFileW(DetailedInfo->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &NoSecurity, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
					ErrorCode |= 0x0008;
					ServHandle = NULL;
					SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
					SetupDiDestroyDeviceInfoList(BLEServInterfaces);
					free(DetailedInfo);
					return;
				}
			}

			free(DetailedInfo);
			DetailedInfo = NULL;

		}
		free(DetailedInfo);
		if (ServHandle == NULL || BattServHandle == NULL) {
			SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
			SetupDiDestroyDeviceInfoList(BLEServInterfaces);
			if (ErrorCode == 0) {
				ErrorCode |= 0x20000;  // Error no connected devices
			}
			return;
		}
		OutputDebugStringW(L"\nPost_Enum\n");


		if ((FuncReturn = BluetoothGATTRegisterEvent(ServHandle, CharacteristicValueChangedEvent, NotifyHandleRegistration, 
			(PFNBLUETOOTH_GATT_EVENT_CALLBACK)BLE_Device::ReadNotifcationData, CallbackContext, &(NotifyHandle), BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
			// Error
			if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
				ErrorCode |= 0x0020;  // No access
			}
			else {
				ErrorCode |= 0x1000;  // Invalid function paramter
				//ErrorCode = FuncReturn;
				SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
				return;
			}
		}
	}
	// If a memory allocation failed before getting the handles
	else {
		ErrorCode |= 0x010000;
		SetupDiDestroyDeviceInfoList(BLEDevInterfaces);
		SetupDiDestroyDeviceInfoList(BLEServInterfaces);
		return;
	}


	// Change the value of the CCC to subscribe to notifications
	USHORT CCC_Size;
	// Get the descriptor value and have it fail to give correct size needed
	if ((FuncReturn = BluetoothGATTGetDescriptorValue(ServHandle, &(CCC), 0, NULL, &CCC_Size, BLUETOOTH_GATT_FLAG_NONE)) != HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
		ErrorCode |= 0x2000;  // Error getting buffer size
	}
	if (CCC_Size != 0) {
		BTH_LE_GATT_DESCRIPTOR_VALUE* CCC_Val = (BTH_LE_GATT_DESCRIPTOR_VALUE*)malloc(CCC_Size);
		if (CCC_Val == NULL) {
			ErrorCode |= 0x010000; // Malloc error
		}
		else if(CCC_Val != NULL) {
			if ((FuncReturn = BluetoothGATTGetDescriptorValue(ServHandle, &(CCC), (ULONG)CCC_Size, CCC_Val, &Real_Buf_Size, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
				ErrorCode |= 0x4000;  // Many different possible error codes, debug separate if necessary
			}
			else {
				CCC_Val->ClientCharacteristicConfiguration.IsSubscribeToNotification = TRUE;
				CCC_Val->ClientCharacteristicConfiguration.IsSubscribeToIndication = FALSE;
				if ((FuncReturn = BluetoothGATTSetDescriptorValue(ServHandle, &(CCC), CCC_Val, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
					if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
						ErrorCode |= 0x0020;  // Access Denied
					}
					else {
						ErrorCode |= 0x08000;  // Once again, abundance of error codes, but debug this function
					}
				}
			}
		// Release dynamic memory
		free(CCC_Val);
		}
	}
	else {
		ErrorCode |= 0x2000; // Final potential buffer size error, buffer size given was zero
	}

	if (!(SetupDiDestroyDeviceInfoList(BLEDevInterfaces)) || !(SetupDiDestroyDeviceInfoList(BLEServInterfaces))) {
		// Failed to destroy the device information set
		ErrorCode |= 0x0010;
	}

	//******************************************************************************************
	// Now Initialize Higher-Level application requirements/settings for the device
	//******************************************************************************************
	std::string funcCallString = {};
	SetFilePath(funcCallString, NONE);
	ClearCSV();
	EnterState(PAUSE);
}

/* 
 * @brief Sends a command to pause or unpause the device
 * @param PAUSE_STATE State command whether to pause or unpause
 *		PAUSE => pause the device if appropriate
 *		UNPAUSE => unpause the device if appropriate
 * @retval bit map signifying what errors occured (find locations in code)
 */
int BLE_Device::EnterState(PAUSE_STATE State) {

	ErrorCode = 0;
	HRESULT FuncReturn;
	USHORT BufSize = 0;
	// Get the correct buffer size of the device state
	if ((FuncReturn = BluetoothGATTGetCharacteristicValue(ServHandle, &(Pause), 0, NULL, &BufSize, BLUETOOTH_GATT_FLAG_NONE)) != HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
		if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
			ErrorCode |= 0x0001;  // Access Denied error code
			return ErrorCode;
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER)) {
			; // No error, functioning as intended
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_INVALID_HANDLE)) {
			ErrorCode |= 0x0002;  // Bluetooth initialization error, handle doesn't work
			return ErrorCode;
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_READ_NOT_PERMITTED)) {
			ErrorCode |= 0x0400;  // No read access
			return ErrorCode;
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_ATTRIBUTE_NOT_LONG)) {
			ErrorCode |= 0x0800;  // Likely a systematic error, will need to find a new API
			return ErrorCode;
		}
		else {
			ErrorCode |= 0x0004;  // Complex error
			return ErrorCode;
		}
	}

	// Read current device state
	if (BufSize != 0) {
		BTH_LE_GATT_CHARACTERISTIC_VALUE* PauseState = (BTH_LE_GATT_CHARACTERISTIC_VALUE*)malloc(BufSize);
		if (PauseState == NULL) {
			ErrorCode |= 0x0008;  // Memory allocation error
			return ErrorCode;
		}
		if ((FuncReturn = BluetoothGATTGetCharacteristicValue(ServHandle, &(Pause), (ULONG)BufSize, PauseState, NULL, BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
			if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
				ErrorCode |= 0x0010; // Access Denied
			}
			else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER)) {
				ErrorCode |= 0x0020;  // Buffer was wrong size, issue with previous function
			}
			else {
				ErrorCode |= 0x0040; // Complex error
			}
		}
		/*
		 * Test current state
		 * 0x00 => in Active state, write to 0x01 on state PAUSE
		 * 0x01 => in Pause state, write to 0x00 on state UNPAUSE
		 * 
		 * Else return error that in an inapropriate state
		 */
		if (( (*(PauseState->Data) == 0x00 && State == PAUSE) || (*(PauseState->Data) == 0x01 && State == UNPAUSE) ) && ErrorCode == 0) {
			// If in a valid transition, perform it
			*(PauseState->Data) ^= 0x01;
			if ((FuncReturn = BluetoothGATTSetCharacteristicValue(ServHandle, &(Pause), PauseState, NULL,BLUETOOTH_GATT_FLAG_NONE)) != S_OK) {
				if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
					ErrorCode |= 0x0080; // Once again, access denied
				}
				else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER)) {
					ErrorCode |= 0x0100; // An above function likely went wrong
				}
				else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_WRITE_NOT_PERMITTED)) {
					ErrorCode |= 0x0200; // Don't have write access
				}
				else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_ATTRIBUTE_NOT_LONG)) {
					ErrorCode |= 0x0800; // API cannot write this descriptor, code will never work on this API
				}
				else {
					ErrorCode |= 0x1000; // Unpredicted error
				}
			}
		}
		// If in an inapropriate state, return an error code
		else if ((*(PauseState->Data) & 0xFE) != 0) {
			ErrorCode |= 0x2000;
		}
		// If not in active state or error occured, operation failed, do not try and write
		free(PauseState);
	}
	return ErrorCode;
}

/*
 * @brief	Gets the CSV file path and opens the file, updating relevant class members
 * @param	string input => input string provided, use depends on next argument
 * @param	FILE_MODE mode => enum describing input string
 *		NONE => unimportant, using default settings
 *		NAME => file name, without '\' or ".csv"
 *		PATH => full path to document (document likely already exists)
 */
void BLE_Device::SetFilePath(std::string& input, FILE_MODE mode) {
	// Clear out old file if present
	if (FilePath != NULL) {
		delete FilePath;
	}
	// If a full path is provided, use it
	if (mode == PATH) {
		FilePath = new std::string(input);
		CallbackContext->fileHandle = CreateFileA(FilePath->c_str(), GENERIC_READ | GENERIC_WRITE | FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	}
	// Else get the current directory and use that to create the path
	else if (mode == NAME || mode == NONE) {
		// Get the current directory into a string
		WCHAR* buf;
		int buf_size = GetExecPath(&buf);
		char* outBuf = (char*)malloc((buf_size+1) * sizeof(char));
		// Append the string with the filename
		if (buf != NULL && outBuf != NULL) {
			wcstombs_s(NULL, outBuf, (buf_size+1) * sizeof(char), buf, (buf_size+1) * sizeof(WCHAR));
			FilePath = new std::string(outBuf);
			if (mode == NAME) {
				*FilePath += input;
			}
			else {
				*FilePath += DEFAULT_NAME;
			}
			*FilePath += ".csv";
			// On windows to allow for long style path
			//FilePath->insert(0, "\\\\?\\");
			// open and create output file
			CallbackContext->fileHandle = CreateFileA(FilePath->c_str(), GENERIC_READ | GENERIC_WRITE | FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
		}
		free(buf);
		free(outBuf);
	}
	
}

/*
 * @brief	Clears the current CSV document's data, useful for repeated trials
 * @retval	Various codes depending on success of the operation, located in ErrorCode public member
 *		0 => Success!
 *		1 => Memory Allocation Error
 *			Else is the system file creation error code
 */
void BLE_Device::ClearCSV() {
	ErrorCode = 0;

	// Recreate the file path within the system
	WCHAR* wideFilePath = (WCHAR*)malloc((FilePath->size()+1) * sizeof(WCHAR));
	if (wideFilePath == NULL) {
		ErrorCode |= 1;
		return;
	}
	mbstowcs_s(NULL, wideFilePath, FilePath->size()+1, FilePath->c_str(), (size_t)(FilePath->size() + 1));
	WCHAR* preppendedPath = (WCHAR*)malloc((FilePath->size() + 6) * sizeof(WCHAR));
	if (preppendedPath == NULL) {
		ErrorCode |= 1;
		free(wideFilePath);
		return;
	}
	// Path manipulation for windows file systems to remove file length requirements
	wcscpy_s(preppendedPath, FilePath->size() + 6, L"\\\\?\\");
	wcscat_s(preppendedPath, FilePath->size()+6, wideFilePath);

	if (CallbackContext->fileHandle != NULL) {
		CloseHandle(CallbackContext->fileHandle);
	}
	// Clear or create the file
	CallbackContext->fileHandle = CreateFileW(preppendedPath, GENERIC_READ | GENERIC_WRITE | FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_FLAG_OVERLAPPED, NULL);
	free(wideFilePath);
	free(preppendedPath);

	// Getting error code from CreateFile
	DWORD Error;
	Error = GetLastError();
	if (Error != HRESULT_FROM_WIN32(ERROR_SUCCESS) && Error != HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
		ErrorCode = (int)Error;
	}
	
	// Mark device as uncallibrated
	CallbackContext->Callibrated = false;
	CallbackContext->CallibrationCounter = 0;

}

/*
 * @brief	Sends a read request to the device to read the battery voltage characteristic
 * @retval	Returns battery percentage as a float
 * @retval	Outputs various error codes in the ErrorCode member, as defined in the code below
 */
uint8_t BLE_Device::BatteryPercentage() {
	ErrorCode = 0;
	uint8_t retVal = 0;
	HRESULT FuncReturn;
	USHORT bufSize = 0;
	// Get size of data buffer
	if ((FuncReturn = BluetoothGATTGetCharacteristicValue(BattServHandle, &Battery, 0, NULL, &bufSize, BLUETOOTH_GATT_FLAG_NONE)) != HRESULT_FROM_WIN32(ERROR_MORE_DATA)) {
		if (FuncReturn == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED)) {
			ErrorCode |= 0x0001;  // Access Denied error code
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(ERROR_INVALID_USER_BUFFER)) {
			; // No error, functioning as intended
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_INVALID_HANDLE)) {
			ErrorCode |= 0x0002;  // Bluetooth initialization error, handle doesn't work
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_READ_NOT_PERMITTED)) {
			ErrorCode |= 0x0004;  // No read access
		}
		else if (FuncReturn == HRESULT_FROM_WIN32(E_BLUETOOTH_ATT_ATTRIBUTE_NOT_LONG)) {
			ErrorCode |= 0x0008;  // Likely a systematic error, will need to find a new API
		}
		else {
			ErrorCode |= 0x0010;  // Complex error
			ErrorCode = FuncReturn;
			return 0;
		}
	}

	// Read battery voltage from device
	if (bufSize != 0) {
		BTH_LE_GATT_CHARACTERISTIC_VALUE* batteryVoltage = (BTH_LE_GATT_CHARACTERISTIC_VALUE*)malloc(bufSize);
		if (batteryVoltage == NULL) {
			ErrorCode |= 0x0020;  // Memory allocation error
			return ErrorCode;
		}
		// Go get voltage
		if ((FuncReturn = BluetoothGATTGetCharacteristicValue(BattServHandle, &(Battery), (ULONG)bufSize, batteryVoltage, NULL, BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE)) != S_OK) {
			if (FuncReturn == ERROR_ACCESS_DENIED) {
				ErrorCode |= 0x0040; // Access Denied
			}
			else if (FuncReturn == ERROR_INVALID_USER_BUFFER) {
				ErrorCode |= 0x0080;  // Buffer was wrong size, issue with previous function
			}
			else {
				ErrorCode |= 0x0100; // Complex error
			}
		}
		// Read voltage from structure
		if (batteryVoltage->DataSize != sizeof(uint8_t)) {
			ErrorCode |= 0x0200;  // Data transmission error, didn't send correct data
		}

		// Output voltage
		return batteryVoltage->Data[0];
	}

	return retVal;
}

/* 
 * @brief	Cleanly closes/terminates the device and all associated memory processes
 */
BLE_Device::~BLE_Device() {
	// Make sure the device isn't doing anything
	EnterState(PAUSE);
	// Disable the notificatino event
	BluetoothGATTUnregisterEvent(NotifyHandle, BLUETOOTH_GATT_FLAG_NONE);

	// Close all handles
	CloseHandle(DevHandle);
	CloseHandle(ServHandle);
	CloseHandle(BattServHandle);

	// Free dynamic memory
	free(NotifyHandleRegistration);
	if (FilePath != NULL) {
		delete FilePath;
	}
	if (CallbackContext != NULL) {
		if (CallbackContext->fileHandle != NULL) {
			CloseHandle(CallbackContext->fileHandle);
		}
	}
	free(CallbackContext);

	OutputDebugStringW(L"Device closed");
}

/* 
 * @brief	Event callback function for whenever device sends a notification
 * @param	BTH_LE_GATT_EVENT_TYPE EventType => The type of event, currently only CharacteristicValueChangedEvent
 *			is supported, only purpose is futureproofing
 * @param	PVOID (BLUETOOTH_GATT_VALUE_CHANGED_EVENT*) EventOutParamter => struct containing the new data after
 *			the write, as well as various data sizes
 * @param	PVOID Context => additional context in the form of a BLE_CALLBACK_CONTEXT struct pointer, this provides
 *			information about the previous sample information for continuety, as well as other important basic info
 */
void BLE_Device::ReadNotifcationData(BTH_LE_GATT_EVENT_TYPE EventType, PVOID EventOutParameter, PVOID Context) {
	// Cast void pointers
	BLE_CALLBACK_CONTEXT* Info = (BLE_CALLBACK_CONTEXT*)Context;
	BLUETOOTH_GATT_VALUE_CHANGED_EVENT* NotifPacket = (BLUETOOTH_GATT_VALUE_CHANGED_EVENT*)EventOutParameter;
	int16_t rawDataX = 0;
	int16_t rawDataY = 0;
	int16_t rawDataZ = 0;
	double accelX = 0;
	double accelY = 0;
	double accelZ = 0;
	USHORT tempSize = 0;
	BTH_LE_GATT_CHARACTERISTIC_VALUE* Packet = NULL;
	OVERLAPPED* IOCalls[POINTS_PER_PACKET];
	DWORD IOSizeOut;
	// Change of basis matrix used to convert accelerometer data to the same basis as gravity (used for plotting)
	/* Matrix format:
	 *	cos(angley)*cos(anglez)		cos(anglex)*sin(-anglez)	sin(angley)	
	 *  cos(angley)*sin(anglez)		cos(anglex)*cos(anglez)		cos(angley)*sin(-anglex)
	 *  sin(-angley)				sin(anglex)					cos(angley)*cos(anglex)
	 *  Av = w : v = <rawAccelX, rawAccelY, rawAccelZ> (typecasted)
	 */
	Eigen::Matrix<double, 3, 3> changeOfBasis;
	Eigen::Matrix<double, 3, 1> adjustedAcceleration;

	// Make sure correct event
	if (EventType != CharacteristicValueChangedEvent)
		return;
	if (NotifPacket->ChangedAttributeHandle != Info->AttributeHandle) {
		return;
	}
	
	// If notification is 1, then new data is available
	if (NotifPacket->CharacteristicValue->Data[0] != 0x00) {
		// Perform long read opperation to get the new data
		BluetoothGATTGetCharacteristicValue(*(Info->ServPtr), Info->DataPtr, 0, NULL, &tempSize, BLUETOOTH_GATT_FLAG_NONE);
		Packet = (PBTH_LE_GATT_CHARACTERISTIC_VALUE)malloc(tempSize);
		BluetoothGATTGetCharacteristicValue(*(Info->ServPtr), Info->DataPtr, tempSize, Packet, NULL, BLUETOOTH_GATT_FLAG_FORCE_READ_FROM_DEVICE);

		// If the current data is callibrated, then collect and use data
		if (Info->Callibrated) {
			OutputDebugStringW(L"\nCallibrated");
			// Begin data parsing
			for (int offset = 0; offset < (POINTS_PER_PACKET * 12); offset += 12) {
				// Get all the raw accelerometer data and calculate
				ParseAccelData(&rawDataX, &rawDataY, &rawDataZ, Packet->Data, offset);

				// Build the new data acceleration vector
				Eigen::Matrix<double, 3, 1> accelVector((((double)rawDataX)* Info->scale * ACCEL_SCALE)/MAX_INT, (((double)rawDataY)* Info->scale * ACCEL_SCALE) / MAX_INT, (((double)rawDataZ)* Info->scale * ACCEL_SCALE) / MAX_INT);

				// Get and use all the raw gyroscope data
				ParseAccelData(&rawDataX, &rawDataY, &rawDataZ, Packet->Data, 6 + offset);
				double dAnglex, dAngley, dAnglez;
				dAnglex = SAMPLE_RATE * ((((double)(rawDataX + Info->anglexComp)) * GYRO_SCALE * M_PI) / (MAX_INT*180));
				dAngley = SAMPLE_RATE * ((((double)(rawDataY + Info->angleyComp)) * GYRO_SCALE * M_PI) / (MAX_INT*180));
				dAnglez = SAMPLE_RATE * ((((double)(rawDataZ + Info->anglezComp)) * GYRO_SCALE * M_PI) / (MAX_INT*180));
				Info->anglex += dAnglex;
				Info->angley += dAngley;
				Info->anglez += dAnglez;

				// If the data appears to be just gravity, then readjust gyroscope values
				long double magnitude = sqrt((long double)((((double)rawDataX) * ACCEL_SCALE) / MAX_INT * (((double)rawDataX) * ACCEL_SCALE) / MAX_INT + (((double)rawDataY) * ACCEL_SCALE) / MAX_INT * (((double)rawDataY) * ACCEL_SCALE) / MAX_INT + (((double)rawDataZ) * ACCEL_SCALE) / MAX_INT * (((double)rawDataZ) * ACCEL_SCALE) / MAX_INT));
				if (abs(1 - magnitude) < TOLERANCE) {
					Info->CallibrationCounter++;
					if (Info->CallibrationCounter > 20) {
						Info->anglex = acos(accelVector(2));
						if (rawDataY > 0) { // Fix the orientation of the cosine operation if necessary
							Info->anglex *= -1;
						}
						Info->angley = -1 * atan2(accelVector(0), accelVector(2));
						Info->CallibrationCounter = 0;
					}
				}
				else {
					Info->CallibrationCounter = 0;
				}

				// With updated angles, change of basis matrix can be constructed
				changeOfBasis << cos(Info->angley) * cos(Info->anglez), cos(Info->anglex)* sin(-1 * (Info->anglez)), sin(Info->angley),
					cos(Info->angley)* sin(Info->anglez), cos(Info->anglex)* cos(Info->anglez), cos(Info->angley)* sin(-1 * (Info->anglex)),
					sin(-1 * (Info->angley)), sin(Info->anglex), cos(Info->angley)* cos(Info->anglex);

				// Calculate the matrix product, yielding the acceleration with respect to the initial orientation (gravity is negative z, x and y are as calibrated)
				adjustedAcceleration = changeOfBasis * accelVector;

				// Now use this to find position using Euler's method approximation where SAMPLE_RATE is the change
				// in time between measurements, and GS_TO_MPS2 is 9.81, or the conversion from Gs to m/s^2
				Info->Vx += adjustedAcceleration(0) * SAMPLE_RATE * GS_TO_MPS2;
				Info->Vy += adjustedAcceleration(1) * SAMPLE_RATE * GS_TO_MPS2;
				Info->Vz += (adjustedAcceleration(2) + 1) * SAMPLE_RATE * GS_TO_MPS2; // Remove gravity from accelerometer measurement
				
#ifdef VELOCITY_CAP
				// Capping velocities to reasonable numbers to not let them go too far due to bad measurements
				if (Info->Vx > VELOCITY_CAP) { Info->Vx = VELOCITY_CAP; }
				if (Info->Vx < -VELOCITY_CAP) { Info->Vx = -VELOCITY_CAP; }
				if (Info->Vy > VELOCITY_CAP) { Info->Vy = VELOCITY_CAP; }
				if (Info->Vy < -VELOCITY_CAP) { Info->Vy = -VELOCITY_CAP; }
				if (Info->Vz > VELOCITY_CAP) { Info->Vz = VELOCITY_CAP; }
				if (Info->Vz < -VELOCITY_CAP) { Info->Vz = -VELOCITY_CAP; }
#endif

				Info->x += Info->Vx * SAMPLE_RATE;
				Info->y += Info->Vy * SAMPLE_RATE;
				Info->z += Info->Vz * SAMPLE_RATE;

#ifdef POSITION_CAP
				// And then capping positions
				if (Info->x > POSITION_CAP) { Info->x = POSITION_CAP; }
				if (Info->x < -POSITION_CAP) { Info->x = -POSITION_CAP; }
				if (Info->y > POSITION_CAP) { Info->y = POSITION_CAP; }
				if (Info->y < -POSITION_CAP) { Info->y = -POSITION_CAP; }
				if (Info->z > POSITION_CAP) { Info->z = POSITION_CAP; }
				if (Info->z < -POSITION_CAP) { Info->z = -POSITION_CAP; }
#endif

				// Begin storing data into a csv file
				std::stringstream stream;
				std::string tempString;

				stream << std::fixed << std::setprecision(3) << Info->x << "," << std::fixed << std::setprecision(3) << Info->y << "," << std::fixed << std::setprecision(3) << Info->z << "\n";
				stream >> tempString;
				OutputDebugStringA("\n");
				OutputDebugStringA(tempString.c_str());
				
				// Format the string into a c-style string (string.c_str() ran into issues)
				char* fileOutputString = (char*)malloc((tempString.length() + 1) * sizeof(char));
				if (fileOutputString != NULL) {
					strcpy_s(fileOutputString, tempString.length()+1, tempString.c_str());
					fileOutputString[tempString.length()] = L'\n';

					// Create an OVERLAPPED structure to allow asynchronous file IO
					IOCalls[offset / 12] = (OVERLAPPED*)calloc(1, sizeof(OVERLAPPED));
					if (IOCalls[offset / 12] != NULL) {
						IOCalls[offset / 12]->Offset = 0xFFFFFFFF;
						IOCalls[offset / 12]->OffsetHigh = 0xFFFFFFFF;
						WriteFileEx(Info->fileHandle, fileOutputString, (tempString.size() + 1) * sizeof(char), IOCalls[offset / 12], (LPOVERLAPPED_COMPLETION_ROUTINE)BLE_Device::FileIOCallback);
					}
					else {
						OutputDebugStringW(L"\nPacket Lost ");
					}
					free(fileOutputString);
				}
				tempString.clear();
			}
			for (int i = 0; i < POINTS_PER_PACKET; i++) {
				if (IOCalls[i] != NULL) {
					GetOverlappedResultEx(Info->fileHandle, IOCalls[i], &IOSizeOut, INFINITE, TRUE);
					free(IOCalls[i]);
					
				}
			}
		}
		// Perform callibration procedure
		else {
			// If starting callibration
			if (Info->CallibrationCounter == 0) {
				OutputDebugStringW(L"\nCallibrate");
				// Check if magnitude close to gravity
				ParseAccelData(&rawDataX, &rawDataY, &rawDataZ, Packet->Data, 0);
				// If the magnitude of this vector is approximately 1 (ie. 1 g), then the data should be callibrated to
				long double magnitude = sqrt((long double)((((double)rawDataX) * ACCEL_SCALE) / MAX_INT * (((double)rawDataX) * ACCEL_SCALE) / MAX_INT + (((double)rawDataY) * ACCEL_SCALE) / MAX_INT * (((double)rawDataY) * ACCEL_SCALE) / MAX_INT + (((double)rawDataZ) * ACCEL_SCALE) / MAX_INT * (((double)rawDataZ) * ACCEL_SCALE) / MAX_INT));
				if (abs(1 - magnitude) < TOLERANCE) {
					// Scaling data mesurements to exactly 1 g compensating for internal MEMs errors
					Info->scale = 1 / magnitude;
					// Load x, y, and z with the raw data for callibration comparison
					Info->x = ((double)rawDataX * Info->scale * ACCEL_SCALE) / MAX_INT;
					Info->y = ((double)rawDataY * Info->scale * ACCEL_SCALE) / MAX_INT;
					Info->z = ((double)rawDataZ * Info->scale * ACCEL_SCALE) / MAX_INT;
					// Solve for the initial angles, with the reference of the current vector being gravity <0, 0, -1>
					Info->anglex = acos(Info->z);  // Anglex => perpendicular to plane consisting of z and xy vectors, D:[0,180)
					if (rawDataY > 0) { // Fix the orientation of the cosine operation if necessary
						Info->anglex *= -1;
					}
					Info->angley = -1 * atan2(Info->x, Info->z); // Rotate such that z axis is coplanar with the ideal yz plane
					Info->anglez = 0;  // Since the direction of the X and Y axis are uniportant, this can remain zero

					// For gyroscope callibration purposes
					Info->Vx = 0;
					Info->Vy = 0;
					Info->Vz = 0;

					// Begin callibration
					Info->CallibrationCounter++;

					std::wstringstream stream;
					std::wstring tempString;
					stream << std::fixed << std::setprecision(3) << Info->anglex;
					stream << L",";
					stream << std::fixed << std::setprecision(3) << Info->angley;
					stream << L",";
					stream << std::fixed << std::setprecision(3) << Info->anglez;
					stream << L",";
					stream << Info->x << L"," << Info->y << L"," << Info->z << L"\n";
					stream >> tempString;
					OutputDebugStringW(L"\n");
					OutputDebugStringW(tempString.c_str());
				}
			}
			// If callibration complete
			else if (Info->CallibrationCounter >= CALLIBRATION_PACKETS) {
				Info->Callibrated = true;
				Info->CallibrationCounter = 0;
				Info->x = 0;
				Info->y = 0;
				Info->z = 0;
				Info->anglexComp = (int16_t)Info->Vx;
				Info->angleyComp = (int16_t)Info->Vy;
				Info->anglezComp = (int16_t)Info->Vz;
				Info->anglexComp *= -1;
				Info->angleyComp *= -1;
				Info->anglezComp *= -1;
				Info->Vx = 0;
				Info->Vy = 0;
				Info->Vz = 0;

				// Tell Device that callibration is complete
				NotifPacket->CharacteristicValue->Data[0] = CALLIBRATION_COMPLETE;
				BluetoothGATTSetCharacteristicValue(*(Info->ServPtr), Info->NotifPtr, NotifPacket->CharacteristicValue, NULL, BLUETOOTH_GATT_FLAG_NONE);
			}
			// continued callibration, check if device has moved/spun
			else {
				OutputDebugStringW(L"\nCallibrating");
				for (int offset = 0; offset < (POINTS_PER_PACKET * 12); offset += 12) {
					// Check if the acceleration data is still within tolerance
					ParseAccelData(&rawDataX, &rawDataY, &rawDataZ, Packet->Data, offset);
					
					// All data points must be within tolerance of 1 g
					if ((((abs((double)(((double)rawDataX * Info->scale * ACCEL_SCALE)/MAX_INT) - (Info->x)))) > TOLERANCE*GS_TO_MPS2)
						|| (((abs((double)(((double)rawDataY * Info->scale * ACCEL_SCALE)/MAX_INT) - (Info->y)))) > TOLERANCE)
						|| (((abs((double)(((double)rawDataZ * Info->scale * ACCEL_SCALE)/MAX_INT) - (Info->z)))) > TOLERANCE)) {
						// if failed restart callibration
						Info->CallibrationCounter = -1;
					}
				}
				// Collect and average angular accelerations to callibrate error
				ParseAccelData(&Info->anglexComp, &Info->angleyComp, &Info->anglezComp, Packet->Data, 6 + ((int)(POINTS_PER_PACKET/2))*12);
				Info->Vx += ((double)Info->anglexComp) / CALLIBRATION_PACKETS;
				Info->Vy += ((double)Info->angleyComp) / CALLIBRATION_PACKETS;
				Info->Vz += ((double)Info->anglezComp) / CALLIBRATION_PACKETS;
				// Increment counter
				Info->CallibrationCounter += 1;
			}

		}
	}
}

/*
 * @brief	Callback function for WIN32 file IO since standard c++ streams didn't seem to work
 * @param	dwErrorCode ==> Mem write error code
 * @param	dwNumberOfBytesTransfered ==> self explanatory
 * @param	lpOverlapped ==> OVERLAPPED* that must be freed, done externally
 */
void BLE_Device::FileIOCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
	// Currently empty, don't need it do do anything
	return;
}

/*
 * @brief	Helper function for the ReadNotificationData callback function for parsing data
 * @param	int16_t* x => location to put the x data
 * @param	int16_t* y => location to put the y data
 * @param	int16_t* z => location to put the z data
 * @param	UCHAR* data => pointer to array containing the raw data sent over bluetooth
 * @param	int offset => byte offset of data to read, every 12 is accelerometer, every 6 is gyroscope
 */
void BLE_Device::ParseAccelData(int16_t* x, int16_t* y, int16_t* z, UCHAR* data, int offset) {
	*x = data[0 + offset];
	*x = *x << 8;
	*x |= data[1 + offset];
	*y = data[2 + offset];
	*y = *y << 8;
	*y |= data[3 + offset];
	*z = data[4 + offset];
	*z = *z << 8;
	*z |= data[5 + offset];
	if ((offset % 12) == 0) {
		*x *= -1;
		*y *= -1;
		*z *= -1;
	}
}

/*
 * @brief	Helper funtion that outputs the path to the folder containing the current exeutable (must be freed)
 * @param	(out) WCHAR** out => outputs a unicode string to the current executable, returns NULL on failure
 * @retval	Size of out, returns 0 on failure
 */
int BLE_Device::GetExecPath(WCHAR** out) {
	WCHAR* temp;
	int bufSize = 300;
	WCHAR* buf = (WCHAR*)malloc(bufSize * sizeof(WCHAR));
	if (buf == NULL) {
		OutputDebugStringW(L"Memory Allocation Error\n");
		return 0;
	}
	else if (GetModuleFileNameW(NULL, buf, bufSize-1) == 0) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			OutputDebugStringW(L"Path Retrieval Error\n");
			free(buf);
			*out = NULL;
			return 0;
		}
		while (err == ERROR_INSUFFICIENT_BUFFER) {
			bufSize += 100;
			temp = (WCHAR*)realloc(buf, bufSize*sizeof(WCHAR));
			if (temp == NULL) {
				OutputDebugStringW(L"Memory Allocation Error\n");
				free(buf);
				return 0;
			}
			buf = temp;
			if (GetModuleFileNameW(NULL, buf, bufSize-1) != 0) {
				break;
			}
			else if (bufSize >= 5000) {
				OutputDebugStringW(L"Unreasonable Buffer Size\n");
				free(buf);
				*out = NULL;
				return 0;
			}
			err = GetLastError();
		}
	}
	// Remove executable name + .csv
	buf[wcslen(buf) - wcslen(EXECUTABLE_NAME)] = L'\0';
	*out = buf;
	return bufSize;
}