/*
 *  Copyright (c) 2015, Ivor Wanders
 *  MIT License, see the LICENSE.md file in the root folder.
*/

#ifndef OBDII9141_H
#define OBDII9141_H
#include "Arduino.h"

#define OBDII9141_KLINE_BAUD 10400 
// as per spec.

#define OBDII9141_BUFFER_SIZE 16
// maximum possible as per protocol is 256 payload, the buffer also contains
// request and checksum, add 5 + 1 for those on top of the max desired length.
// User needs to guarantee that the ret_len never exceeds the buffer size.

#define OBDII9141_INTERSYMBOL_WAIT 5
// Milliseconds delay between writing of subsequent bytes on the bus.
// Is 5ms according to the specification.

// When data is sent over the serial port to the K-line transceiver, an echo of
// this data is heard on the Rx pin; this determines the timeout of readBytes
// used after sending data and trying to read the same number of bytes from the
// serial port to discard the echo.
// The total timeout for reading the echo is:
// (OBDII9141_REQUEST_ECHO_TIMEOUT_MS * sent_len + 
//                      OBDII9141_WAIT_FOR_ECHO_TIMEOUT) milliseconds.
#define OBDII9141_WAIT_FOR_ECHO_TIMEOUT 5
// Time added to the timeout to read the echo.
#define OBDII9141_REQUEST_ECHO_MS_PER_BYTE 3
// Time added per byte sent to wait for the echo.

// When a request is to be made, request bytes are pushed on the bus with
// OBDII9141_INTERSYMBOL_WAIT delay between them. After the transmission of the
// request has been completed, there is a delay of OBDII9141_AFTER_REQUEST_DELAY
// milliseconds. Then the readBytes method is used  with a timeout of
// (OBDII9141_REQUEST_ANSWER_MS_PER_BYTE * ret_len + 
//                      OBDII9141_WAIT_FOR_REQUEST_ANSWER_TIMEOUT) milliseconds.

#define OBDII9141_REQUEST_ANSWER_MS_PER_BYTE 3
// The ECU might not push all bytes on the bus immediately, but wait several ms
// between the bytes, this is the time allowed per byte for the answer

#define OBDII9141_WAIT_FOR_REQUEST_ANSWER_TIMEOUT (30 + 10)
// Time added to the read timeout when reading the response to a request. 
// This should incorporate the 30 ms that's between the request and answer
// according to the specification.

#define OBDII9141_INIT_IDLE_BUS_BEFORE 3000
// Before the init sequence; the bus is kept idle for this duration in ms.

#define OBDII9141_INIT_POST_INIT_DELAY 50
// This is a delay after the initialisation has been completed successfully.
// It is not present in the spec, but prevents a request immediately after the
// init has succeeded when the other side might not yet be ready.

class OBDII9141{
    private:
        HardwareSerial* serial;

        void kline(bool); // sets the state of the Tx pin.
        uint8_t tx_pin;
        uint8_t rx_pin;

        void write(uint8_t b);
        // writes a byte and removes the echo.

        void write(void* b, uint8_t len);
        // writes an array and removes the echo.

        uint8_t buffer[OBDII9141_BUFFER_SIZE]; // internal buffer.


    public:
        OBDII9141();

        void begin(HardwareSerial & serial_port, uint8_t rx_pin, uint8_t tx_pin);
        // begin method which allows setting the serial port and pins.

        bool getCurrentPID(uint8_t pid, uint8_t return_length);
        // The standard PID request on the current state, this is what you
        // probably want to use.
        // actually calls: getPID(pid, 0x01, return_length)

        bool getPID(uint8_t pid, uint8_t mode, uint8_t return_length);
        // Sends a request containing {0x68, 0x6A, 0xF1, mode, pid}
        // Returns whether the request was answered with a correct answer
        // (correct PID and checksum)
        
        bool request(void* request, uint8_t request_len, uint8_t ret_len);
        // Sends buffer at request, up to request_len, adds a checksum.
        // Needs to know the returned number of bytes, checks if the appropiate
        // length was returned and if the checksum matches.
        // User needs to ensure that the ret_len never exceeds the buffer size.

        uint8_t readUint8(); // returns right part from the buffer as uint8_t
        uint16_t readUint16(); // idem...
        uint8_t readUint8(uint8_t index); // returns byte on index.

        void set_port(bool enabled);
        // need to disable the port before init.
        // need to enable the port if we want to skip the init.

        bool init(); // attempts 'slow' ISO9141 5 baud init.
        // returns whether the procedure was finished correctly.
        // The class keeps no track of whether this was successful or not.
        // It is up to the user to ensure that the initialisation is called.

        static uint8_t checksum(void* b, uint8_t len); // public for sim.
};

#endif
