// BLE_Computer_Interaction.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "resource.h"
#include "BLE_Communication.h"
#include "Windowing.h"
#include "Python_Embed.h"

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HMENU menu;                                     // window menu
BLE_Device* Device;                             // pointer to BLE device 
EmbeddedPython* Python;                         // pointer to embedded python system
int errorCode;                                  // Error code returnd from various BLE_communication functions
bool outputGraph;                               // Manages whether or not application is writing to file or graphing
BUTTON_HANDLES buttonHandles;                   // Struct containing various button handles

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    errorCode = 0;
    Device = NULL;
    outputGraph = true;

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BLECOMPUTERINTERACTION, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);
    RegisterErrorClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BLECOMPUTERINTERACTION));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    ApplicationShutdown(hInstance);
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BLECOMPUTERINTERACTION));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_BLECOMPUTERINTERACTION);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      WINDOW_INITIAL_X, WINDOW_INITIAL_Y, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);


   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   
   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case MENU_CONNECT:
                
                if (Device != NULL) {
                    delete Device;
                    Device = NULL;
                }
                Device = new BLE_Device(hWnd);
                errorCode = Device->ErrorCode;
                if (errorCode == 0) {
                    ControlConnections(hWnd, MF_GRAYED);
                    ControlPause(hWnd, MF_GRAYED);
                    ControlUnpause(hWnd, MF_ENABLED);
                }
                else {
                    RestoreMenuOptions(hWnd, MF_GRAYED);
                    // Temporarily disable connections, like a mutex so multiple error codes don't get quickly written
                    ControlConnections(hWnd, MF_GRAYED);
                    HWND hErr = CreateWindowW(L"errorBox", L"Bluetooth Init Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
                    ShowWindow(hErr, SW_NORMAL);
                }
                break;
            case MENU_PAUSE:
                errorCode = Device->EnterState(PAUSE);
                if (errorCode == 0) {
                    // Do nothing aside from button enabling, just let the graph sit on display if applicable
                    ControlPause(hWnd, MF_GRAYED);
                    ControlUnpause(hWnd, MF_ENABLED);
                }
                else {
                    // If error, call error window and shut down connection
                    RestoreMenuOptions(hWnd, MF_GRAYED);
                    ControlConnections(hWnd, MF_ENABLED);
                    HWND hErr = CreateWindowW(L"errorBox", L"Bluetoth_Pause_Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
                    ShowWindow(hErr, SW_NORMAL);
                    delete Device;
                    Device = NULL;
                }
                break;
            case MENU_UNPAUSE:
                Device->ClearCSV();
                errorCode = Device->EnterState(UNPAUSE);
                if (errorCode == 0) {
                    ControlUnpause(hWnd, MF_GRAYED);
                    ControlPause(hWnd, MF_ENABLED);
                    // If a graph should be output, output it
                    if (outputGraph) {
                        Python->SetCSVPath(Device);
                        Python->GraphData();
                    }
                    // Else just let the CSV grow
                }
                else {
                    // If error, call error window and shut down connection
                    RestoreMenuOptions(hWnd, MF_GRAYED);
                    ControlConnections(hWnd, MF_ENABLED);
                    HWND hErr = CreateWindowW(L"errorBox", L"Bluetoth Unpause Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
                    ShowWindow(hErr, SW_NORMAL);
                    delete Device;
                    Device = NULL;
                }
                break;
            case MENU_BATTERY:
            {
                uint8_t batteryLevel;
                // Get the battery percentage
                batteryLevel = Device->BatteryPercentage();
                if (Device->ErrorCode != 0) {
                    // If failed, call error window and shut down connection
                    errorCode = Device->ErrorCode;
                    RestoreMenuOptions(hWnd, MF_GRAYED);
                    ControlConnections(hWnd, MF_ENABLED);
                    HWND hErr = CreateWindowW(L"errorBox", L"Battery Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
                    ShowWindow(hErr, SW_NORMAL);
                    delete Device;
                    Device = NULL;
                }
                DisplayBattery(hWnd, batteryLevel);
            }
                break;
            case MENU_DISCONNECT:
                if (Device != NULL) {
                    delete Device;
                    Device = NULL;
                }
                RestoreMenuOptions(hWnd, MF_GRAYED);
                ControlConnections(hWnd, MF_ENABLED);
                break;
            case BUTTON_EXTERN:
            {
                OPENFILENAMEW openFileName;
                OpenCSVInit(&openFileName, hWnd);
                if (GetOpenFileNameW(&openFileName)) {
                    char* outBuf = (char*)malloc((((long long)openFileName.nFileExtension) + 4) * sizeof(char));
                    if (outBuf != NULL) {
                        wcstombs_s(NULL, outBuf, ((long long)openFileName.nFileExtension) + 4, openFileName.lpstrFile, openFileName.nMaxFile);
                        std::string outBufString;
                        if (outBuf != NULL) {
                            outBufString = outBuf;
                            Python->SetCSVPath(outBufString, PATH);
                            Python->GraphData();
                        }
                    }
                    else {
                        RestoreMenuOptions(hWnd, MF_GRAYED);
                        ControlConnections(hWnd, MF_GRAYED);
                        HWND hErr = CreateWindowW(L"errorBox", L"Memory Allocation Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
                        ShowWindow(hErr, SW_NORMAL);
                    }
                    free(outBuf);
                }
                OpenFileNameClose(&openFileName);
            }
            break;
            case MENU_RST_PY:
                OPENFILENAMEW openFileName;
                OpenPythonExeInit(&openFileName, hWnd);
                if (GetOpenFileNameW(&openFileName)) {
                    Python->SetPythonPath(openFileName.lpstrFile);
                }
                OpenFileNameClose(&openFileName);
                break;
            case MENU_GRAPH_CF:
                // Enable/Disable the ability for the graph to automatically load
                outputGraph = !outputGraph;
                ControlGraphOutput(hWnd, outputGraph);
                break;
            case BUTTON_OUTPUT:
            {
                // Open a save dialogue box and get input
                OPENFILENAMEW openFileName;
                OpenFileOutInit(&openFileName, hWnd);
                if (GetSaveFileNameW(&openFileName)) {
                    char* outBuf = (char*)malloc((((long long)openFileName.nFileExtension) + 4) * sizeof(char));
                    if (outBuf != NULL){
                        // Save file
                        wcstombs_s(NULL, outBuf, ((long long)openFileName.nFileExtension) + 4, openFileName.lpstrFile, openFileName.nMaxFile);
                        std::string outBufString;
                        if (outBuf != NULL) {
                            outBufString = outBuf;
                            Device->SetFilePath(outBufString, PATH);
                        }
                    }
                    else {
                        RestoreMenuOptions(hWnd, MF_GRAYED);
                        ControlConnections(hWnd, MF_GRAYED);
                        HWND hErr = CreateWindowW(L"errorBox", L"Output Selection Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
                        ShowWindow(hErr, SW_NORMAL);
                        delete Device;
                        Device = NULL;
                    }
                    free(outBuf);
                }
                OpenFileNameClose(&openFileName);
                
            }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_CREATE:
        // Initialize python
        OPENFILENAMEW openFileName;
        OpenPythonExeInit(&openFileName, hWnd);
        if (GetOpenFileNameW(&openFileName)) {
            Python = new EmbeddedPython(openFileName.lpstrFile, hWnd);
        }
        else {
            // If python isn't supplied, shut down program
            OpenFileNameClose(&openFileName);
            DestroyWindow(hWnd);
            return 0;
        }
        OpenFileNameClose(&openFileName);
        if (Python->ErrorCode != 0) {
            errorCode = Python->ErrorCode;
            delete Python;
            Python = NULL;
            HWND hErr = CreateWindowW(L"errorBox", L"Python Initialization Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, NULL, NULL, NULL, NULL);
            ShowWindow(hErr, SW_NORMAL);
            DestroyWindow(hWnd);
            
        }
        InitMenu(hWnd);
        InitControls(hWnd);
        RestoreMenuOptions(hWnd, MF_GRAYED);
        break;
    case PM_GRAPHDONE:
        // Get error code
        errorCode = Python->ErrorCode;

        // If error, display it
        if (errorCode != 0) {
            HWND hErr = CreateWindowW(L"errorBox", L"Python Graph Error", ERROR_WINDOW, ERROR_BOX_X, ERROR_BOX_Y, ERROR_BOX_WIDTH, ERROR_BOX_HEIGHT, hWnd, NULL, hInst, NULL);
            ShowWindow(hErr, SW_NORMAL);
        }
        
        break;
    case PM_PYTHONSET:
        // If python has been setup, disable any changes to it
        ControlPythonConfig(hWnd, MF_GRAYED);
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        delete Python;
        Python = NULL;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
