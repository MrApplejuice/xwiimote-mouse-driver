# Socket protocol

The mouse driver exposes a linux socket to allow mouse movements and buttons to 
be configured and calibrated. It also allows a listening client to receive
telemetry about wiimote data used to track the mouse driver.

## General message format

All messages from driver to client or vice versa have the following format:

- Maximum message length is 1024
- The content is a readable ASCII string, terminated by a single new-line '\n'
  character
- The general layout of messages is a list of `:` seperated strings. A `:` will
  not be included in any other places, the `:` is reserved as a list-separator.
- There is always one string in a valid message, namely the first designating
  the event type. Example: `message[:value][:value]...`. `message` is the event
  type and all subsequent `:`-separated values are message-specific parameters.
- Requests sent by a client to the server are always acknowledged either by:
    - A (literal) `OK[:parameter][:parameter]...` answer, possibly including
      parameter relating to the client inquery.
    - An error message of the form `ERROR[:message]`. The `:message` part is 
      a readable errors message and is optional.

# Messages

List of all supported messages and their parameters. Each message
is prefixed by either `SERVER` or `CLIENT` designating the side which is allowed
to generate the message.

## `SERVER ir`

`ir` messages communicate the positions of the four IR dots the wiimote can 
detect and track. These messages are continously sent by the server to connected
client in bursts of 4.

`ir:[index]:[valid]:[x]:[y]`

- `index`: The index of the tracked ir dot as reported by the wiimote (0-3)
- `valid`: 0 if the point is invalid (no ir signal detected). 1 if there is a
  valid ir signal.
- `x`, `y`: Only valid for the given entry if `valid==1`. If valid, these are
  the x and y coordinates of the point detected, the values are undefined if
  `valid==0`. Valid x coordinates seem to fall in the range of `[1-1022]` and
  y coordinates `[1-768]`

## `SERVER lr`

`lr` messages contain the _filtered_ left-right coordinates that are used by
the mouse driver to generate the mouse location. There are several filter steps
implemented that compute these coordinates. The points are meant to represent
the coordinates of a detected wii sensor bar in wiimote coordinates.

`lr:invalid` or `lr:[lx]:[ly]:[rx]:[ry]`

The message `lr:invalid` means that the left-right coordinates could not be
computed, usually because not enough ir-points were visible.

`lr:[lx]:[ly]:[rx]:[ry]` communicates the left-right coordinates of the two
sides of a detected wii sensor bar in wiimote relative coordinates. Depending
on the filter steps used, these coordinates can _exceed_ the valid ranges of the
ir-coordinates. Also, it is possible that the lx,ly and rx,ry pairs are equal
to each other, most typically when only one dot was detected by the wiimote.
The pair `lx`, `ly` and `rx`, `ry` designate the x/y coordinates of the left and
right side of the detected wii sensor bar, respectively.

## `SERVER b`

## `SERVER OK`

## `SERVER ERROR`

## `CLIENT mouse`

## `CLIENT cal100`

## `CLIENT getscreenarea100`

## `CLIENT screenarea100`

