#include "Python_Embed.h"
#include "framework.h"

/*
 * @brief	Constuctor for the EmbeddedPython class, starts the python interpreter and imports our module and functions
 * @param	WCHAR* pythonExePath	=> path to the python.exe file that we will use to run out python
 * @param	HWND _windowHandle		=> handle to the application window, used for sending messages
 * @retval	Outputs a bitmap of errors
 */
EmbeddedPython::EmbeddedPython(WCHAR* pythonExePath, HWND _windowHandle) {
	// Raw initialize variables before errors can occur
	WindowHandle = _windowHandle;
	PythonActive = false;
	GraphActive = false;
	ThreadHandle = NULL;
	PythonSource = NULL;
	SourceFilePath = NULL;
	ErrorCode = 0;

	// Initialize PythonSource
	if (!SetPythonPath(pythonExePath)) {
		ErrorCode |= 0x01;		// Python path initialization failure
		return;
	}

	// Initizlize CSV
	std::string tempString = {};
	SetCSVPath(tempString, NONE);


	// Get path to executable
	WCHAR* buf;
	GetExecPath(&buf);
	if (buf == NULL) {
		ErrorCode |= 0x02;		// Memory allocation error
		return;
	}
	size_t stringSize = wcslen(buf);
	// Delete the back of the string to start gathering python path and home directories
	stringSize -= wcslen(L"BLE_Computer_Interaction\\x64\\Release\\");
	buf[stringSize] = '\0';
	// Move into directory contining python source code
	stringSize += wcslen(L"Python_Source");
	wcscat_s(buf, stringSize + 1, L"Python_Source");
	SourceFilePath = (char*)malloc((stringSize + 1) * sizeof(char));
	if (SourceFilePath == NULL) {
		ErrorCode |= 0x04;		// Memory allocation error
		return;
	}
	// Transfer from wchar to char
	wcstombs_s(NULL, SourceFilePath, (stringSize + 1) * sizeof(char), buf, stringSize * sizeof(WCHAR));
	free(buf);

	// Build graphing thread
	Command = TRUEWAIT;
	ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EmbeddedPython::GraphingThread, this, 0, NULL);

	return;
}

/*
 * @brief	Destructor function, shuts down python on frees relevant memory
 */
EmbeddedPython::~EmbeddedPython() {
	// Close down the graphing thread
	int counter = 0;
	// Signal shutdown and mark thread as inactive
	if (!SendCommand(FINALIZE)) {
		// If timeout, shut down thread manually
		TerminateThread(ThreadHandle, 1);
	}
	DWORD ThreadStatus = STILL_ACTIVE;
	while (ThreadStatus == STILL_ACTIVE) {
		GetExitCodeThread(ThreadHandle, &ThreadStatus);
		Sleep(100);
		counter++;
		if (counter > 50) {
			TerminateThread(ThreadHandle, 1);
		}
	}
	CloseHandle(ThreadHandle);
	free(SourceFilePath);
	free(CSV);
	free(PythonSource);
}

/*
 * @brief	Starts up the python interpreter when necessary, signalling no more changes can be made to it
 * @retval	Outputs an error bitmap, the definitions of each are defined in the thread code, although the 
 *			most common are listed here
 *				- 0x004 => Numpy import error
 *				- 0x010 => Primary module import error
 *				- 0x002 => Error initializing python, perhaps bad path? 
 */
void EmbeddedPython::StartPython() {
	// If python already active, don't try again
	ErrorCode = 0;
	if (PythonActive) {
		ErrorCode |= 0x01;
		return;
	}
	if (!SendCommand(INITIALIZE)) {
		ErrorCode |= 0x02;
		return;
	}
	
}

/*
 * @brief	Starts graphing the CSV file specified by the CSV file path, assuming such a file exists.
 *			Does this by signalling the graphing thread
 */
void EmbeddedPython::GraphData() {
	// If no CSV exists, then do nothing
	if (CSV == NULL) {
		OutputDebugStringW(L"No CSV Detected\n");
		return;
	}
	// If a graph is active, shut it down
	if (GraphActive) {
		if (!SendCommand(WAIT)) {
			OutputDebugStringW(L"Graph Shutdown Failed\n");
			return;
		}
	}

	// Need python, so start it if inactive
	StartPython();
	ErrorCode = 0;
		
	// Open start the new graph
	SendCommand(NEWGRAPH);
}

/*
 * @brief	Gets the CSV file path and opens the file, updating relevant class members
 * @param	string input => input string provided, use depends on next argument
 * @param	FILE_MODE mode => enum describing input string
 *		NONE => unimportant, using default settings
 *		NAME => file name, without '\' or ".csv"
 *		PATH => full path to document (document likely already exists)
 */
void EmbeddedPython::SetCSVPath(std::string& input, FILE_MODE mode) {
	// If there is currently a graph, shut it down
	if (GraphActive) {
		SendCommand(WAIT);
	}
	delete CSV;
	// If a full path is provided, use it
	if (mode == PATH) {
		CSV = new std::string(input);
	}
	// Else get the current directory and use that to create the path
	else if (mode == NAME || mode == NONE) {
		// Get the current directory into a string
		WCHAR* buf;
		int buf_size = GetExecPath(&buf);
		char* outBuf = (char*)malloc((buf_size + 1) * sizeof(char));
		// Append the string with the filename
		if (buf != NULL && outBuf != NULL) {
			wcstombs_s(NULL, outBuf, (buf_size + 1) * sizeof(char), buf, (buf_size + 1) * sizeof(WCHAR));
			CSV = new std::string(outBuf);
			if (mode == NAME) {
				*CSV += input;
			}
			else {
				*CSV += DEFAULT_NAME;
			}
			*CSV += ".csv";
		}
		free(buf);
		free(outBuf);
	}
}

#if __has_include("BLE_Communication.h")
/*
 * @brief	Uses the CSV referenced by a BLE_Device.  If this device is deleted, it is the user's responsibility
 *			to assign a new path
 * @param	BLE_Device* Device => Device from which CSV is taken/referenced
 */
void EmbeddedPython::SetCSVPath(BLE_Device* Device) {
	// If there is a graph, shut it down
	if (GraphActive) {
		SendCommand(WAIT);
	}
	CSV = new std::string(Device->GetFilePath()->c_str());
}
#endif

/*
 * @brief	Sets the path to the python executable currently in use, can be called to reset the path if one exists
 * @param	WCHAR* input => Unicode string containing path to the python.exe executable
 * @retval	True on success, false on failure.
 */
bool EmbeddedPython::SetPythonPath(WCHAR* input) {
	// If python is already running, do nothing
	if (PythonActive) {
		return false;
	}

	// Get rid of old PythonSource, if it exists
	free(PythonSource);

	// Copy new string into buffer
	size_t bufLen = wcslen(input);
	PythonSource = (WCHAR*)malloc((bufLen + 1) * sizeof(WCHAR));
	if (PythonSource == NULL) {
		return false;
	}
	wcscpy_s(PythonSource, bufLen * sizeof(WCHAR), input);

	// Cut off \python.exe, as we just want the home directory
	bufLen -= wcslen(L"\\python.exe");
	PythonSource[bufLen] = L'\0';
	OutputDebugStringW(PythonSource);
	return true;
}

/*
 * @brief	Static member function used for multithreading the graphing process with Python and building
 *			the graph.  This function represents the heart of this python object, as this thread performs
 *			all python calls, and all other member functison simply send commands to this thread.
 * 
 * @param	EmbeddedPython* Python => Python that should be used for graphing, aka this pointer
 * @retval	Should the function fail, returns relevent error code to Python->ErrorCode
 */
void EmbeddedPython::GraphingThread(EmbeddedPython* Python) {
	PyObject* Numpy = NULL, * Pandas = NULL, * Matplotlib = NULL, * Module = NULL;
	PyObject* MakeGraph = NULL, * Redraw = NULL, * DeleteGraph = NULL;
	PyObject* args = NULL, * tuple = NULL, * graph = NULL, * axis = NULL, * FuncReturn = NULL;
	PyConfig PyConfiguration;
	LARGE_INTEGER smallWait;
	HANDLE timer;
	int counter = 0, funcTest = 1;
	volatile bool timeout;
	while (1) {
		switch (Python->Command) {
		case WAIT:		// Get signalled to wait and do nothing
			Python->CommandComplete = true;
			Python->Command = TRUEWAIT;
			break;
		case TRUEWAIT:  // Wait and do nothing
			Sleep(10);
			break;
		case NEWGRAPH:	// Build a new graph
			Python->ErrorCode = 0;
			Python->Command = TRUEWAIT;
			Python->GraphActive = true;

			// If python has yet to be initialized, do nothing
			if (!Python->PythonActive){
				Python->ErrorCode |= 0x08000;
				Python->CommandComplete = true;
				break;
			}

			counter = 0;
			funcTest = 1;

			// Create the animation
			args = PyTuple_Pack(2, PyUnicode_FromString(Python->CSV->c_str()), PyLong_FromLong(GRAPH_DELAY));
			tuple = PyObject_CallObject(MakeGraph, args);
			if (tuple == NULL) {
				// For some unknown reason MakeGraph gets dereferenced/garbage collected, just restocks if needed
				Py_XDECREF(tuple);
				Py_XDECREF(MakeGraph);
				MakeGraph = PyObject_GetAttrString(Module, "MakeGraph");
				tuple = PyObject_CallObject(MakeGraph, args);
				if (tuple == NULL) {
					Py_XDECREF(args);
					Py_XDECREF(tuple);
					Python->ErrorCode |= 0x01;			// Error, MakeGraph likely doesn't exist/cannot be accessed  
					Python->GraphActive = false;
					Python->Command = TRUEWAIT;
					Python->CommandComplete = true;
					PostMessageW(Python->WindowHandle, PM_GRAPHDONE, NULL, NULL);
					break;
				}
			}

			Py_DECREF(args);
			graph = Py_NewRef(PyTuple_GetItem(tuple, 0));
			axis = Py_NewRef(PyTuple_GetItem(tuple, 1));
			args = PyTuple_Pack(4, PyFloat_FromDouble(((double)THREAD_CHECKINS) / ((double)1000)), axis, PyUnicode_FromString(Python->CSV->c_str()), graph);

			// Build a timer to implement a partially blocking timed delay
			timeout = false;
			smallWait.LowPart = -1 * ((THREAD_CHECKINS * 10000) - 10000);
			smallWait.HighPart = -1;
			
			timer = CreateWaitableTimerW(NULL, TRUE, L"Graph Timer");
			// If timer creation fails, shut down the process
			if (timer == NULL) {
				PyObject_CallObject(DeleteGraph, graph);
				Py_XDECREF(args);
				Py_XDECREF(graph);
				Py_XDECREF(tuple);
				Py_XDECREF(axis);
				Python->ErrorCode |= 0x02;			// Timer Creation error
				Python->GraphActive = false;
				PostMessageW(Python->WindowHandle, PM_GRAPHDONE, NULL, NULL);
				Python->Command = TRUEWAIT;
				Python->CommandComplete = true;
				return;
			}
			// Configure timer
			SetWaitableTimer(timer, &smallWait, THREAD_CHECKINS, (PTIMERAPCROUTINE)EmbeddedPython::TimerCallback, (void*)&timeout, FALSE);
			Sleep(1000);
			// Infinite while loop until shut down
			Python->CommandComplete = true;
			while (1) {
				// Infinite loop dependent on timer function to make code blocking
				while (timeout && (funcTest != 0)) {
					// Draw the graph
					FuncReturn = PyObject_CallObject(Redraw, args);
					// Check if still active
					funcTest &= PyLong_AsLong(FuncReturn);
					Py_XDECREF(FuncReturn);
				}
				timeout = true;
				// Every timer call, make a check to see if graph should be shut down

				// Application called graph to stop
				if (Python->GraphActive == false || funcTest == 0 || counter > THREAD_TIMEOUT) {
					// Python call to delete python objects
					if (Python->GraphActive == false || counter > THREAD_TIMEOUT) {
						FuncReturn = PyObject_CallNoArgs(DeleteGraph);
					}
					Py_XDECREF(graph);
					Py_XDECREF(axis);
					Py_XDECREF(args);
					Py_XDECREF(tuple);
					CancelWaitableTimer(timer);
					CloseHandle(timer);
					Python->GraphActive = false;
					PostMessageW(Python->WindowHandle, PM_GRAPHDONE, NULL, NULL);
					break;
				}

				counter++;
			}
			break;
		case INITIALIZE:	// Initialize the python interpreter
			// If python has already been initialized, do nothing
			if (Python->PythonActive == true) {
				Python->CommandComplete = true;
				break;
			}
			PyStatus status;
			// Begin running python
			PyConfig_InitPythonConfig(&PyConfiguration);
			PyConfig_SetString(&PyConfiguration, &PyConfiguration.home, Python->PythonSource);
			status = Py_InitializeFromConfig(&PyConfiguration);
			if (PyStatus_Exception(status)) {
				Python->ErrorCode |= 0x02;			// Initialization error
				return;
			}
			// Tell application Python has been initialized
			PostMessageW(Python->WindowHandle, PM_PYTHONSET, NULL, NULL);

			// Append location of our module to the path
			PyObject* path, * curPath;
			path = Py_NewRef(PySys_GetObject("path"));
			curPath = PyUnicode_FromString(Python->SourceFilePath);
			PyList_Insert(path, 0, curPath);

			// Get python modules
			Numpy = PyImport_ImportModule("numpy");
			if (Numpy == NULL) {
				Python->ErrorCode |= 0x04;			// Various error codes corresponding to which module failed to be located
			}
			Pandas = PyImport_ImportModule("pandas");
			if (Pandas == NULL) {
				Python->ErrorCode |= 0x08;
			}
			Matplotlib = PyImport_ImportModule("matplotlib");
			if (Matplotlib == NULL) {
				Python->ErrorCode |= 0x010;
			}
			Module = PyImport_ImportModule(MODULE_NAME);
			if (Module == NULL) {
				Python->ErrorCode |= 0x020;
				PyObject* tuple;
				WCHAR* buf;
				int counter, funcTest;
				// Debugging code left in, prints out all path directories to debug output
				int buf_size = GetCurrentDirectoryW(0, NULL);
				buf = (WCHAR*)malloc(buf_size * sizeof(WCHAR));
				GetCurrentDirectoryW(buf_size, buf);
				OutputDebugStringW(buf);
				OutputDebugStringW(L"\n");
				counter = PyList_Size(path);
				for (int i = 0; i < counter; i++) {
					tuple = PyList_GetItem(path, i);
					funcTest = PyUnicode_GetLength(tuple);
					OutputDebugStringW(PyUnicode_AsWideCharString(tuple, (Py_ssize_t*)&funcTest));
					OutputDebugStringW(L"\n");
				}
				free(buf);
			}

			// Stop if any module import errors
			if (Python->ErrorCode != 0) {
				Py_XDECREF(path);
				Py_XDECREF(curPath);
				Python->CommandComplete = true;
				break;
			}

			// Get defined functions
			MakeGraph = PyObject_GetAttrString(Module, "MakeGraph");
			if (MakeGraph == NULL) {
				Python->ErrorCode |= 0x040;			// More error codes indicating if functions failed to be loaded
			}
			DeleteGraph = PyObject_GetAttrString(Module, "DeleteGraph");
			if (DeleteGraph == NULL) {
				Python->ErrorCode |= 0x080;
			}
			Redraw = PyObject_GetAttrString(Module, "Redraw");
			if (Redraw == NULL) {
				Python->ErrorCode |= 0x0100;
			}

			// Stop if any function import errors
			Py_XDECREF(path);
			Py_XDECREF(curPath);
			if (Python->ErrorCode != 0) {
				Python->CommandComplete = true;
				break;
			}
			Python->PythonActive = true;
			Python->Command = TRUEWAIT;
			Python->CommandComplete = true;
			break;
		case FINALIZE:
			if (Python->PythonActive) {
				Py_XDECREF(Numpy);
				Py_XDECREF(Pandas);
				Py_XDECREF(Matplotlib);
				Py_XDECREF(Module);
				Py_XDECREF(MakeGraph);
				Py_XDECREF(Redraw);
				Py_XDECREF(DeleteGraph);
				Py_FinalizeEx();
			}
			PyConfig_Clear(&PyConfiguration);
			Python->CommandComplete = true;
			return;
		}
	}
}

/*
 * @brief	Simple timer callback function to implement a nonblocking delay
 * @param	LPVOID arg => arg passed to timer, in this case bool* set false to escape wait loop
 */
void EmbeddedPython::TimerCallback(LPVOID arg, DWORD lowTime, DWORD highTime) {
	bool* timeout = (bool*)arg;
	*timeout = false;
}

/*
 * @brief	Helper funtion that outputs the path to the folder containing the current exeutable (must be freed)
 * @param	(out) WCHAR** out => outputs a unicode string to the current executable, returns NULL on failure
 * @retval	Size of out, returns 0 on failure
 */
int EmbeddedPython::GetExecPath(WCHAR** out) {
	WCHAR* temp;
	int bufSize = 300;
	WCHAR* buf = (WCHAR*)malloc(bufSize * sizeof(WCHAR));
	if (buf == NULL) {
		OutputDebugStringW(L"Memory Allocation Error\n");
		return 0;
	}
	else if (GetModuleFileNameW(NULL, buf, bufSize - 1) == 0) {
		DWORD err = GetLastError();
		if (err != ERROR_INSUFFICIENT_BUFFER) {
			OutputDebugStringW(L"Path Retrieval Error\n");
			free(buf);
			*out = NULL;
			return 0;
		}
		while (err == ERROR_INSUFFICIENT_BUFFER) {
			bufSize += 100;
			temp = (WCHAR*)realloc(buf, bufSize * sizeof(WCHAR));
			if (temp == NULL) {
				OutputDebugStringW(L"Memory Allocation Error\n");
				free(buf);
				return 0;
			}
			buf = temp;
			if (GetModuleFileNameW(NULL, buf, bufSize - 1) != 0) {
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

/*
 * @brief	Sends a command to the python thread
 * @param	GRAPH_COMMAND => enum command to send
 * @retval	true on success, false on timeout
 */
bool EmbeddedPython::SendCommand(GRAPH_COMMAND _Command) {
	if (_Command == TRUEWAIT) {
		_Command = WAIT;
	}
	GraphActive = false;
	CommandComplete = false;
	Command = _Command;
	int counter = 0;
	while (!CommandComplete) {
		Sleep(100);  // Wait .1 seconds
		counter++;
		// After 5 minutes, timeout
		if (counter > 3000) {
			ErrorCode |= 0x020000;  // Timeout Errorcode
			return false;
		}
	}
	if (ErrorCode != 0) {
		return false;
	}
	return true;
}