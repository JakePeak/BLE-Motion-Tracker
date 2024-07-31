PCB Design
=================================

This folder contains the two iterations of my PCB design.  The first was never
ordered, but played a crucial role in the design process.  When designing, I
chose to use Altium, as I have used it though Illini Electric Motorsports.

Verson 1
---------------------------------

The first version of the PCB was much smaller.  It assumed components would
be mounted externally, and would be wired and soldered into vias.  As well,
I designed it before I knew the feasibility of soldering QFN packages for the
accelerometer, so left vias to manually wire into the accelerometer.  I decided
this was a poor solution, hence the redesign.  As well, the PCB manufacturer
I used had limited support for small boards, and my board was too small.  This
led me to redesign the board and make it larger, leaving space for more 
mounting holes.

Version 2
---------------------------------

The final version of the PCB.  It features additional mounting holes to 
directly mount the battery pack to the PCB, which while not a very resilient
solution was very convenient for keeping all electronic components together
during software/firmware development.  The PCB features a flashing switch, as
well as an on/off switch.  It has a USB-UART connector for utilizing the
microcontrollers UART bootloader.  It also uses an LED/resistor output, to
help debug and calculated to have low power consumption.  As well, SPI lines
are routed from the PCB to the accelerometer.  All other resistors are pull-up
resistors, and all capacitors are bypass capacitors.  J2 was originally a
connector, however it's footprint was wrong so wires were directly soldered to
the VIAs.  The board also utilizes two power planes (ground and 3V), making
it a 4 layer board.

Skills
---------------------------------

- Digital Circuit Design
	While simple, this project gave me an introduction to digital circuit
	design.  All my digital traces worked perfectly as intended.

- PCB design
	Outside of mounting holes, this is a relatively robust PCB.  I am more
	than capable of making it significantly smaller, but took advantage
	of the extra space requirements to improve manufacturability.

- Analog Circuit Design
	This feature did feature one analog component, the LED.  While simple,
	it was consistent and error-free

- Soldering
	Not only did I get to use classic soldering using an iron, I also got
	exposure to heat gun soldering and solder paste.  While I lost many
	accelerometers to this process, I am confident in my ability to 
	manufacture QFN packages in the future with use of reflow diagrams.
	I am a longtime solderer, and this was just another opportunity to
	work with it.
