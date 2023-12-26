# Wiimote-mouse

This project implements a userspace mouse driver for Linux (root required!).
The general design is to interface with the kernel wiimote driver using
the library [xwiimote](https://github.com/xwiimote/xwiimote). It then does
matches wiimote motion to screen coordinates and forwards the resulting events
to [libevdev](https://www.freedesktop.org/wiki/Software/libevdev/) to
create and control a virtual mouse.

Special thanks to [Suricrasia Online](https://suricrasia.online/) for providing 
this excellent blog post on how to get started with libevdev virtual mice:
 - https://suricrasia.online/blog/turning-a-keyboard-into/