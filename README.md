`debug'
=======

This is an Arduino library for managing output of debug information.
Tested on Arduino nano.

It implements:
  - A way to easily output file and line-of-file where the debug string comes
    from
  - An output with substitutions "a la printf"
  - A way to record finely-grained timing data with periodic output
  - `debug' will automatically save the debug strings in PROGMEM, relieving
    dynamic memory for that much data


Installation
------------

I guess you should download a zip of this repository, then include it from the
Arduino IDE.
But the author never tested it in this way.


Usage
-----

See RFLink library that contains debug instructions (not compiled by default).
RFLink library is available here:
[https://github.com/sebmillet/rflink](https://github.com/sebmillet/rflink)

