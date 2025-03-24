#include <SPI.h>

// SPI1 sur GPIO 10–13
const int PIN_SCK  = 10; // SCK1 → JA1
const int PIN_MOSI = 11; // MOSI1 → JA3
const int PIN_MISO = 12; // MISO1 ← JA4
const int PIN_CS   = 13; // CS1   → JA2

void setup() {
  Serial.begin(115200);

  // Configuration manuelle des broches SPI1
  SPI1.setSCK(PIN_SCK);
  SPI1.setTX(PIN_MOSI);
  SPI1.setRX(PIN_MISO);

  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH); // Désactive CS au repos

  SPI1.begin();
  SPI1.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0)); // 500 kHz, MSB first
}

void sendSPICommand(uint8_t command) {
  // Étape 1 : envoi de la commande
  digitalWrite(PIN_CS, LOW);
  SPI1.transfer(command);
  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(100); // court délai pour laisser la Basys traiter

  // Étape 2 : lecture de la réponse
  digitalWrite(PIN_CS, LOW);
  uint8_t response = SPI1.transfer(0x00);
  digitalWrite(PIN_CS, HIGH);

  // Affichage dans la console
  Serial.print("Commande 0x");
  Serial.print(command, HEX);
  Serial.print(" → Réponse 0x");
  Serial.println(response, HEX);
}

void loop() {
  // Allume la LED
  sendSPICommand(0x01);
  delay(3000);

  // Éteint la LED
  sendSPICommand(0x02);
  delay(3000);
}
