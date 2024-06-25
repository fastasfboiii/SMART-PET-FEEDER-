#include <Arduino.h>
#include <ESP32Servo.h>  
#include <DHT.h>         
#include <Stepper.h>     

// Constants for the ultrasonic sensor pins
const int trigPin = 14; // Trig pin connected to GPIO 14
const int echoPin = 27; // Echo pin connected to GPIO 27


long duration;
int distance;

// Servo object
Servo myServo;
const int servoPin = 16; 


#define DHTPIN 4      // DHT11 sensorto GPIO 4
#define DHTTYPE DHT11 


DHT dht(DHTPIN, DHTTYPE);

const int waterLevelPin = 35; // Analog pin connected to water level sensor

const int irSensorPin = 17; // IR sensor pin connected to GPIO 17

const int motorIn1 = 26; 
const int motorIn2 = 15; 
const int motorEnA = 25; 

const int stepsPerRevolution = 2048; 
const int stepperPin1 = 18;          // Stepper pin 1 to GPIO 18
const int stepperPin2 = 19;          // Stepper pin 2 to GPIO 19
const int stepperPin3 = 21;          // Stepper pin 3 to GPIO 21
const int stepperPin4 = 22;          // Stepper pin 4 to GPIO 22

// Initialize the  stepper motor (agin sigh)
Stepper myStepper(stepsPerRevolution, stepperPin1, stepperPin3, stepperPin2, stepperPin4);

// okay ig
void moveMotorForward() {
  digitalWrite(motorIn1, HIGH);
  digitalWrite(motorIn2, LOW);
  analogWrite(motorEnA, 255); // Set motor speed 255 or whtever
}

// Function to stop the motor lol
void stopMotor() {
  digitalWrite(motorIn1, LOW);
  digitalWrite(motorIn2, LOW);
}

void setup() {
  
  Serial.begin(19200);

 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

 
  myServo.attach(servoPin);
  myServo.write(90); //90 degrees, middle

  
  dht.begin();

  pinMode(irSensorPin, INPUT);

  pinMode(motorIn1, OUTPUT);
  pinMode(motorIn2, OUTPUT);
  pinMode(motorEnA, OUTPUT);

  pinMode(stepperPin1, OUTPUT);
  pinMode(stepperPin2, OUTPUT);
  pinMode(stepperPin3, OUTPUT);
  pinMode(stepperPin4, OUTPUT);

  
  myStepper.setSpeed(10); // Set the speed to 10 RPM

  // Print a start message ig
  Serial.println("Ultrasonic sensor, servo, DHT11 sensor, water level sensor, IR sensor, and motor test begins");
}

void loop() {
  // Ultrasonic sensor code
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= 10) {
    // Move the stepper motor to open the gate
    myStepper.step(stepsPerRevolution / 4); 
    Serial.println("Object detected: Stepper motor moved to open the gate");

    // Move the servo 
    for (int pos = 90; pos <= 180; pos += 1) {
      myServo.write(pos);
      delay(15);
    }
    for (int pos = 180; pos >= 0; pos -= 1) {
      myServo.write(pos);
      delay(15);
    }
    for (int pos = 0; pos <= 90; pos += 1) {
      myServo.write(pos);
      delay(15);
    }

    // Move the motor forward for 5 seconds
    moveMotorForward();
    delay(5000); 
    stopMotor();
    Serial.println("Motor stopped after 5 seconds");
  } else {
    stopMotor();
    Serial.println("Object not detected: Motor stopped");
  }

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");
  }

  int waterLevelValue = analogRead(waterLevelPin);
  float waterLevel = map(waterLevelValue, 0, 4095, 0, 100); // Map analog value to percentage

  Serial.print("Water Level: ");
  Serial.print(waterLevel);
  Serial.println(" %");

  // IR sensor code
  int irValue = digitalRead(irSensorPin);

  if (irValue == LOW) { 
    Serial.println("There is food in the bowl");
  } else {
    Serial.println("No food in the bowl");
  }


  delay(2000);
}
