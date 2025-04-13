// Viral Science www.viralsciencecreativity.com www.youtube.com/c/viralscience
// Smart Attendance System with Google Sheets
#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
//-----------------------------------------
#define RST_PIN  D3
#define SS_PIN   D4
#define LED_RED  D1  // LED merah
#define LED_YELLOW D2 // LED kuning
//-----------------------------------------
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  
MFRC522::StatusCode status;      
//-----------------------------------------
int blockNum = 2;  
byte bufferLen = 18;
byte readBlockData[18];
String lastCardID = "";  // Menyimpan ID kartu terakhir
unsigned long lastReadTime = 0; // Waktu terakhir membaca kartu
const int debounceDelay = 2000; // Delay untuk debounce kartu (2 detik)
//-----------------------------------------
String card_holder_name;
const String sheet_url = "https://script.google.com/macros/s/AKfycbxKpoCSPVUhuiwfy1LFsXOMwYiYUZffUspToTfq-6HWnZddFfrCl9mDjg5P2PO5THc6/exec?name=";  
//-----------------------------------------
#define WIFI_SSID "Aljomog"  // Enter WiFi Name
#define WIFI_PASSWORD "11223344"  // Enter WiFi Password
//-----------------------------------------

void setup() {
  Serial.begin(9600);
  Serial.println();

  // Setup LED pins
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  digitalWrite(LED_RED, HIGH);    // LED merah menyala awalnya
  digitalWrite(LED_YELLOW, LOW);  // LED kuning mati awalnya

  // WiFi Connectivity
  Serial.print("Connecting to AP");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize SPI bus
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID reader initialized. Ready to scan...");
}

void loop() {
  digitalWrite(LED_RED, HIGH);  // LED merah menyala saat menunggu kartu

  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  // Membaca ID kartu sebagai string
  String currentCardID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    currentCardID += String(mfrc522.uid.uidByte[i], HEX);
  }
  currentCardID.toUpperCase();

  // Debounce kartu agar tidak diproses berulang kali
  if (currentCardID == lastCardID && millis() - lastReadTime < debounceDelay) {
    return;
  }

  lastCardID = currentCardID;
  lastReadTime = millis();

  Serial.println(F("Reading data from RFID..."));
  ReadDataFromBlock(blockNum, readBlockData);

  Serial.print(F("Card ID: "));
  Serial.println(currentCardID);

  // Menggabungkan URL untuk Google Sheets
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();

    card_holder_name = sheet_url + String((char*)readBlockData);
    card_holder_name.trim();
    Serial.println("Sending to Google Sheets: " + card_holder_name);

    HTTPClient https;
    if (https.begin(*client, card_holder_name)) {
      int httpCode = https.GET();
      if (httpCode > 0) {
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // Jika data berhasil dikirim
        digitalWrite(LED_RED, LOW);    // Matikan LED merah
        digitalWrite(LED_YELLOW, HIGH); // Nyalakan LED kuning
        delay(1000);                   // Tunggu 1 detik
        digitalWrite(LED_YELLOW, LOW); // Matikan LED kuning
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
  }

  // Hentikan komunikasi dengan kartu
  mfrc522.PICC_HaltA();        // Hentikan komunikasi dengan kartu
  mfrc522.PCD_StopCrypto1();   // Matikan sesi enkripsi untuk menghindari masalah membaca ulang
  Serial.println("Waiting for next card...");
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) { 
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
     Serial.print("Authentication failed: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.println("Block read successfully");
}
