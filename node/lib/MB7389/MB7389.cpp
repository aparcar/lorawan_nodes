#include <softSerial.h>

uint32_t get_distance(softSerial softwareSerial) {
  boolean foundValue;
  boolean foundStart;
  String inString;
  uint32_t measurements = 0;
  uint32_t total = 0;
  uint32_t start;

  softwareSerial.begin(9600);
  while (!softwareSerial.available()) {
  }

  while (measurements < 60) {
    start = millis();
    foundStart = false;
    foundValue = false;
    softwareSerial.flush();
    inString = "";
    while (!foundValue) {
      if (softwareSerial.available()) {
        char inChar = softwareSerial.read();
        if (foundStart == false) {
          if (inChar == 'R') {
            foundStart = true;
          } else {
            softwareSerial.read();
          }
        } else {
          if (inChar != '\r') {
            inString += inChar;
          } else {
            if (inString.length() != 4) {
              Serial.println("incomplete measurement");
              foundStart = false;
              inString = "";
            } else {
              foundValue = true;
              total += inString.toInt();
              measurements += 1;
              delay(1000 - (millis() - start));
            }
          }
        }
      } else {
        delay(100);
      }
    }
    Serial.printf(".");
  }
  Serial.printf("average distance is %d\n", total / 600);
  return (total / 600); // to cm
}
