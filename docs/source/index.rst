.. Xwiimote Mouse Driver documentation master file, created by
    sphinx-quickstart on Sat Jan 20 15:42:51 2024.
    You can adapt this file completely to your liking, but it should at least
    contain the root `toctree` directive.

Xwiimote Mouse Driver
=====================

A mouse driver for WiiMote remote controls based on the `xwiimote kernel
driver for Linux <https://github.com/xwiimote/xwiimote>`_.


What is this project meant for?
-------------------------------

This project is meant for people who want to control a Linux-based multimedia 
station like `Kodi <https://kodi.tv/>`_ or 
`Steam Big Picture mode <https://store.steampowered.com/bigpicture>`_
using a WiiMote.

Features
--------

- Use the WiiMote as a pointing device on any Linux GUI
- The driver contains a configuration utility for:
  - Calibrating the WiiMote sensor to match your screen setup
  - Allow to map keys to arbitrary mouse/keyboard/gamepad buttons

What do you need to run the Xwiimote Mouse Driver?
--------------------------------------------------

.. image:: _static/wiimote.jpg
  :height: 15em
  :align: center


1. A WiiMote; the thing shown in the image above.

2. A Linux computer with a compatible bluetooth device and the 
   `hid-wiimote` kernel driver running. I have no clue which bluetooth
   devices are compatbile and which are not, but I have seen mainly older
   bluetooth dongles not being able to detect the WiiMote. 

3. A standalone Wii sensor bar. This is one that works without a Wii console.
   There are USB-powered or battery powered sensor bars out there hat one can
   buy. `Details about the sensor bar on wiibrew.org <https://wiibrew.org/wiki/Sensor_Bar>`_.

4. Root access to the machine you want to use the driver on (in most cases).

Ready to start?
---------------

.. toctree::
  :maxdepth: 2

  gettingstarted.rst
  driverconfig.rst

Why does this project exist? 
----------------------------

There are two different wiimote drivers for Linux out there that I know about:

- `xorg-input-xwiimote <https://manpages.ubuntu.com/manpages/xenial/man4/xorg-xwiimote.4.html>`_ 

  This driver works fine, but only for X11. It is an X11 mouse driver afterall.
  For newer operating systems shipping with Wayland, this is a deal-breaker and
  the driver does not work.

- `wminput of libcwiid <https://github.com/abstrakraft/cwiid>`_

  libcwiid seems to the the original user-space driver to get the wiimote to 
  work. Though the code is old, and the wminput-driver does not work well at 
  all.

I found the implementation of `xorg-input-xwiimote` good, but not good enough.
There was no easily configurable pointer calibarion for the driver so finding
the exact right relation between screen coordinates and the sensor bar was 
a hassle. The keymap was also hardcoded, there was no horizontal detection in 
the driver, the driver relied on having a single IR-LED opposed to a standard
wiimote sensor bar, and, of course, it was X11 based, and I wanted something 
that *also* worked on wayland.

So this project takes all the good ideas fron `xorg-input-xwiimote` but 
interfaces with `libevdev` instead of X11. `libevdev` is the input library for
`libinput` which works for both X11 and Wayland. In addition, the project 
contains a configuration utility that allows configuring the xwiimote driver
without needing to restart the driver.


Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
