/*
   LoRa Receiver with RSSI RGB ( catode ) signals.
   This example is made for 3v3 Arduino.

   LoRa library: https://github.com/sandeepmistry/arduino-LoRa

   DaveCalaway - https://github.com/DaveCalaway
*/
#include <SPI.h>
#include <LoRa.h>

// RGB leds pins
byte red = 5;
byte green = 4;
byte blue = 3;

// The frequency depends on the board you have: 433E6, 866E6, 915E6
long int frequency = 915E6;

// Time between 2 signals (seconds)
int t_between = 5;

unsigned long previousMillis = 0;

void setup() {
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver");
  // 3v3 Arduinos works at 8Hmz for the SPI
  LoRa.setSPIFrequency(8E6);

  if (!LoRa.begin(frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    int rssi = LoRa.packetRssi();
    if (rssi > -70) {
      digitalWrite(red, 1);
      digitalWrite(green, 0);
      Serial.print("green");
      digitalWrite(blue, 1);
    }
    else if (rssi <= -70 && rssi >= -85) {
      digitalWrite(red, 1);
      Serial.print("blue");
      digitalWrite(green, 1);
      digitalWrite(blue, 0);
    }
    else if (rssi <= -85 && rssi >= -100) {
      digitalWrite(red, 0);
      Serial.print("yellow");
      digitalWrite(green, 0);
      digitalWrite(blue, 1);
    }
    else { // rssi < -100
      digitalWrite(red, 0);
      Serial.print("red");
      digitalWrite(green, 1);
      digitalWrite(blue, 1);
    }
    Serial.println(rssi);
    previousMillis = millis();
  }

  else {
    if (millis() - previousMillis > ((t_between*1000)+1000)) {
      digitalWrite(red, 0);
      Serial.println("no message");
      digitalWrite(green, 1);
      digitalWrite(blue, 1);
      previousMillis = millis();
    }
  }
}
