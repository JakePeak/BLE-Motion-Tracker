Overview of the Windows Application
=========================================

This directory contains the visual studio 2022 project in which the laptop
application which interacts with the motion tracker resides.  This document
consists of 5 segments: how to run the project in visual studio, how to
use the project, skills involved, largest bugs, and references.

How to Run in Visual Studio
-----------------------------------------
While the project should work fine on its own, any attempt to compile will
inevitably fail.  This project is dependent on two external libraries, the
C-python API and Eigen.  When I constructed this, I manually installed them
into the Visual Studio default library location, which would let me use them
on further projects.  If you would rather not, I recommend downloading both
libraries and altering the framework.h header file with the necessary paths
and .dlls.  Since this isn't designed to be a public utility, but rather a
skills showcase, hard-coding my personal setup made the most sense rather
than reuploading those APIs to Github.

Using the Project
-----------------------------------------
To start this project, double click or run the compiled executable from the
command line.  It should prompt you to select your python application.  While
I originally empedded python into the project, the embedded distribution 
has compatibility issues with importing libraries using the C-Python APIs, so
rather than embedded a full python download I opted to use the user's python.

After this, the application window should open.  It is rather simple, as 
this project focuses on the lower levels of the full-stack implementation.
The menu is rather reduntant, however the python tab is useful for configuring
the embedded python.  As for the buttons:
- Connect     --> connects the application to the device
- Pause       --> Pause the devices data collection (enters pause state)
- Unpause     --> Starts data collection (enters active state)
- Battery     --> Outputs current battery percentage
- Import File --> Select an external file to graph, ie. prior saved data
- Set Output  --> Allows for changing from the default output CSV
- Disconnect  --> Disconnects the application from the device

If the device has already been connected to the PC by finding it under the
extended connectable bluetooth devices list, then connect should complete
without error.  Then simply select unpause to start graphing.  The device
first will calibrate, which is signaled by the LED turning on until
calibration is complete.  Then data will be plotted live, until the window
is exited or the pause button is selected.

Skills Involved
-----------------------------------------
In creating this application, I primarily learned about different API sets,
as well as the windows system.

1) C++ coding with Object Oriented Programming (OOP)
	The way python interaction and bluetooth device interaction was coded
	closely followed the principles of OOP, and helped developed my
	programming skills.

2) win32 API set
	I used a large swath of the win32 api.  The most notable were the 
	BLE device node APIs, the device enumeration APIs, and the windowing
	APIs.  To help understand these and get my device to connect to them,
	I also researched the win32 device driver stack, although due to time
	constraints and the steep learning curve I left it for a later projet.
	Out of these API sets, the device enumeration APIs are the most
	generally useful, as I see myself working with other forms of device
	I/O in the future.

3) C-Python APIs
	After using these APIs, I strongly discourage their use.  They 
	natively introduced a host of bugs and compatibility issues, 
	especially with library imports.  However, I do find it very 
	appealing to be able to call python code from C, and could see myself
	utilizing them in the future, now that I know many of the issues to
	avoid when using them.

4) Eigen
	I used a little bit of Eigen and linear algebra to estimate position
	using the Inertial Measurement Unit.  The mathematics was interesting,
	and I found Eigen to be a very consistent solution to my problems.

5) Multithreading
	While I have already covered multithreading in advanced coursework,
	I opted to somewhat utilize it in this project.  The two locations
	where I used it was operating an independent 'python' thread and
	using asynchronous file IO to streamline data throughput over BLE.

6) Full-Stack Development
	The massive scope of this project as a whole provided an introduction
	to full-stack development unlike one any normal project could deliver.

Bugs
-----------------------------------------
As I have covered the numerous python bugs in that respective README, I will
not review them here.

- LEEnumerator driver
	The windows-provided basic BLE driver has numerous pitfalls, and almost
	pushed me to design my own.  If it doesn't detect a GATT profile
	device, then it will disconnect the device and won't permit 
	connections.  While not an elegent solution, I found the only way shy
	of designing my own driver was to build a mock HID Over Gatt Profile
	(HOGP) profile on my device.  While I followed all regulations and
	standards, I very much dislike the need for this solution simply 
	because the LEEnumerator driver doesn't need to have this arbitrary
	requirement, custom services work just fine if it detects any GATT
	profile.

- Dots Per Inch (DPI) nonfunctional with MatPlotLib
	When attempting to use the visual studio example code as the foundation
	of my windowing, I ran into an issue where after making MatPlotLib 
	calls my window would randomly resize.  I was able to track this issue
	down to an application error where it didn't inately use the system's
	DPI.  I suspect this issue could happen for many other reasons, and
	the solution was to add:
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE)
	as the application initialized.

- Bluetoothleapis.h poor documentation
	While not a bug, the documentation deserves it's own discussion.  As
	the only non-UWP microsoft API set to interact with BLE devices, it
	should have better documentation.  Notably, the documentation often
	confuses device handles with service handles.  If running into bugs
	with this API set, check which seems more intuitive first, as the 
	documentation sometimes directly contradicts itself as to which is 
	correct.

As with any project, bugs are commonplace, however these were some of the most
prominent bugs encountered.  That being said, I found Visual Studio 2022 to be
a very bug-free IDE which helped me solve my problems quickly and efficiently.

References
-----------------------------------------

The win32 documentation is very extensive, and for the most part, very good.
Below is the link to their error documentation, as sometimes the application
reports win32's error codes.
https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-

To help learn how use the windowing APIs, I found "The PentaMollis Project"'s 
youtube series to be a lifesaver to quickly introduce the important features
of this vast API set.
https://www.youtube.com/playlist?list=PLWzp0Bbyy_3i750dsUj7yq4JrPOIUR_NK

The C-Python API reference manual, while not as thorough as it could be, was
essential for debugging, and is linked below.
https://docs.python.org/3/c-api/index.html

Bluetooth SIG does a great job documenting all of the defined profiles, and
allowed me to implement my ghost HOGP profile to get around LEEnumerator's 
defficiencies.
https://www.bluetooth.com/specifications/specs/
