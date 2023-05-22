#include <Arduino.h>
#include <WiFi.h>
#include "DHT.h"
#include <FirebaseESP32.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define DEVICE_UID "1X"
#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "WIFI_PASSWORD"
#define API_KEY "API_KEY"
#define DATABASE_URL "DATABASE_URL"
#define ANALOGPIN 34
#define DHTPIN 4
#define DHTTYPE DHT22 // DHT 22  (AM2302), AM2321

// Initialise DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables to hold sensor readings
float temperature = 0;
float humidity = 0;
int moisture = 0;
int sensor_Analog = 0;

// JSON object to hold updated sensor values to be sent to be firebase
FirebaseJson temperature_json;
FirebaseJson humidity_json;
FirebaseJson moisture_json;

// Device Location config
String device_location = "Living Room";
// Firebase Realtime Database Object
FirebaseData fbdo;
// Firebase Authentication Object
FirebaseAuth auth;
// Firebase configuration Object
FirebaseConfig config;
// Firebase database path
String databasePath = "";
// Firebase Unique Identifier
String fuid = "";
// Stores the elapsed time from device start up
unsigned long elapsedMillis = 0;
// The frequency of sensor updates to firebase, set to 10seconds
unsigned long update_interval = 10000;
// Dummy counter to test initial firebase updates
int count = 0;
// Store device authentication status
bool isAuthenticated = false;

void Wifi_Init()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void firebase_init()
{
  // configure firebase API Key
  config.api_key = API_KEY;
  // configure firebase realtime database url
  config.database_url = DATABASE_URL;
  // Enable WiFi reconnection
  Firebase.reconnectWiFi(true);
  Serial.println("------------------------------------");
  Serial.println("Sign up new user...");
  // Sign in to firebase Anonymously
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Success");
    isAuthenticated = true;
    // Set the database path where updates will be loaded for this device
    databasePath = "/" + device_location;
    fuid = auth.token.uid.c_str();
  }
  else
  {
    Serial.printf("Failed, %s\n", config.signer.signupError.message.c_str());
    isAuthenticated = false;
  }
  // Assign the callback function for the long running token generation task, see addons/TokenHelper.h
  config.token_status_callback = tokenStatusCallback;
  // Initialise the firebase library
  Firebase.begin(&config, &auth);
}

// put function declarations here:
// int myFunction(int, int);

void updateSensorReadings()
{
  Serial.println("------------------------------------");
  Serial.println("Reading Sensor data ...");
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  sensor_Analog = analogRead(ANALOGPIN);
  moisture = (100 - ((sensor_Analog / 4095.00) * 100));
  // Check if any reads failed and exit early (to try again).
  if (isnan(temperature) || isnan(humidity) || isnan(moisture))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  Serial.printf("Temperature reading: %.2f \n", temperature);
  Serial.printf("Humidity reading: %.2f \n", humidity);
  Serial.printf("Mositure reading: \n", moisture);
  temperature_json.set("value", temperature);
  humidity_json.set("value", humidity);
  moisture_json.set("value", moisture);
}

void uploadSensorData()
{
  if (millis() - elapsedMillis > update_interval && isAuthenticated && Firebase.ready())
  {
    elapsedMillis = millis();
    updateSensorReadings();
    String temperature_node = databasePath + "/temperature";
    String humidity_node = databasePath + "/humidity";
    String moisture_node = databasePath + "/moisture";
    if (Firebase.setJSON(fbdo, temperature_node.c_str(), temperature_json))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println("ETag: " + fbdo.ETag());
      Serial.print("VALUE: ");
      printResult(fbdo); // see addons/RTDBHelper.h
      Serial.println("------------------------------------");
      Serial.println();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
    if (Firebase.setJSON(fbdo, humidity_node.c_str(), humidity_json))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println("ETag: " + fbdo.ETag());
      Serial.print("VALUE: ");
      printResult(fbdo); // see addons/RTDBHelper.h
      Serial.println("------------------------------------");
      Serial.println();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
    if (Firebase.setJSON(fbdo, moisture_node.c_str(), moisture_json))
    {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
      Serial.println("ETag: " + fbdo.ETag());
      Serial.print("VALUE: ");
      printResult(fbdo); // see addons/RTDBHelper.h
      Serial.println("------------------------------------");
      Serial.println();
    }
    else
    {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
      Serial.println("------------------------------------");
      Serial.println();
    }
  }
}

void setup()
{
  // Initialise serial communication for local diagnostics
  Serial.begin(115200);
  // Initialise Connection with location WiFi
  Wifi_Init();
  // Initialise firebase configuration and signup anonymously
  firebase_init();

  // Initialise DHT library
  dht.begin();
  // Initialise temperature and humidity json data
  temperature_json.add("deviceuid", DEVICE_UID);
  temperature_json.add("name", "DHT22-Temp");
  temperature_json.add("type", "Temperature");
  temperature_json.add("location", device_location);
  temperature_json.add("value", temperature);
  // Print out initial temperature values
  String jsonStr;
  temperature_json.toString(jsonStr, true);
  Serial.println(jsonStr);

  humidity_json.add("deviceuid", DEVICE_UID);
  humidity_json.add("name", "DHT22-Hum");
  humidity_json.add("type", "Humidity");
  humidity_json.add("location", device_location);
  humidity_json.add("value", humidity);
  // Print out initial humidity values
  String jsonStr2;
  humidity_json.toString(jsonStr2, true);
  Serial.println(jsonStr2);

  moisture_json.add("deviceuid", DEVICE_UID);
  moisture_json.add("name", "Soil-Moisture");
  moisture_json.add("type", "Moisture");
  moisture_json.add("location", device_location);
  moisture_json.add("value", moisture);
  // Print out initial humidity values
  String jsonStr3;
  moisture_json.toString(jsonStr3, true);
  Serial.println(jsonStr3);
}

void loop()
{
  uploadSensorData();
}
