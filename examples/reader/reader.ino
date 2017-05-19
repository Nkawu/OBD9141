#include "Arduino.h"
#include "OBDII9141.h"

#define RX_PIN 0
#define TX_PIN 1
#define EN_PIN 5

OBDII9141 obd;

void setup(){
    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, HIGH);

    obd.begin(Serial, RX_PIN, TX_PIN);
}
    
void loop(){
    bool init_success =  obd.init();

    if (init_success){
        bool res;
        while(1){
            res = obd.getCurrentPID(0x11, 1);
            if (res){
                // Do something
            }
            
            res = obd.getCurrentPID(0x0C, 2);
            if (res){
                // Do something
            }

            res = obd.getCurrentPID(0x0D, 1);
            if (res){
                // Do something
            }

            delay(200);
        }
    }
    delay(3000);
}
