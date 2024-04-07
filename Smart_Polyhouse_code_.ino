#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG_SW 1
#define FIREBASE_HOST "https://smart-polyhouse-spartan-webdev-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "QhtUru4lxT6A9Q85wrHbufIDGewncUm2yJwJ6JGJ"
#define WIFI_SSID "esp8266"
#define WIFI_PASSWORD "12345678"
#define bucket "shraddha20102003"

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(T4, DHT11);
FirebaseData firebaseData;
FirebaseJson json;

const int sensor_pin = A0;
#define RELAY_PIN_1 T3
#define RELAY_PIN_2 T6
#define RELAY_PIN_3 T5

int _moisture, sensor_analog;

void setup() {
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);
  digitalWrite(RELAY_PIN_1, LOW);
  digitalWrite(RELAY_PIN_2, LOW);
  digitalWrite(RELAY_PIN_3, LOW);
  dht.begin();
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  Initializing  ");
  for (int a = 5; a <= 10; a++) {
    lcd.setCursor(a, 1);
    lcd.print(".");
    delay(500);
  }
  lcd.clear();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (DEBUG_SW) Serial.println("Not Connected");
    if (DEBUG_SW) Serial.println(bucket " TESTING");
  } else {
    Data_from_firebase();
    DHT11sensor();
    soilMoistureSensor();
    temperaturemaintain();
  }
}

void Data_from_firebase() {
  float moist = 100;
  float minmoist = 100;
  float maxmoist = 100;
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/minmoist")) {
    minmoist = firebaseData.stringData().toFloat();
  }
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/maxmoist")) {
    maxmoist = firebaseData.stringData().toFloat();
  }
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/moisture")) {
    moist = firebaseData.stringData().toFloat();
  }
  if (DEBUG_SW) Serial.println(moist);
  if (DEBUG_SW) Serial.println(maxmoist);
  if (DEBUG_SW) Serial.println(minmoist);
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/motor")) {
    String motor = firebaseData.stringData();
    if (DEBUG_SW) Serial.print("Relay1 - ");
    if (DEBUG_SW) Serial.println(motor);
    if (motor == "ON" && moist < minmoist || motor == "ON" && moist < maxmoist) {
      digitalWrite(RELAY_PIN_1, HIGH);
      lcd.setCursor(11, 1);
      lcd.print("W:ON ");
      if (DEBUG_SW) Serial.print("Motor Started");
    } else {
      digitalWrite(RELAY_PIN_1, LOW);
      lcd.setCursor(11, 1);
      lcd.print("W:OFF");
    }
  }
}

void DHT11sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  json.set("/temperature", t);
  Firebase.updateNode(firebaseData, bucket "/Polyhouse", json);
  json.set("/humidity", h);
  Firebase.updateNode(firebaseData, bucket "/Polyhouse", json);
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t);
  lcd.setCursor(8, 0);
  lcd.print("H:");
  lcd.print(h);
  Serial.println(t);
  Serial.println(h);
}

void soilMoistureSensor() {
  sensor_analog = analogRead(sensor_pin);
  _moisture = (100 - ((sensor_analog / 4095.00) * 100));
  Serial.print("Moisture = ");
  Serial.print(_moisture);
  Serial.println("%");
  json.set("/moisture", _moisture);
  Firebase.updateNode(firebaseData, bucket "/Polyhouse", json);
  lcd.setCursor(0, 1);
  lcd.print("S:");
  lcd.print(_moisture); 
  lcd.print(" ");
}

void temperaturemaintain() {
  float temp = 100;
  float mintemp = 100;
  float maxtemp = 100;
  String heater = "OFF";
  String cool = "OFF";
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/mintemp")) {
    mintemp = firebaseData.stringData().toFloat();
  }
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/maxtemp")) {
    maxtemp = firebaseData.stringData().toFloat();
  }
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/temperature")) {
    temp = firebaseData.stringData().toFloat();
  }
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/heater")) {
    heater = firebaseData.stringData();
  }
  if (Firebase.getString(firebaseData, bucket "/Polyhouse/cool")) {
    cool = firebaseData.stringData();
  }
  if (DEBUG_SW) Serial.print("temp");
  if (DEBUG_SW) Serial.println(temp);
  if (DEBUG_SW) Serial.print("max temp");
  if (DEBUG_SW) Serial.println(maxtemp);
  if (DEBUG_SW) Serial.print("min temp");
  if (DEBUG_SW) Serial.println(mintemp);
  if (temp <= mintemp && heater == "ON" ) {
    digitalWrite(RELAY_PIN_2, HIGH);
    if (DEBUG_SW) Serial.println("Heating started");
  } else {
    digitalWrite(RELAY_PIN_2, LOW);
    if (DEBUG_SW) Serial.println("Heating stopped");
  }
  if (temp >= maxtemp && cool == "ON" ) {
    digitalWrite(RELAY_PIN_3, HIGH);
    if (DEBUG_SW) Serial.println("Cooling Started");
  } else {
    digitalWrite(RELAY_PIN_3, LOW);
    if (DEBUG_SW) Serial.println("Cooling stopped");
  }
}
