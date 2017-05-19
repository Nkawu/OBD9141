OBDII9141
======

This is a library to read data from the the ISO 9141-2 (K-line) pin of an [OBD-II][obd2] port found in cars using a MX33290 chip.

Usage
--------

The EN pin can either be connected to a pin on the microcontroller or just pulled high in order to always enable the chip.

Timing
-----

Several parameters related to timing are given by the specification, some others are beyond our influence. To understand how a request works, lets consider the case the `0x0D` PID is requested, this represents the vehicle's speed.
![Timing diagram of a request](/../master/extras/timing_diagram/timing_diagram.png?raw=true "Timing diagram of a request")

Requesting the value of a PID consists of two phases, the first is the request phase the second the response.

In the first phase, the bytes necessary for the request are written on the Tx line, according to the specification there should be a 5 millisecond delay between each symbol. The duration of this delay is defined by `INTERSYMBOL_WAIT`. It is possible that the ECU still discerns the bytes correctly when this delay is lowered.

The transceiver IC puts the waveform seen on the Tx line on the K-line. However, a echo of this waveform is also provided as output of the transceiver IC and is seen on the Rx of the serial port. Because this is not part of the response we have to deal with this echo. This is done by using the `readBytes` method to read the same number of bytes as were sent during the request. The timeout used for this read is given by (`REQUEST_ECHO_TIMEOUT_MS` * sent_len + `WAIT_FOR_ECHO_TIMEOUT`) milliseconds, where sent_len is the number of bytes sent in the request. Because the serial port used should contain a hardware buffer, very little time is spent in this `readBytes` call, as all the bytes from the echo should already be available and the function returns when the requested number of bytes has been read.

After the echo has been read, the waiting game begins as we wait for the ECU to answer. According to the specification this is atleast 30 milliseconds, this duration (which is the major part of a request duration) is something that cannot be influenced. The timeout set to read the response is given by (`REQUEST_ANSWER_MS_PER_BYTE` * ret_len +  `WAIT_FOR_REQUEST_ANSWER_TIMEOUT`) milliseconds, ret_len represents the number of bytes expected from the ECU. Because this number is known beforehand the `readBytes` method can be used. According to the specification the ECU should also pause between sending the bytes, but this is not necessarily the case.

The number of bytes to be received for each phase is known beforehand so the `readBytes` method can be used; it ensures that we stop reading immediately after the expected amount has been read. The main impact on the performance is given by the time the ECU takes to send a response and the `INTERSYMBOL_WAIT` between the bytes sent on the bus. There is no delay parameter for the minimum duration between two requests, that is up to the user.

License
------
MIT License, see LICENSE.md

Copyright (c) 2015 Ivor Wanders

[obd2]:https://en.wikipedia.org/wiki/On-board_diagnostics
[mc33290]:http://www.freescale.com/products/archived/iso9141-k-line-serial-link-interface:MC33290
