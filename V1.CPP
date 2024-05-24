#include <NewPing.h>

const int trigPin = 5;      // GPIO5 (D1 on ESP8266)
const int echoPin = 4;      // GPIO4 (D2 on ESP8266)
const int ledPin = 2;       // GPIO2 (D4 on ESP8266)

NewPing sonar(trigPin, echoPin, 400); // NewPing setup of pins and maximum distance.

bool ledOn = false; // Flag to track the LED state

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  unsigned int uS = sonar.ping(); // Send ping, get ping time in microseconds (uS).
  unsigned int distance = uS / US_ROUNDTRIP_CM; // Convert ping time to distance in cm.

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance < 20 && !ledOn) {
    digitalWrite(ledPin, HIGH); // Turn on the LED
    ledOn = true;               // Update the flag
  } else if (distance >= 20 && ledOn) {
    digitalWrite(ledPin, LOW);  // Turn off the LED
    ledOn = false;              // Update the flag
  }

  delay(200); // Wait 200ms between readings (5 times per second)
}
