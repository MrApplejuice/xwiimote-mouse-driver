# Plan

1. Salvage code from the x-input driver as much as possible to get started quickly
  with the hid-wii driver.
2. libevdev!!! is the foot in the door it seems to inject a virtual mouse!
    Example: https://suricrasia.online/blog/turning-a-keyboard-into/


Nope: libinput is a no-go! It is only a translation layer for HID->userspace...
one would need to hack the library itself!
2. Use `libinput` to inject the cursor events as mouse events