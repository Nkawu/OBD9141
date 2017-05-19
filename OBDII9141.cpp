/*
 *  Copyright (c) 2015, Ivor Wanders
 *  MIT License, see the LICENSE.md file in the root folder.
*/
#include "OBDII9141.h"

OBDII9141::OBDII9141(){};

void OBDII9141::begin(HardwareSerial & serial_port, uint8_t rx_pin, uint8_t tx_pin){
    this->serial = &serial_port;
    this->tx_pin = tx_pin;
    this->rx_pin = rx_pin;

    // Enable the pullup on the Rx Pin, this is not changed by set_port.
    pinMode(this->rx_pin, INPUT);
    digitalWrite(this->rx_pin, HIGH);
    this->set_port(true); // prevents calling this->serial->end() before start.
}

void OBDII9141::set_port(bool enabled){
    if (enabled){
        // Work around the incorrect pinmode configuration in Due.
        this->serial->begin(OBDII9141_KLINE_BAUD);
    } else {
        this->serial->end();
        pinMode(this->tx_pin, OUTPUT);
        digitalWrite(this->tx_pin, HIGH);
    }
}

void OBDII9141::kline(bool enabled){
    digitalWrite(this->tx_pin, enabled);
}

uint8_t OBDII9141::checksum(void* b, uint8_t len){
    uint8_t ret = 0;
    for (uint8_t i=0; i < len; i++){
        ret += reinterpret_cast<uint8_t*>(b)[i];
    }
    return ret;
}

void OBDII9141::write(uint8_t b){
    this->serial->write(b);
    
    this->serial->setTimeout(OBDII9141_REQUEST_ECHO_MS_PER_BYTE * 1 + OBDII9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[1]; // temporary variable to read into.
    this->serial->readBytes(tmp, 1);
}

void OBDII9141::write(void* b, uint8_t len){
    for (uint8_t i=0; i < len ; i++){
        this->serial->write(reinterpret_cast<uint8_t*>(b)[i]);
        delay(OBDII9141_INTERSYMBOL_WAIT);
    }
    this->serial->setTimeout(OBDII9141_REQUEST_ECHO_MS_PER_BYTE * len + OBDII9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[len]; // temporary variable to read into.
    this->serial->readBytes(tmp, len);
}

bool OBDII9141::request(void* request, uint8_t request_len, uint8_t ret_len){
    uint8_t buf[request_len+1];
    memcpy(buf, request, request_len); // copy request

    buf[request_len] = this->checksum(&buf, request_len); // add the checksum

    this->write(&buf, request_len+1);

    // wait after the request, officially 30 ms, but we might as well wait
    // for the data in the readBytes function.
    
    // set proper timeout
    this->serial->setTimeout(OBDII9141_REQUEST_ANSWER_MS_PER_BYTE * ret_len + OBDII9141_WAIT_FOR_REQUEST_ANSWER_TIMEOUT);
    memset(this->buffer, 0, ret_len+1);
    
    //OBDII9141print("Trying to get x bytes: "); OBDII9141println(ret_len+1);
    if (this->serial->readBytes(this->buffer, ret_len+1)){
        return (this->checksum(&(this->buffer[0]), ret_len) == this->buffer[ret_len]);// have data; return whether it is valid.
    } else {
        // OBDII9141println("Timeout on reading bytes.");
        return false; // failed getting data.
    }
}

/*
    No header description to be found on the internet?

    raw request: {0x68, 0x6A, 0xF1, 0x01, 0x0D}
        returns:  0x48  0x6B  0x11  0x41  0x0D  0x00  0x12 
        returns 1 byte
        total of 7 bytes.

    raw request: {0x68, 0x6A, 0xF1, 0x01, 0x00}
        returns   0x48  0x6B  0x11  0x41  0x00  0xBE  0x3E  0xB8  0x11  0xCA
        returns 3 bytes
        total of 10 bytes

    Mode seems to be 0x40 + mode, unable to confirm this though.

*/


bool OBDII9141::getPID(uint8_t pid, uint8_t mode, uint8_t return_length){
    uint8_t message[5] = {0x68, 0x6A, 0xF1, mode, pid};
    // header of request is 5 long, first three are always constant.

    bool res = this->request(&message, 5, return_length+5);
    // checksum is already checked, verify the PID.

    if (this->buffer[4] != pid){
        return false;
    }
    return res;
}

bool OBDII9141::getCurrentPID(uint8_t pid, uint8_t return_length){
    return this->getPID(pid, 0x01, return_length);
}

uint8_t OBDII9141::readUint8(){
    return this->buffer[5];
}

uint16_t OBDII9141::readUint16(){
    return this->buffer[5]*256 + this->buffer[6]; // need to reverse endianness
}

uint8_t OBDII9141::readUint8(uint8_t index){
    return this->buffer[5 + index];
}

bool OBDII9141::init(){
    // this function performs the ISO9141 5-baud 'slow' init.
    this->set_port(false); // disable the port.
    this->kline(true);
    delay(OBDII9141_INIT_IDLE_BUS_BEFORE); // no traffic on bus for 3 seconds.
    // OBDII9141println("Before magic 5 baud.");
    // next, send the startup 5 baud init..
    this->kline(false); delay(200); // start
    this->kline(true); delay(400);  // first two bits
    this->kline(false); delay(400); // second pair
    this->kline(true); delay(400);  // third pair
    this->kline(false); delay(400); // last pair
    this->kline(true); delay(200);  // stop bit
    // this last 200 ms delay could also be put in the setTimeout below.
    // But the spec says we have a stop bit.

    // done, from now on it the bus can be treated ad a 10400 baud serial port.

    // OBDII9141println("Before setting port.");
    this->set_port(true);
    // OBDII9141println("After setting port.");
    uint8_t buffer[1];

    this->serial->setTimeout(300+200);
    // wait should be between 20 and 300 ms long

    // read first value into buffer, should be 0x55
    if (this->serial->readBytes(buffer, 1)){
        // OBDII9141print("First read is: "); OBDII9141println(buffer[0]);
        if (buffer[0] != 0x55){
            return false;
        }
    } else {
        // OBDII9141println("Timeout on read 0x55.");
        return false;
    }
    // we get here after we have passed receiving the first 0x55 from ecu.

    this->serial->setTimeout(20); // w2 and w3 are pauses between 5 and 20 ms

    uint8_t v1=0, v2=0; // sent by car:  (either 0x08 or 0x94)

    // read v1
    if (!this->serial->readBytes(buffer, 1)){
        // OBDII9141println("Timeout on read v1.");
        return false;
    } else {
        v1 = buffer[0];
        // OBDII9141print("read v1: "); OBDII9141println(v1);
    }

    // read v2
    if (!this->serial->readBytes(buffer, 1)){
        // OBDII9141println("Timeout on read v2.");
        return false;
    } else {
        v2 = buffer[0];
        // OBDII9141print("read v2: "); OBDII9141println(v2);
    }
    
    // OBDII9141print("v1: "); OBDII9141println(v1);
    // OBDII9141print("v2: "); OBDII9141println(v2);

    // these two should be identical according to the spec.
    if (v1 != v2){
        return false;
    }

    // we obtained w1 and w2, now invert and send it back.
    // tester waits w4 between 25 and 50 ms:
    delay(30);
    this->write(~v2);
    this->serial->setTimeout(50); // w5 is same as w4...  max 50 ms

    // finally, attempt to read 0xCC from the ECU, indicating succesful init.
    if (!this->serial->readBytes(buffer, 1)){
        // OBDII9141println("Timeout on 0xCC read.");
        return false;
    } else {
        // OBDII9141print("read 0xCC?: "); OBDII9141println(buffer[0], HEX);
        if ((buffer[0] == 0xCC)){ // done if this is inverse of 0x33
            delay(OBDII9141_INIT_POST_INIT_DELAY);
            // this delay is not in the spec, but prevents requests immediately
            // after the finishing of the init sequency.

            return true; // yay! we are initialised.
        } else {
            return false;
        }
    }
}

