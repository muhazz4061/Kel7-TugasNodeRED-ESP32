#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// ======================================
// KONFIGURASI RUANG
// ======================================

#define ROOM 4
#define DHTPIN 4
#define DHTTYPE DHT11
#define LED1 25
#define LED2 26

// ======================================
// WIFI CONFIGURATION
// ======================================

// Isi dengan Wi-Fi yang digunakan
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// ======================================
// MQTT CONFIGURATION
// ======================================

const char* MQTT_SERVER = "10.57.154.212";
const int MQTT_PORT = 1883;

const char* MQTT_USER = "klp7";
const char* MQTT_PASSWORD = "123456789";

const char* CLIENT_ID = "ESP32_KLP7_RUANG4";

// ======================================
// MQTT TOPICS
// ======================================

// Sensor
const char* TOPIC_TEMP =
  "pens/klp7/ruang4/temperature";

const char* TOPIC_HUM =
  "pens/klp7/ruang4/humidity";

// Perintah kontrol lampu
const char* TOPIC_LED1 =
  "pens/klp7/ruang4/led1/set";

const char* TOPIC_LED2 =
  "pens/klp7/ruang4/led2/set";

// Status lampu
const char* TOPIC_LED1_STATUS =
  "pens/klp7/ruang4/led1/status";

const char* TOPIC_LED2_STATUS =
  "pens/klp7/ruang4/led2/status";

// Status ESP32
const char* TOPIC_STATUS =
  "pens/klp7/ruang4/status";

// ======================================
// OBJECT
// ======================================

WiFiClient espClient;
PubSubClient mqtt(espClient);
DHT dht(DHTPIN, DHTTYPE);

// ======================================
// TIMER
// ======================================

unsigned long lastSensorPublish = 0;

const unsigned long sensorInterval = 5000;

// ======================================
// WIFI CONNECTION
// ======================================

void connectWiFi() {

  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println();
  Serial.println("Menghubungkan ke WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");

  }

  Serial.println();
  Serial.println("WiFi berhasil terhubung!");

  Serial.print("IP ESP32: ");
  Serial.println(WiFi.localIP());

}

// ======================================
// PUBLISH STATUS LAMPU
// ======================================

void publishLedStatus() {

  const char* status1 =
    digitalRead(LED1) == HIGH ? "ON" : "OFF";

  const char* status2 =
    digitalRead(LED2) == HIGH ? "ON" : "OFF";

  mqtt.publish(
    TOPIC_LED1_STATUS,
    status1,
    true
  );

  mqtt.publish(
    TOPIC_LED2_STATUS,
    status2,
    true
  );

  Serial.print("Lampu 1: ");
  Serial.println(status1);

  Serial.print("Lampu 2: ");
  Serial.println(status2);

}

// ======================================
// MQTT CALLBACK
// MENERIMA PERINTAH DARI NODE-RED
// ======================================

void mqttCallback(
  char* topic,
  byte* payload,
  unsigned int length
) {

  String message = "";

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  message.trim();

  Serial.println();
  Serial.println("Perintah MQTT diterima!");

  Serial.print("Topic: ");
  Serial.println(topic);

  Serial.print("Pesan: ");
  Serial.println(message);

  // Kontrol Lampu 1
  if (String(topic) == TOPIC_LED1) {

    if (message == "ON") {

      digitalWrite(LED1, HIGH);

    } else if (message == "OFF") {

      digitalWrite(LED1, LOW);

    }

    publishLedStatus();

  }

  // Kontrol Lampu 2
  else if (String(topic) == TOPIC_LED2) {

    if (message == "ON") {

      digitalWrite(LED2, HIGH);

    } else if (message == "OFF") {

      digitalWrite(LED2, LOW);

    }

    publishLedStatus();

  }

}

// ======================================
// MQTT CONNECTION
// ======================================

void connectMQTT() {

  while (!mqtt.connected()) {

    Serial.println();
    Serial.println("Menghubungkan ke MQTT Broker...");

    bool connected = mqtt.connect(
      CLIENT_ID,
      MQTT_USER,
      MQTT_PASSWORD,
      TOPIC_STATUS,
      1,
      true,
      "offline"
    );

    if (connected) {

      Serial.println("MQTT berhasil terhubung!");

      // Publikasi status ESP32
      mqtt.publish(
        TOPIC_STATUS,
        "online",
        true
      );

      // Subscribe perintah lampu
      mqtt.subscribe(TOPIC_LED1);
      mqtt.subscribe(TOPIC_LED2);

      Serial.println("Subscribe Lampu 1 berhasil");
      Serial.println("Subscribe Lampu 2 berhasil");

      // Kirim status aktual kedua lampu
      publishLedStatus();

    } else {

      Serial.print("MQTT gagal, state: ");
      Serial.println(mqtt.state());

      Serial.println("Mencoba kembali dalam 3 detik...");

      delay(3000);

    }

  }

}

// ======================================
// PUBLISH DATA SENSOR
// ======================================

void publishSensor() {

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Periksa pembacaan sensor
  if (isnan(temperature) || isnan(humidity)) {

    Serial.println("Gagal membaca sensor DHT11!");

    return;

  }

  // Konversi data ke String
  char tempBuffer[10];
  char humBuffer[10];

  dtostrf(temperature, 1, 1, tempBuffer);
  dtostrf(humidity, 1, 1, humBuffer);

  // Kirim suhu
  mqtt.publish(
    TOPIC_TEMP,
    tempBuffer
  );

  // Kirim kelembapan
  mqtt.publish(
    TOPIC_HUM,
    humBuffer
  );

  Serial.println();
  Serial.println("===== DATA RUANG 4 =====");

  Serial.print("Suhu: ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Kelembapan: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.println("========================");

}

// ======================================
// SETUP
// ======================================

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("================================");
  Serial.println("  IoT SMART ROOM - RUANG 4");
  Serial.println("  KELOMPOK 7");
  Serial.println("================================");

  // Konfigurasi LED
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // Kondisi awal LED mati
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);

  // Inisialisasi DHT11
  dht.begin();

  Serial.println("DHT11 berhasil diinisialisasi");

  // Hubungkan WiFi
  connectWiFi();

  // Konfigurasi MQTT
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqttCallback);

  // Hubungkan MQTT
  connectMQTT();

}

// ======================================
// LOOP
// ======================================

void loop() {

  // Periksa koneksi WiFi
  if (WiFi.status() != WL_CONNECTED) {

    connectWiFi();

  }

  // Periksa koneksi MQTT
  if (!mqtt.connected()) {

    connectMQTT();

  }

  // Jalankan MQTT
  mqtt.loop();

  // Kirim sensor setiap 5 detik
  unsigned long currentMillis = millis();

  if (currentMillis - lastSensorPublish >= sensorInterval) {

    lastSensorPublish = currentMillis;

    publishSensor();

  }

}
