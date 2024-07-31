On-Chip Firmware
=======================================

One of the main reasons behind this project was an opportunity to teach myself
embedded software/firmware and flashing.  Thoughout this project, I will refer
to any embedded software as firmware, as firmware is a loosely defined concept.
In this directory, there are 4 folders.  Debug_LED contains a WISE Studio
project that was used to unit test and debug on-board functions and peripherals.
The rest were used as modifications to the BLE_SensorDemo demo project provided
by the WISE Studio software development kit.  This README will contain 5
sections: design overview, running the project in the IDE, skills involved, 
bugs, and references.

Design Overview
---------------------------------------

The initial concept behind the design is a 3-state state machine.  After an
on-boot initialization process, the device would enter a "connect" state, 
where it would try and develop a connection.  Once a connection is found, it
enters a "pause" state.  From the pause state, the user can activate the device,
moving into the "active" state where all peripherals are running and data is 
being send over bluetooth low energy to the external application.  The user
can toggle between pause and active states, and should the user disconnect the
device will reenter the "connect" state.

On a peripheral level, the microcontroller interacts with an LED using GPIO,
and a accelerometer using SPI.  Data is collected every select interval using
a timer through individually reading each data register.  As well, the 
microcontroller contains a build-in ADC bridge to measure the input voltage,
which is used alongside a model of output voltage decay in AAA Alkaline 
batteries to determine the battery percentage.

This device contains 6 GATT Services.  The first two are GATT and GAP services
for establishing connection.  The next 3 are the battery service, HID service, 
and device information service.  The battery service was used to hold the
battery level characteristic.  However, the HID service and device information
services are both skeleton services with little functionality, as windows has
poor support for BLE devices, which will be discussed in greater detail in the
bugs section.  The final service is a custom IMU service.  It contains 3 
characteristics: a boolean, used to change between active and pause states;
Sensor location and Speed, used to hold IMU data; a custom notify, which sends
a notification for the application to initiate a long read on the IMU data.

Running the Project
----------------------------------------

Inside BLE_SensorDemo/Release there are binary and hex files that can be
flashed without opening the project, should one desire.  Beyond this, running
the IDE will take some work.  First, download WISE Studio and its SDK off of
ST's website.  Then, opening the CPROJECT with wise studio contained within
this repository might work.  If it doesn't, then one must link the 
Additional_Source_Files to the project, specifically by right-clicking and
following the menu on the folder containing the main() function.  Doing this
project-wide breaks the SensorDemo_Config.h file, and with it all GATT 
functionality.  Afterwareds, copy the text of all the Aletered_Source_Files
into their respective files.  sensor.c contains all BlueNRG event callbacks.
SensroDemo_config.h contains the BLE stack memory allocation, which is very
buggy and the reason why I migrated my files from my own project to a demo
project.  SensorDemo_main.c contains the main function.  This should resolve
all errors, and allow for the code to be modified in the native IDE.

Debug_LED should just work, as it isn't a demo project since it has no 
bluetooth functionality.

Skills
----------------------------------------

- Embedded Firmware Design
	The core of this project was learning how to design firmware for
	microcontrollers.  I learned about numerous peripherals, register
	access, compiling, config, and flashing.  WISE studio is much more
	hands-on than the classic STM32CubeIDE, and on multiple dead end
	paths I ended up using it as well, so can confidently say that I
	am experienced in both IDEs, and with a greater understanding than most
	who simply use STM32CubeIDE for ST's products.

- Bluetooth Low-Energy Stack
	Bluetooth LE is a complex topic involving many standards and rules
	to follow.  Regardless of the microcontroller chosen, I am confident
	in my ability to design for BLE and execute.

- Flashing
	In this project, I used the built in UART flasher simply becuase it
	was cheaper than investing in a JTAG or other standard flasher.  
	However, after learning one method I am confident in my ability to 
	learn another, and have added microcontroller flashing to my skillset

- C Programming
	Wihle I already consider myself a skilled C programmer, practice makes
	perfect and writing/debugging a couple thousand lines of code on the 
	chip alone once again stands as a testament to my abilities.

- HID protocol
	Building the skeleton HID strucutre required me to learn the 
	foundations of HID, allowing me to make other devices HID-compliant

Bugs
----------------------------------------

1) WISE studio's BlueNRG_Stack_Initialization();
	The majority of this project was designed in a custom project rather
	than a demo project.  This was a much cleaner solution, but it ran
	into an issue.  The BlueNRG_Stack_Initialization function would always
	return an error.  I was unable to trace the root of this error beyond
	the User_config.h file, and like every other person I could find online,
	resorted to modifying a built-in demo application.  There is something
	innately wrong with WISE studio's custom projects, and I would like
	to document this as a warning for others using BlueNRG modules.

2) HOGP profile
	I have already discussed this under the windows_application readme,
	but there is more to it.  I was unable to translate all parts of this
	project into a HID device, as windows BLE doesn't perform a long read
	of the HID Report map.  As I couldn't alter the MTU due to limitations,
	my only option was a skeleton HOGP.  I believe this to be a bad 
	implementation, but I could see no way around it other than custom
	driver development

3) Burning IMU QFN package
	This project involved an accelerometer with a small footprint and a
	QFN package, which caused many setbacks.  As a hobbyist, my only option
	was to solder paste and heat gun.  This resulted in me burning 3/4 of
	my IMUs, to various degrees.  I would recommend avoiding QFN packages
	when possible, and if not like my situation, find and use a soldering
	specific heat gun with controllable temperatures.  Be it disfunctional
	SPI, broken pads, or busted MEMs devices damaged chips provided a host
	of issues.  While estimating position from IMU readins is already a 
	rather flawed system, damaged IMUs made my final project much less
	impressive, as the live location graphing was usually inaccurate.

4) Accelerometer SPI FIFO
	While polling individual registers works, it is much more elegant to
	read all registers at once through the accelerometer-provided FIFO.
	However, after a long time debugging this approach was abandoned.
	My accelerometer didn't like gaps between SPI data frames, and 
	my microcontroller supported a max data frame size of 32 bits, so
	I was sadly unable to get a reading out of the FIFO.  I left the
	FIFO config instructions in the final project, despite being unable
	to utilize them.

References
----------------------------------------

The microcontroller used, the BlueNRG-M2SA module, was very convenient in that
it could be hand-soldered.  While it had many bluetooth issues and limitations,
I would recommend it for hobbyists simply for the manufacturability.  From this
link, one can find the RF flasher utility, WISE studio, and the SDK.
https://www.st.com/en/wireless-connectivity/bluenrg-m2.html#documentation

The accelerometer has a host of useful functions, called APEX motion functions.
As well, the non-burnt gyroscope axis managed to be accurate to around 1/1000 
of a radian per second, so I do believe it to be a very powerful device.  Just
be cautious with temperature.
https://product.tdk.com/system/files/dam/doc/product/sensor/mortion-inertial/imu/data_sheet/ds-000451-icm-42670-p.pdf

