#include "framework.h"
#if __has_include("BLE_Communication.h")
#include "BLE_Communication.h"
#endif
#include "Windowing.h" // Used for sending messages to application, can be removed/replaced if necessary

// Python adjustable macros
#define MODULE_NAME "MakeGraph"
#define GRAPH_DELAY (long)1000 // Animation delay in ms
// Python unadjustable macros
#define THREAD_CHECKINS 100  // Time between when thread checks to close graph in ms
#define THREAD_TIMEOUT 600000/THREAD_CHECKINS // 10 minute timeout before thread automatically shuts down

// Additional Windows Messages
#define PM_GRAPHDONE	0x6400
#define PM_PYTHONSET	0x6401

// Application name
#ifndef EXECUTABLE_NAME
#define EXECUTABLE_NAME L"BLE_Computer_Interaction.exe"
#endif
#ifndef DEFAULT_NAME
#define DEFAULT_NAME "BLE_output_file"
#endif

typedef enum {INITIALIZE, FINALIZE, NEWGRAPH, WAIT, TRUEWAIT} GRAPH_COMMAND;

/*
 * @brief	Embedded python object, representing the python interpreter and how it can be used.  Methods are discussed
 *			in Python_Embed.cpp.  In general, this object wraps around a single python thread, where methods simply
 *			change the state of said thread.  This avoids any GIL and module import based errors.
 */
class EmbeddedPython {
private:
	GRAPH_COMMAND Command;
	bool CommandComplete;
	HWND WindowHandle;
	std::string* CSV;
	bool PythonActive;
	bool GraphActive;
	HANDLE ThreadHandle;
	WCHAR* PythonSource;
	char* SourceFilePath;

	// Private helper functions
	static int GetExecPath(WCHAR** out);
	void StartPython();
	bool SendCommand(GRAPH_COMMAND _Command);
public:
	int ErrorCode;
	
	// Public Methods
	EmbeddedPython(WCHAR* pythonExePath, HWND _windowHandle);
	~EmbeddedPython();
	void GraphData();
	void SetCSVPath(std::string& input, FILE_MODE mode);
#if __has_include("BLE_Communication.h")
	void SetCSVPath(BLE_Device* Device);						// Removable function overload for using a device's CSV
#endif
	bool SetPythonPath(WCHAR* input);

	// Callback functions, used for multithreading
	static void GraphingThread(EmbeddedPython* Python);
	static void TimerCallback(LPVOID arg, DWORD lowTime, DWORD highTime);

	// copy, default, and copy assignment methods removed, should only ever have one active EmbeddedPython instance running
	EmbeddedPython() = delete;
	EmbeddedPython(EmbeddedPython const& device2) = delete;
	void operator=(EmbeddedPython const& device2) = delete;
};