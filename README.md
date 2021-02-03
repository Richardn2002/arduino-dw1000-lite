<div align="center">

### Arduino library for starting with Decawave's DWM1000 modules easily and fast

</div>

Who would need this library?
---
If you just happened to come across UWB technology moments ago on the Internet and want to get hands on it/

If you are working on a high school project that requires the use of UWB (most likely for ranging),

then you have come to the right place.

Usage
---
For extreme simplicity this repo isn't even an proper Arduino Library. You don't have to 'Add .ZIP Library'. Just structure your projects as those in the 'examples' folder and you are ready for compile and uploading.

Features
---
Multiple Modules Support on One Arduino

Module Initialization (to the default configuration)

Module Hardware Reset

Read/Write Registers

Delayed/Immediate Message Transmission/Reception

Read Transmission/Reception Status

(Actually the library offers more than these (like read accumulator memory)... but as I am not in the mood of writing a detailed doc right now I'm just listing some straight forward functionalities here.)

API Doc
---
Coming s∞n.

Technical Details
---
This library initializes all modules to run under the configuration of 6.8Mbps, 16 MHz PRF, 128 preamble length, 8 PAC, preamble code 4, channel 5, line-of-sight(los) optimization and does not offer functions to reconfig.

More advanced functions like double buffering, frame filtering, power management... are also not implemented.

As I said, this library is for high-school-ish projects. If you are working on an industrial project please read the DW1000 user manual and write your own library.

References
---
During writing, [arduino-dw1000-ng](https://github.com/F-Army/arduino-dw1000-ng) was heavily referenced (but this repo is nothing of a fork of it). Thanks to [F-Army](https://github.com/F-Army) and [thotro](https://github.com/thotro).

And of course, [DW1000 User Manual](https://www.decawave.com/wp-content/uploads/2019/07/DW1000-User-Manual-1.pdf). As long as you have a month before your project deadline you shall read it :D.

License
---
This project is under MIT

Copyright 2021 Richardn

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
