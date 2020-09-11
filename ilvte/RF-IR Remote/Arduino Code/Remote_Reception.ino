#include <RH_ASK.h>
#include <SPI.h>
#include <IRLibDecodeBase.h>  //We need both the coding and
#include <IRLibSendBase.h>    // sending base classes
#include <IRLib_P01_NEC.h>    //Lowest numbered protocol 1st
#include <IRLib_P02_Sony.h>   // Include only protocols you want
#include <IRLib_P03_RC5.h>
#include <IRLib_P04_RC6.h>
#include <IRLib_P05_Panasonic_Old.h>
#include <IRLib_P07_NECx.h>
#include <IRLib_HashRaw.h>    //We need this for IRsendRaw
#include <IRLibCombo.h>
#include <PWM.h>

#define RH_ASK_ARDUINO_USE_TIMER2

const char *INPUT1 = "IN1\0"; //these are the strings for each message
const char *INPUT2 = "IN2\0";
const char *INPUT3 = "IN3\0";
const char *INPUT4 = "IN4\0";
const char *PIP = "PIP\0";
const char *BACK = "RET\0";
const char *SELECT = "SEL\0";
const char *ENTER = "ENT\0";

int32_t frequency = 25000;
int driverPin = 9;

void checkRadio(char *reception);

RH_ASK driver; //RH receiver driver
IRsend mySender; //Ir sender driver

void setup() {
  Serial.begin(9600);
  if (!driver.init()) Serial.println("RH initialization failed.");
  delay(4000);
  Serial.println("Ready to receive Radio message");
  pinMode(5, OUTPUT);
}

void loop() {

  uint8_t buf[4]; //buffer of size four
  uint8_t buflen = sizeof(buf);
  //Serial.println("LOOPING");
  if (driver.recv(buf, &buflen)) {
    Serial.println("Entered if statement...");
    buf[3] = 0;
    char *received = (char*)buf; //make entire buffer into a string again
    Serial.println(received);
    checkRadio(received);
    if(!driver.init()){
      Serial.println("Reinit failed");
    }
  }
analogWrite(6, 255);
if(Serial.available()){
  int val = 0;
  val = Serial.read();
  analogWrite(5, val);
  Serial.flush();
}

}



void checkRadio(char *reception) {
  Serial.println(reception);
  if (strcmp(reception, INPUT1) == 0) {
    Serial.println("Sending message...");
    mySender.send(NEC, 0x1FE48B7, 0);
    Serial.println("Message sent.");
  }
  else if (strcmp(reception, INPUT2) == 0) {
    mySender.send(NEC, 0x1FE58A7, 0);
  }
  else if (strcmp(reception, INPUT3) == 0) {
    mySender.send(NEC , 0x1FE7887, 0);
  }
  else if (strcmp(reception, INPUT4) == 0) {
    mySender.send(NEC, 0x1FE807F, 0);
  }
  else if (strcmp(reception, PIP) == 0) {
    mySender.send(NEC, 0x1FE50AF, 0);
  }
  else if (strcmp(reception, BACK) == 0) {
    mySender.send(NEC, 0x1FEC03F, 0);
  }
  else if (strcmp(reception, SELECT) == 0) {
    mySender.send(NEC, 0x1FED827, 0);
  }
  else if (strcmp(reception, ENTER) == 0) {
    mySender.send(NEC, 0x1FEF807, 0);
  }
  else {
    Serial.println("No valid message found");
  }
}
