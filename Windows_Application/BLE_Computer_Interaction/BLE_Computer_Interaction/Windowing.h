#pragma once

#define MAX_LOADSTRING 100
#define INPUT_FILE_BUF_SIZE 500

#define MATPLOTLIB_WIDTH	2240
#define MATPLOTLIB_HEIGHT	1400

#define WINDOW_INITIAL_X	0
#define WINDOW_INITIAL_Y	0
#define WINDOW_WIDTH		GetSystemMetrics(SM_CXSCREEN)
#define WINDOW_HEIGHT		GetSystemMetrics(SM_CYSCREEN)

#define ERROR_BOX_X			350
#define ERROR_BOX_Y			350
#define ERROR_BOX_WIDTH		(WINDOW_WIDTH/5)
#define ERROR_BOX_HEIGHT	(WINDOW_HEIGHT/4)

#define MENU_CONNECT	201
#define MENU_PAUSE		202
#define MENU_UNPAUSE	203
#define MENU_BATTERY	204
#define MENU_DISCONNECT 205
#define BUTTON_EXTERN	206
#define BUTTON_OUTPUT	207
#define MENU_RST_PY		208
#define MENU_GRAPH_CF	209


#define DEBUG_POS		1
#define BUT_FUN_POS		0
#define PYTHON_POS		2

#define INTERACT_BUTTON (WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER | BS_PUSHBUTTON)
#define ERROR_WINDOW (WS_BORDER | WS_CAPTION | WS_OVERLAPPED | WS_SYSMENU | WS_POPUP)

void InitMenu(HWND window);
ATOM RegisterErrorClass(HINSTANCE hInstance);
void RestoreMenuOptions(HWND window, UINT uEnable);
void ControlConnections(HWND window, UINT uEnable);
void ControlPause(HWND window, UINT uEnable);
void ControlUnpause(HWND window, UINT uEnable);
void ControlPythonConfig(HWND window, UINT uEnable);
void ControlGraphOutput(HWND window, bool status);
void DisplayBattery(HWND window, uint8_t batteryLevel);
void InitControls(HWND window);
void InitErrorControls(HWND window);
INT_PTR CALLBACK ErrorProc(HWND hErr, UINT message, WPARAM wParam, LPARAM lParam);
void OpenCSVInit(OPENFILENAMEW* openFileName, HWND window);
void OpenPythonExeInit(OPENFILENAMEW* openFileName, HWND window);
void OpenFileOutInit(OPENFILENAMEW* openFileName, HWND window);
void OpenFileNameClose(OPENFILENAMEW* openFileName);
void ApplicationShutdown(HINSTANCE hInstance);

// Struct used to hold handles to the various buttons for enabling/disabling
typedef struct {
	HWND connectBut;
	HWND pauseBut;
	HWND unpauseBut;
	HWND batteryBut;
	HWND externBut;
	HWND fileBut;
	HWND disconnectBut;
}BUTTON_HANDLES;