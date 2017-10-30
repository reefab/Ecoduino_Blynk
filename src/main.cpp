/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <BlynkSimpleShieldEsp8266.h>
#include "DHT.h"

#include "config.h"

// Hardware Serial on Leonardo
#define EspSerial Serial1

// Your ESP8266 baud rate:
#define ESP8266_BAUD 115200

ESP8266 wifi(&EspSerial);

#define DHTTYPE DHT11
#define DHTPIN 9
DHT dht(DHTPIN, DHTTYPE);

#include <LightDependentResistor.h>
LightDependentResistor photocell(A3, 10000);

BlynkTimer timer;

int soilMoisture;
int soilMoistureThreshold = 0;

void readSensors() {
    Serial.println("Reading sensors");
    // DHT
    float h = dht.readHumidity();
    Serial.print("Ambiant Humdity: ");
    Serial.println(h);
    Blynk.virtualWrite(V1, h);
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    Serial.print("Ambiant Temperature: ");
    Serial.println(t);
    Blynk.virtualWrite(V2, t);
    // Soil Moisture Sensor
    int s1 = analogRead(A2);
    soilMoisture = map(s1, 0, 1023, 0, 100);
    Serial.print("Soil Moisture: ");
    Serial.println(soilMoisture);
    Blynk.virtualWrite(V3, soilMoisture);
    // Lux
    float intensity_in_lux = photocell.getCurrentLux();
    Serial.print("Light intensity: ");
    Serial.print(intensity_in_lux);
    Blynk.virtualWrite(V4, intensity_in_lux);
}

// Let the app set the threshold below which the watering will be triggered via a
// slider (0-100)
BLYNK_WRITE(V10) {
    soilMoistureThreshold = param.asInt();
    Serial.print("Updated SoildMoisture Threshold to: ");
    Serial.println(soilMoistureThreshold);
}

// Receive a button event to trigger sensor data refresh and push
BLYNK_WRITE(V11) {
    int refresh_requested = param.asInt();
    if (refresh_requested == 1) {
        readSensors();
    }
}

//open pump
void pumpOn() {
    digitalWrite(5, HIGH);
    digitalWrite(6, HIGH);
}

//close pump
void pumpOff() {
    digitalWrite(5, LOW);
    digitalWrite(6, LOW);
}

void water() {
    // refresh soil moisture data (among others)
    readSensors();
    // refresh threshold setting from app
    Blynk.syncVirtual(V10);
    if (soilMoisture < soilMoistureThreshold) {
        Serial.println("Watering triggered");
        pumpOn();
        // Close the pump after 5 seconds
        timer.setTimeout(5000L, pumpOff);
    }
}

void setup() {
    // Pump pins setup
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pumpOff();
    // Debug console
    Serial.begin(9600);
    Serial.println("Starting....");

    EspSerial.begin(ESP8266_BAUD);
    delay(10);

    Blynk.begin(auth, wifi, ssid, pass);
    // Fetch previous soil moisture threshold from server
    Blynk.syncVirtual(V10);
    dht.begin();
    timer.setInterval(300000L, readSensors);
    // The pump needs some time between activations so that the water can seep into
    // the soil between sensor readings. This should hopefully prevent over-watering.
    timer.setInterval(600000L, water);
}

void loop() {
    Blynk.run();
    timer.run();
}
