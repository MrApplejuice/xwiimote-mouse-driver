Setup
=====

This section describes the required steps to setup the wiimote 
mouse driver and have the mouse react to WiiMote inputs.

Install xwiimote and hid-wiimote kernel driver
----------------------------------------------

The first step is to install the software that is *not included* in this 
project but is required for this project to run. The first item is the
kernel driver `hid-wiimote`. On Ubuntu, this driver is provided by the 
package `linux-modules-extra*`. Check your own distro to find details
on how to install this driver if it is not already present.

Install and run the xwiimote-mouse-driver
-----------------------------------------

Download the binaries for Linux on your system. For the python-based
you will need to have python3 installed and the following python packages:

- tkinter
- numpy

On Ubuntu you can achieve this by issueing:

`sudo apt-get install python3 python3-tk python3-numpy`

Pair the WiiMote with your computer
-----------------------------------

By far the trickiest thing to get right. 

Run the WiiMote mouse driver
----------------------------

Next steps
----------
