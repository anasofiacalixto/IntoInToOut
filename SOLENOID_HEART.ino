#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

const int ButtonPin = 10;
const int SolenoidPin = 12;

const byte RATE_SIZE = 4; // Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; // Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; // Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
long baseValue = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  pinMode(ButtonPin, INPUT);
  pinMode(SolenoidPin, OUTPUT);

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) // Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  // Setup to sense a nice-looking sawtooth on the plotter
  byte ledBrightness = 0x1F; // Options: 0=Off to 255=50mA
  byte sampleAverage = 8; // Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3; // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 100; // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; // Options: 69, 118, 215, 411
  int adcRange = 4096; // Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

  // Take an average of IR readings at power up
  const byte avgAmount = 64;
  for (byte x = 0; x < avgAmount; x++) {
    baseValue += particleSensor.getIR(); // Read the IR value
  }
  baseValue /= avgAmount;

  // Pre-populate the plotter so that the Y scale is close to IR values
  for (int x = 0; x < 500; x++)
    Serial.println(baseValue);
}

void loop() {
  byte ButtonState = digitalRead(ButtonPin);
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    // We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
      rates[rateSpot++] = (byte)beatsPerMinute; // Store this reading in the array
      rateSpot %= RATE_SIZE; // Wrap variable

      // Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;

      if (ButtonState == HIGH || baseValue > 6000) {
        // Heartbeat detected, move solenoid high
        digitalWrite(SolenoidPin, HIGH);
      } else {
        // No heartbeat, move solenoid low
        digitalWrite(SolenoidPin, LOW);
      }
    }
  }

  Serial.print("IR=");
  Serial.print(irValue);
  Serial.print(", BPM=");
  Serial.print(beatsPerMinute);
  Serial.print(", Avg BPM=");
  Serial.print(beatAvg);

  if (irValue < 50000)
    Serial.print(" No finger?");

  Serial.println();
  delay(100);
}

boolean checkForBeat(long irValue) {
  static long threshold = 50000; // Adjust threshold based on sensor data
  static long lastFilteredIR = 0;
  static long filteredIR = 0;
  static long filterWeight = 700; // Adjust weight based on sensor data

  // Apply a simple low-pass filter to the IR values
  filteredIR = ((filterWeight * irValue) + ((1000 - filterWeight) * lastFilteredIR)) / 1000;
  lastFilteredIR = filteredIR;

  // Check for a beat if the filtered IR value is above the threshold
  if (filteredIR > threshold) {
    return true; // Heartbeat detected
  } else {
    return false; // No heartbeat
  }
}
