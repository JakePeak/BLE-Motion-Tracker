
Python Embedding Information
============================

Getting it Operational
----------------------------

In the windows application, I chose to embbed python using the c-python api,
which ultimately had many issues.  While most of this will work out of the
box, there are a couple steps that must be done.

1) Edit the path to <Python.h> to your own respective python.h
	- Make sure to have Numpy and Pandas installed

2) Edit the path to <Eigen\Dense> to your own eigen install
	- This is a rather easy library to download, so I didn't include
	  it in the final project

Notable Pitfalls and Bugs
----------------------------

Throughout the development of this project, the python embedded installation
took much longer than was anticipated due to a combination of external modules
and a nontrivial API.  Should anyone attempt to build their own embedded python
installation using this API, here are some recommentations.

1) External Modules (Numpy, MatplotLib) aren't designed to work well with this
API set.  These two modules gave me countless troubles, and here were my
solutions.
	- Numpy can only be initialized once
		- If you want to embed Numpy, Don't repeatedly Py_FinalizeEx()
		  and Py_Initialize().  As part of its startup, it only works
		  the first time.

	- Matplotlib rescales the application's DPI and steals focus
		- At least with win32, calling on Matplotlib is very dangerous.
		  SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
		  is was required, as it would rescale windows if they didn't
		  adapt to the system DPI that matplotlib would overlap
		- As well, if building your own animation method, use rcParams.
		  Specifically, "figure.raise_window" = False.  Without this,
		  every time the plot resets it steals focus, stoping the
		  user from accessing the rest of the application.

2) In my experience, I was unable to get the embedded windows distribution of
Python to work.  I believe this is because the API doesn't look for standard
modules in .pyc files, so it cannot import any modules.  Outside of the simplest
implementations, it is required to have a full version of python "embedded"
into the software, which can be a large space constraint, hence why it wasn't
used.

3) Reference counts are powerful and if used incorrectly, the source of many
bugs.  I strongly recommend anyone familiarize themselves with that 
documentation.

Content Overview
----------------------------

The heart of the python distribution lies in MakeGraph.py.  Small but mighty,
it has code for everything necessary.  Debug.py was a simple debug script
used for debugging, as well as PythonSetup.py.  libs and DLLs are self 
explanitory.  Finally, data3D.csv is an example dataset that can be run from
the application individually for testing without building the actual device.
