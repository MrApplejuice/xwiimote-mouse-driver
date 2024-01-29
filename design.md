# Plan

1. Salvage code from the x-input driver as much as possible to get started quickly
  with the hid-wii driver.
2. libevdev!!! is the foot in the door it seems to inject a virtual mouse!
    Example: https://suricrasia.online/blog/turning-a-keyboard-into/


Nope: libinput is a no-go! It is only a translation layer for HID->userspace...
one would need to hack the library itself!
2. Use `libinput` to inject the cursor events as mouse events

## Notes 2024-01-24

Acceleration vector of the wiimote:

X-axis=right positive
Y-axis=front positive
Z-axis=botton positive

## Predictive correction

Possiblities:

- 2 points detected, no correction needed
- One of the points is out-of-view
- Both points collapsed into one
- (point reflections visible - not handled right now)

Logic:

- Model individual point movements.
- When a cluster point disappears, start out with a high probability that they are
  still at the same location.
- Start tracking the smoothed output data.
  - If points collapse into one point, increase liklihood of collapse.
  - If points keep to be in one location, matching the last average movement
    vectors, assume out-of-view. Handle accordingly!
- When tracking normally, use a weighted average to deduce the distance the two
  points are apart.
