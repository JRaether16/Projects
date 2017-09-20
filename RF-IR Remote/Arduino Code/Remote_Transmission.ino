#include <RH_ASK.h>
#include <SPI.h>
#define RH_ASK_ARDUINO_USE_TIMER2

const char *INPUT1 = "IN1x"; //The x's have a purpose I promise, but these are the strings for each message
const char *INPUT2 = "IN2x";
const char *INPUT3 = "IN3x";
const char *INPUT4 = "IN4x";
const char *PIP = "PIPx";
const char *BACK = "RETx";
const char *SELECT = "SELx";
const char *ENTER = "ENTx";

char *decideButton(int voltage);
void transmitMessage(char *transmission);

RH_ASK driver;

void setup() {
  
  Serial.begin(9600);

  
  if (!driver.init()) {
    Serial.println("Initialization failed.");
  }
  
  delay(4000); //Let driver initialize
  
  Serial.println("Initialization success");
}

void loop() {

  int voltageLevel; //Variable to store voltage level, 0-1024

  voltageLevel = analogRead(A1); //Read voltage level from a button press

  if(voltageLevel > 10){
    delay(50);
  }

  voltageLevel = analogRead(A1); //Read again, since voltage isn't instantaneous, just to double check

 // Serial.println(voltageLevel);

 char *msg = decideButton(voltageLevel); //If we had a button press, assign its correct character

  if (!msg) { //Block if no button is pressed
  }
  else {
    transmitMessage(msg); //If button pressed, send a message
  }

  while(voltageLevel > 10){ //If some idiot is holding down the button, run this until he/she let's it go
    voltageLevel = analogRead(A1);
  }
}

char *decideButton(int voltage) {

  if (voltage >= 910 && voltage <= 950) { //Voltage level for input 1 conditions based on resistor placed, and so on for other buttons...
    return INPUT1;
  }
  else if (voltage >= 820 && voltage <= 860) {
    return INPUT2;
  }
  else if (voltage >= 745 && voltage <= 785) {
    return INPUT3;
  }
  else if (voltage >= 675 && voltage <= 715) {
    return INPUT4;
  }
  else if (voltage >= 590 && voltage <= 630) {
    return PIP;
  }
  else if (voltage >= 300 && voltage <= 340) {
    return BACK;
  }
  else if (voltage >= 215 && voltage <= 255) {
    return SELECT;
  }
  else if (voltage >= 160 && voltage <= 200) {
    return ENTER;
  }
  else {
    return 0;
  }



}

void transmitMessage(char *transmission) {

if(transmission){
driver.send((uint8_t *)transmission, strlen(transmission)); //Send transmission as a byte packet
driver.waitPacketSent(); //Wait for packet to be sent
delay(100); //Wait .1 seconds just to make sure everything is okay
Serial.print(transmission);
Serial.println(" sent.");
}

}

