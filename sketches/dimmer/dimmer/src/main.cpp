#include <Arduino.h>

uint16_t analogVal;
uint8_t pwmVal = 0;

void setup() {
  Serial.begin(115200);

}

void loop() {
analogVal = analogRead(1);
delay(2);
pwmVal = map(analogVal,0,4095,0,255);
Serial.printf("%d\tPWM: %d\n",analogVal, pwmVal);
delay(10);
analogWrite(10,pwmVal);
//analogVal = map(analogVal,0,4095,0,255);
//neopixelWrite(21,(int)analogVal,(int)analogVal,(int)analogVal);
delay(10);
}

