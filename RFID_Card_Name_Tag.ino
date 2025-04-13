#include <SPI.h>
#include <MFRC522.h>

// Pin Configuration
constexpr uint8_t RST_PIN = D3;
constexpr uint8_t SS_PIN = D4;

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// Block configuration
int blockNum = 2;  // Block tujuan
byte blockData[16];  // Data yang akan disimpan
byte bufferLen = 18;
byte readBlockData[18];

// Status code
MFRC522::StatusCode status;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  
  Serial.println("Siapkan RFID Tag dan input data melalui Serial Monitor...");
}

void loop() {
  // Setup kunci default (0xFF)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Cek apakah kartu RFID baru terdeteksi
  if (!mfrc522.PICC_IsNewCardPresent()) { return; }
  if (!mfrc522.PICC_ReadCardSerial()) { return; }

  Serial.println("\n**Kartu Terdeteksi**");
  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  // Input data dari Serial Monitor
  Serial.println("Masukkan data (maksimal 16 karakter):");
  while (Serial.available() == 0);  // Menunggu input
  String inputData = Serial.readStringUntil('\n');  // Baca data input

  // Konversi input ke array byte
  memset(blockData, 0, sizeof(blockData));  // Reset array
  inputData.getBytes(blockData, 16);  // Masukkan data ke array byte

  // Menulis data ke kartu
  WriteDataToBlock(blockNum, blockData);

  // Membaca kembali data dari kartu
  ReadDataFromBlock(blockNum, readBlockData);
  Serial.print("Data di Block ");
  Serial.print(blockNum);
  Serial.print(": ");
  for (int j = 0; j < 16; j++) {
    Serial.write(readBlockData[j]);
  }
  Serial.println();
  
  // Hentikan komunikasi dengan kartu
  mfrc522.PICC_HaltA();
}

/****************************************************************************************************
 * Function untuk menulis data ke Block
 ****************************************************************************************************/
void WriteDataToBlock(int blockNum, byte blockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Autentikasi Gagal untuk Write: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.println("Autentikasi Berhasil - Menulis Data...");
  
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Penulisan Gagal: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.println("Data Berhasil Ditulis!");
  }
}

/****************************************************************************************************
 * Function untuk membaca data dari Block
 ****************************************************************************************************/
void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Autentikasi Gagal untuk Read: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Pembacaan Gagal: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    Serial.println("Data Berhasil Dibaca!");
  }
}
