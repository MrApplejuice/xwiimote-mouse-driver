# xwiimote-mouse-driver

This project implements a userspace mouse driver for Linux (root required).
The general design is to interface with the kernel wiimote driver using
the library [xwiimote](https://github.com/xwiimote/xwiimote). It then does
matches wiimote motion to screen coordinates and forwards the resulting events
to [libevdev](https://www.freedesktop.org/wiki/Software/libevdev/) to
create and control a virtual mouse.

Special thanks to [Suricrasia Online](https://suricrasia.online/) for providing 
this excellent blog post on how to get started with libevdev virtual mice:

- https://suricrasia.online/blog/turning-a-keyboard-into/

# Documentation

The documentation can be found at https://mrapplejuice.github.io/xwiimote-mouse-driver/#

# License

[Full license text](LICENSE.md)

xwiimote-mouse-driver is a user-space mouse driver that allows using a wiimote
as a mouse on a desktop computer.

Copyright (C) 2024  Paul Konstantin Gerke

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Build requirements

- libevdev (version 1.12 or higher) development files 

    - See https://www.freedesktop.org/wiki/Software/libevdev/
    - License: MIT

- libxwiimote development files

    - See https://github.com/xwiimote/xwiimote
    - License: Custom XWiimote License  

- cmake
- pkgconfig

# More handy documents about libevdev and the wiimote interfaces

- debug logs: https://who-t.blogspot.com/2016/09/understanding-evdev.html
- kernel events: https://www.kernel.org/doc/html/v4.17/input/event-codes.html
- multitouch: https://www.kernel.org/doc/html/latest/input/multi-touch-protocol.html
- python demo project: https://github.com/alex-s-v/10moons-driver
- Someone wrote an audio driver in C#: https://github.com/trigger-segfault/WiimoteLib.Net/tree/master
