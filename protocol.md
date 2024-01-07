# Socket protocol

The mouse driver exposes a linux socket to allow mouse movements and buttons to 
be configured and calibrated.

## General message format

All messages from driver to client or vice versa have the following format:

- Maximum message length is 1024
- The content is a readable ASCII string, terminated by a single new-line '\n'
  character
- The general layout of messages is a list of `:` seperated strings. A `:` will
  not be included in any other places, the `:` is reserved as a list-separator.
- There is always one string in a valid message, namely the first designating
  the event type. Example: `ir:1:312:302`. `ir` is the event type and all
  subsequent `:`-separated values are message-specific paramters.