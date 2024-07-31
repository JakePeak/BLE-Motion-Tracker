// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
#define __STDC_WANT_LIB_EXT1__ 1
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
// User includes
#include <bluetoothleapis.h>
#include <bthledef.h>
#include <stdint.h>
#include <Bthsdpdef.h>
#include <setupapi.h>
#include <cstddef>
#include <windowsx.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <commdlg.h>
// Include C-Python APIs
//#define Py_LIMITED_API
#define PY_SSIZE_T_CLEAN
#include <C:/Users/akj56/AppData/Local/Programs/Python/Python312/include/Python.h>
// String libraries
#include <iostream>
#include <fstream>
#include <ios>
#include <iomanip>
#include <sstream>
#include <cstdlib>
// Includes manually added

#include <Eigen/Dense>

#pragma comment(lib, "bthprops.lib")
#pragma comment(lib, "BluetoothApis.lib")
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "python312_d.lib")
#pragma comment(lib, "python3_d.lib")