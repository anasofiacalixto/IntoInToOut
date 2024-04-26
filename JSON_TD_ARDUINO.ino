#include <ArduinoJson.h>

const int analogInPin = A0;
const int analogOutPin = 9;

int sensorValue = 0;
int outputValue = 0;



void setup() {  

  // SERIAL
  Serial.begin(115200);
  
}


void loop() {
  
  // Initialise the JSON document
  DynamicJsonDocument  doc(200);
  
  // Add various values to the JSON document.
  sensorValue = analogRead(analogInPin);
  outputValue = map(sensorValue, 742, 405, 0, 255);
  analogWrite(analogOutPin, outputValue);

  doc["GSR1"] = outputValue;
   
   // Initilaise the string to send
  String jsonOut = "";

  // Convert the JSON to text
  serializeJson(doc, jsonOut);

  // Send the JSON out over Serial
  Serial.println(jsonOut);
  delay(2);

}
