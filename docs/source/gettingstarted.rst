Setup
=====

This section describes the required steps to setup the wiimote 
mouse driver and have the mouse react to WiiMote inputs.

Install ``hid-wiimote`` kernel driver
-------------------------------------

The first step is to install the software that is *not included* in this 
project but is required for this project to run. The first item is the
kernel driver `hid-wiimote`. On Ubuntu, this driver is provided by the 
package `linux-modules-extra*`. Check your own distro to find details
on how to install this driver if it is not already present.

Install xwiimote-mouse-driver
-----------------------------

Download the binaries for Linux on your system. For the python-based
you will need to have python3 installed and the following python packages:

- tkinter
- numpy

On Ubuntu you can achieve this by issueing:

`sudo apt-get install python3 python3-tk python3-numpy`

Pair the WiiMote with your computer
-----------------------------------

By far the trickiest thing to get right in my experience. Here what
worked for me. Most of it consisted of doing the following steps
quickly(!):

1. Open your bluetooth-panel and get ready to press "scan for new devices"

.. image:: _static/1-bluetooth.jpg
  :height: 20em

2. Open the battery tray of the WiiMote and press the red pair-button.

.. image:: _static/2-bluetooth.jpg
  :height: 20em

3. Quickly hit the "search for new devices" button from (1).
4. Select "Pair" from the bluetooth menu and start mashing "A" on the WiiMote:

.. image:: _static/3-bluetooth.jpg
  :height: 20em

5. You are not done yet! When the "Connected"-message appears stop mashing
  the A button. Wait for a "Disconnected" message to appear in the bluetooth
  notification messages - about a second after the "Connected" message
  appeared.

6. Now right-click on the WiiMote device in the bluetooth manager and
  click "Connect". Also make sure that the "Audio and input profiles"
  checkmark is checked, otherwise the WiiMote will not work!
  
7. A "Connected" message should appear in the bluetooth notifications 
  after this and the WiiMote should remain connected. A single
  blue indicator light, replacing the flashing lights on the WiiMote
  indicates that the pairing suceeded:

.. image:: _static/4-bluetooth.jpg
  :height: 20em

8. Suggestion: Also press the "Trust" button (see image 4) so that
  the WiiMote quickconnect feature works!

Run the WiiMote mouse driver
----------------------------

Next steps
----------
