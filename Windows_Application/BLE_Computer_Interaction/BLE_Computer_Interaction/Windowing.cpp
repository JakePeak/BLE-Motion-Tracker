#include "framework.h"
#include "Windowing.h"
#include "Resource.h"
#include "BLE_Communication.h"
#include "Python_Embed.h"

extern HMENU menu;
extern BLE_Device* Device;
extern EmbeddedPython* Python;
extern WCHAR szWindowClass[MAX_LOADSTRING];
extern HINSTANCE hInst;
extern int errorCode;
extern BUTTON_HANDLES buttonHandles;
HWND batteryControl;

/*
 *  @brief  Creates the menu for the web app
 *  @param  (OUT) pointer to the HMENU object to initialize
 */
void InitMenu(HWND window) {
    // Create menus/submenus
    menu = CreateMenu();
    HMENU file = CreateMenu();
    HMENU help = CreateMenu();
    HMENU debug = CreateMenu();
    HMENU python = CreateMenu();
    HMENU button_functions = CreateMenu();

    // Work inside out, building and appending the menus
    AppendMenu(button_functions, MF_STRING | MF_ENABLED, MENU_CONNECT, L"Connect");
    AppendMenu(button_functions, MF_STRING | MF_GRAYED, MENU_PAUSE, L"Pause");
    AppendMenu(button_functions, MF_STRING | MF_GRAYED, MENU_UNPAUSE, L"Unpause");
    AppendMenu(button_functions, MF_STRING | MF_GRAYED, MENU_BATTERY, L"Battery Level");
    AppendMenu(button_functions, MF_STRING | MF_GRAYED, MENU_DISCONNECT, L"Disconnect");

    AppendMenu(debug, MF_POPUP, (UINT_PTR)button_functions, L"Button Functions");

    AppendMenu(help, MF_STRING, IDM_ABOUT, L"About");

    AppendMenu(file, MF_STRING, IDM_EXIT, L"Exit");

    AppendMenu(python, MF_STRING | MF_ENABLED, MENU_RST_PY, L"Reset Python Path");
    AppendMenu(python, MF_STRING | MF_CHECKED, MENU_GRAPH_CF, L"Enable Autographing");

    AppendMenu(menu, MF_POPUP, (UINT_PTR)file, L"File");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)debug, L"Debug");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)python, L"Python");
    AppendMenu(menu, MF_POPUP, (UINT_PTR)help, L"Help");

    SetMenu(window, menu);
}

/* 
 * @brief   Controls whether or not Pause, Unpause, and Battery level options are enabled in the menu and buttons
 * @param   HWND window => window hanle of main window
 * @param   UINT uEnable => enable or disable the options, using either MF_ENABLED and MF_GRAYED
 */
void RestoreMenuOptions(HWND window, UINT uEnable) {
    HMENU menu = GetMenu(window);
    HMENU debug = GetSubMenu(menu, DEBUG_POS);
    HMENU button_functions = GetSubMenu(debug, BUT_FUN_POS);
    EnableMenuItem(button_functions, MENU_PAUSE, uEnable | MF_BYCOMMAND);
    EnableMenuItem(button_functions, MENU_UNPAUSE, uEnable | MF_BYCOMMAND);
    EnableMenuItem(button_functions, MENU_BATTERY, uEnable | MF_BYCOMMAND);
    EnableMenuItem(button_functions, MENU_DISCONNECT, uEnable | MF_BYCOMMAND);
    DrawMenuBar(window);
    bool buttonStatus = (uEnable == MF_ENABLED);
    Button_Enable(buttonHandles.pauseBut, buttonStatus);
    Button_Enable(buttonHandles.unpauseBut, buttonStatus);
    Button_Enable(buttonHandles.batteryBut, buttonStatus);
    Button_Enable(buttonHandles.fileBut, buttonStatus);
    Button_Enable(buttonHandles.disconnectBut, buttonStatus);
}

/*
 * @brief   Controls whether or not the connection/extern option is enabled in menu and buttons
 * @param   HWND window => window handle of main window
 * @param   UINT uEnable => enable or disable the option, using either MF_ENABLED and MF_GRAYED
 */
void ControlConnections(HWND window, UINT uEnable) {
    HMENU menu = GetMenu(window);
    HMENU debug = GetSubMenu(menu, DEBUG_POS);
    HMENU button_functions = GetSubMenu(debug, BUT_FUN_POS);
    EnableMenuItem(button_functions, MENU_CONNECT, uEnable | MF_BYCOMMAND);
    DrawMenuBar(window);
    Button_Enable(buttonHandles.connectBut, (uEnable==MF_ENABLED));
    Button_Enable(buttonHandles.externBut, (uEnable == MF_ENABLED));
}

/*
 * @brief   Controls whether or not the pause is enabled in menu and buttons
 * @param   HWND window => window handle of main window
 * @param   UINT uEnable => enable or disable the option, using either MF_ENABLED and MF_GRAYED
 */
void ControlPause(HWND window, UINT uEnable) {
    HMENU menu = GetMenu(window);
    HMENU debug = GetSubMenu(menu, DEBUG_POS);
    HMENU button_functions = GetSubMenu(debug, BUT_FUN_POS);
    EnableMenuItem(button_functions, MENU_PAUSE, uEnable | MF_BYCOMMAND);
    DrawMenuBar(window);
    Button_Enable(buttonHandles.pauseBut, (uEnable == MF_ENABLED));
}

/*
 * @brief   Controls whether or not the unpause and related buttons are enabled in menu and buttons
 * @param   HWND window => window handle of main window
 * @param   UINT uEnable => enable or disable the option, using either MF_ENABLED and MF_GRAYED
 */
void ControlUnpause(HWND window, UINT uEnable) {
    HMENU menu = GetMenu(window);
    HMENU debug = GetSubMenu(menu, DEBUG_POS);
    HMENU button_functions = GetSubMenu(debug, BUT_FUN_POS);
    EnableMenuItem(button_functions, MENU_UNPAUSE, uEnable | MF_BYCOMMAND);
    EnableMenuItem(button_functions, MENU_BATTERY, uEnable | MF_BYCOMMAND);
    EnableMenuItem(button_functions, MENU_DISCONNECT, uEnable | MF_BYCOMMAND);
    DrawMenuBar(window);
    Button_Enable(buttonHandles.unpauseBut, (uEnable == MF_ENABLED));
    Button_Enable(buttonHandles.batteryBut, (uEnable == MF_ENABLED));
    Button_Enable(buttonHandles.fileBut, (uEnable == MF_ENABLED));
    Button_Enable(buttonHandles.disconnectBut, (uEnable == MF_ENABLED));
}

/*
 * @brief   Enables/Disables menu options related to conifiguring python
 * @param   HWND window => window handle of main window
 * @param   UINT uEnable => enable or disable the option, using either MF_ENABLED and MF_GRAYED
 */
void ControlPythonConfig(HWND window, UINT uEnable) {
    HMENU menu = GetMenu(window);
    HMENU pythonMenu = GetSubMenu(menu, PYTHON_POS);
    EnableMenuItem(pythonMenu, MENU_RST_PY, uEnable);
    DrawMenuBar(window);
}

/*
 * @brief   Toggles whether Pause/Unpause outputs a graph or just a file
 * @param   HWND window => window handle of main window
 * @param   bool status => new state
 */
void ControlGraphOutput(HWND window, bool status) {
    HMENU menu = GetMenu(window);
    HMENU pythonMenu = GetSubMenu(menu, PYTHON_POS);
    if (status) {
        CheckMenuItem(pythonMenu, MENU_GRAPH_CF, MF_CHECKED);
    }
    else {
        CheckMenuItem(pythonMenu, MENU_GRAPH_CF, MF_UNCHECKED);
    }
}

/*
 * @brief   Displays the battery level on the main window
 * @param   HWND window => window handle of main window
 * @param   double batteryLevel => battery level of battery, in percentage
 */
void DisplayBattery(HWND window, uint8_t batteryLevel) {
    std::wstring text;
    std::wstringstream stream;
    stream << batteryLevel << L'%%';
    stream >> text;
    Static_SetText(batteryControl, text.c_str());
    ShowWindow(batteryControl, SW_SHOW);
}

/*
 * @brief   Initializes controls of the primary application window
 * @param   HWND window => Handle of main window
 */
void InitControls(HWND window) {
    // Create Buttons on window
    buttonHandles.connectBut = CreateWindowW(L"Button", L"Connect", INTERACT_BUTTON, 20, WINDOW_HEIGHT / 56, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)MENU_CONNECT, hInst, NULL);
    buttonHandles.pauseBut = CreateWindowW(L"Button", L"Pause", INTERACT_BUTTON, 1 * ((WINDOW_WIDTH - 120) / 4 + 20) + 20, WINDOW_HEIGHT / 56, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)MENU_PAUSE, hInst, NULL);
    buttonHandles.unpauseBut = CreateWindowW(L"Button", L"Unpause", INTERACT_BUTTON, 2 * ((WINDOW_WIDTH - 120) / 4 + 20) + 20, WINDOW_HEIGHT / 56, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)MENU_UNPAUSE, hInst, NULL);
    buttonHandles.batteryBut = CreateWindowW(L"Button", L"Battery", INTERACT_BUTTON, 3 * ((WINDOW_WIDTH - 120) / 4 + 20) + 20, WINDOW_HEIGHT / 56, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)MENU_BATTERY, hInst, NULL);
    buttonHandles.externBut = CreateWindowW(L"Button", L"Import File", INTERACT_BUTTON, 20, 2 * WINDOW_HEIGHT / 56 + WINDOW_HEIGHT / 7, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)BUTTON_EXTERN, hInst, NULL);
    buttonHandles.fileBut = CreateWindowW(L"Button", L"Set Output File", INTERACT_BUTTON, 1 * ((WINDOW_WIDTH - 120) / 4 + 20) + 20, 2 * WINDOW_HEIGHT / 56 + WINDOW_HEIGHT / 7, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)BUTTON_OUTPUT, hInst, NULL);
    buttonHandles.disconnectBut = CreateWindowW(L"Button", L"Disconnect", INTERACT_BUTTON, 3 * ((WINDOW_WIDTH - 120) / 4 + 20) + 20, 2 * WINDOW_HEIGHT / 56 + WINDOW_HEIGHT / 7, (WINDOW_WIDTH - 120) / 4, WINDOW_HEIGHT / 7, window, (HMENU)MENU_DISCONNECT, hInst, NULL);
    batteryControl = CreateWindowW(L"Static", L"Battery Level", WS_CHILD | SS_CENTER, (WINDOW_WIDTH * 2) / 5, (WINDOW_HEIGHT * 5) / 8, WINDOW_WIDTH / 5, WINDOW_HEIGHT / 8, window, NULL, hInst, NULL);
}

/* 
 * @brief   Initializes controls of the error poppup window
 * @param   HWND window => Handle of the error window
 */
void InitErrorControls(HWND window) {
    // Parse the error code and build the error message
    std::stringstream stream;
    stream << ("Oh No! An Error Occured!\r\n\r\nError Code:  ");
    stream << std::hex << errorCode;
    std::string stringErrorCodeUnformatted(stream.str());
    wchar_t* stringErrorCode = new wchar_t[stringErrorCodeUnformatted.size() + 1];
    mbstowcs_s(NULL, stringErrorCode, stringErrorCodeUnformatted.size() + 1, stringErrorCodeUnformatted.c_str(), stringErrorCodeUnformatted.size() + 1);
    CreateWindowW(L"Static", stringErrorCode, WS_VISIBLE | WS_CHILD | SS_RIGHT, 20, 20, ERROR_BOX_WIDTH - 60, ERROR_BOX_HEIGHT/2, window, NULL, hInst, NULL);
    delete[]stringErrorCode;

    // Create the "OK" button
    CreateWindowW(L"Button", L"Ok", WS_VISIBLE | WS_CHILD | BS_CENTER | BS_VCENTER | BS_PUSHBUTTON, ERROR_BOX_WIDTH - 100, ERROR_BOX_HEIGHT / 5 * 3, 70, ERROR_BOX_HEIGHT/8, window, (HMENU)IDM_EXIT, hInst, NULL);
    
}

/* 
 * @brief Function registers the window class of the error window
 * @param hInstance instance handle of application
 * @retval ATOM
 */
ATOM RegisterErrorClass(HINSTANCE hInstance) {

    WNDCLASSEXW err;

    err.cbSize = sizeof(WNDCLASSEXW);
    err.style = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;
    err.lpfnWndProc = ErrorProc;
    err.cbClsExtra = 0;
    err.cbWndExtra = 0;
    err.hInstance = hInstance;
    err.hIcon = NULL;
    err.hCursor = LoadCursor(nullptr, IDC_ARROW);
    err.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    err.lpszMenuName = NULL;
    err.lpszClassName = L"errorBox";
    err.hIconSm = NULL;

    return RegisterClassExW(&err);

}

/*
 * @brief   Callback procedure for the error window
 * @param   hErr => window handle
 * @param   message => command type occured
 * @param   wParam => parameter dependent on message
 * @param   lParam => parameter dependent on message
 * @result  Result of processing
 */
LRESULT CALLBACK ErrorProc(HWND hErr, UINT message, WPARAM wParam, LPARAM lParam) {

    switch (message)
    {
    case WM_COMMAND:
    {
        // handle commands
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hErr);
            break;
        default:
            return DefWindowProc(hErr, message, wParam, lParam);
            break;
        }
    }
    break;
    case WM_CREATE:
        // Build out controls here
        InitErrorControls(hErr);
        break;
    case WM_DISPLAYCHANGE:
        SetWindowPos(hErr, HWND_NOTOPMOST, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, SWP_SHOWWINDOW);
        ShowWindow(hErr, SW_NORMAL);
        break;
    case WM_DESTROY:
        errorCode = 0;
        // Enable connetions, releasing the "mutex"
        ControlConnections(GetParent(hErr), MF_ENABLED);
        break;
    default:
        return DefWindowProc(hErr, message, wParam, lParam);
        break;
    }
    return 0;
}

/*
 * @brief   Initializes an openFileName structure for getting user to select a csv to input
 * @param   Structure to initialize
 */
void OpenCSVInit(OPENFILENAMEW* openFileName, HWND window) {
    openFileName->lStructSize = sizeof(OPENFILENAMEW);
    openFileName->hwndOwner = window;
    openFileName->hInstance = hInst;
    openFileName->lpstrFilter = L"CSV Files\0*.CSV*\0\0";
    openFileName->lpstrCustomFilter = NULL;
    openFileName->nFilterIndex = 1;
    openFileName->lpstrFile = (WCHAR*)malloc(INPUT_FILE_BUF_SIZE * sizeof(WCHAR));
    if (openFileName->lpstrFile != NULL) {
        wcscpy_s(openFileName->lpstrFile, INPUT_FILE_BUF_SIZE, DEFAULT_NAME_WIDE);
    }
    openFileName->nMaxFile = INPUT_FILE_BUF_SIZE;
    openFileName->lpstrFileTitle = NULL;
    openFileName->lpstrInitialDir = NULL;
    openFileName->lpstrTitle = L"Select the CSV File";
    openFileName->Flags = OFN_FILEMUSTEXIST; // Do OFN_ENABLETEMPLATE if a template resource is added
    openFileName->lpstrDefExt = L"csv";
    openFileName->FlagsEx = 0;
}

/*
 * @brief   Initializes openFileName structure for getting the user's python installation
 * @param   Structure to initialize
 */
void OpenPythonExeInit(OPENFILENAMEW* openFileName, HWND window) {
    openFileName->lStructSize = sizeof(OPENFILENAMEW);
    openFileName->hwndOwner = window;
    openFileName->hInstance = hInst;
    openFileName->lpstrFilter = L"EXE Files\0*.EXE*\0\0";
    openFileName->lpstrCustomFilter = NULL;
    openFileName->nFilterIndex = 1;
    openFileName->lpstrFile = (WCHAR*)malloc(INPUT_FILE_BUF_SIZE * sizeof(WCHAR));
    if (openFileName->lpstrFile != NULL) {
        wcscpy_s(openFileName->lpstrFile, INPUT_FILE_BUF_SIZE, L"python.exe");
    }
    openFileName->nMaxFile = INPUT_FILE_BUF_SIZE;
    openFileName->lpstrFileTitle = NULL;
    openFileName->lpstrInitialDir = NULL;
    openFileName->lpstrTitle = L"Locate the Desired Python Executable (Must have NUMPY and PANDAS installed)";
    openFileName->Flags = OFN_FILEMUSTEXIST | OFN_READONLY; // Do OFN_ENABLETEMPLATE if a template resource is added
    openFileName->lpstrDefExt = L"exe";
    openFileName->FlagsEx = 0;
}

/*
 * @brief   Initializes openFileName structure for creating the output file
 * @param   Structure to initialize
 */
void OpenFileOutInit(OPENFILENAMEW* openFileName, HWND window) {
    openFileName->lStructSize = sizeof(OPENFILENAMEW);
    openFileName->hwndOwner = window;
    openFileName->hInstance = hInst;
    openFileName->lpstrFilter = L"CSV Files\0*.CSV*\0\0";
    openFileName->lpstrCustomFilter = NULL;
    openFileName->nFilterIndex = 1;
    openFileName->lpstrFile = (WCHAR*)malloc(INPUT_FILE_BUF_SIZE * sizeof(WCHAR));
    if (openFileName->lpstrFile != NULL) {
        wcscpy_s(openFileName->lpstrFile, INPUT_FILE_BUF_SIZE, DEFAULT_NAME_WIDE);
    }
    openFileName->nMaxFile = INPUT_FILE_BUF_SIZE;
    openFileName->lpstrFileTitle = NULL;
    openFileName->lpstrInitialDir = NULL;
    openFileName->lpstrTitle = L"Select/Create CSV to Write";
    openFileName->Flags = OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT; // Do OFN_ENABLETEMPLATE if a template resource is added
    openFileName->lpstrDefExt = L"csv";
    openFileName->FlagsEx = 0;
}

/*
 * @brief   Closes an openFileName structure for getting user to select a file
 * @param   Structure to close
 */
void OpenFileNameClose(OPENFILENAMEW* openFileName) {
    free((void*)openFileName->lpstrFile);
}

/*
 * @brief Deallocates memory and procedures when the window is shut down
 * @param hInstance instance handle of application
 */
void ApplicationShutdown(HINSTANCE hInstance) {
    // Free classes from memory
    UnregisterClass(szWindowClass, hInstance);
    UnregisterClass(L"errorBox", hInstance);

    // Call for bluetooth interaction to close out the connection
    delete Device;
    delete Python;
}