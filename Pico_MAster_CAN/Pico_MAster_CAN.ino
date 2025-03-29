#include <ACAN2515.h>
#include <SPI.h>

// ------------------------- PIN CONFIG -------------------------
const byte MCP2515_SCK  = 2;
const byte MCP2515_MOSI = 3;
const byte MCP2515_MISO = 4;
const byte MCP2515_CS   = 5;
const byte MCP2515_INT  = 1;

const byte PIN_SCK  = 10; // SPI1 SCK -> JA1
const byte PIN_MOSI = 11; // SPI1 MOSI -> JA3
const byte PIN_MISO = 12; // SPI1 MISO -> JA4
const byte PIN_CS   = 13; // SPI1 CS -> JA2

ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);
static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL; // 16MHz

uint32_t gBlinkLedDate = 0;
uint32_t gReceivedFrameCount = 0;
uint32_t gSentFrameCount = 0;

// ------------------------- SETUP -------------------------
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while (!Serial) {
    delay(50);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }

  // SPI pour MCP2515 (CAN)
  SPI.setSCK(MCP2515_SCK);
  SPI.setTX(MCP2515_MOSI);
  SPI.setRX(MCP2515_MISO);
  SPI.setCS(MCP2515_CS);
  SPI.begin();

  Serial.println("Configure ACAN2515");
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 125UL * 1000UL);
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  uint16_t errorCode = can.begin(settings, [] { can.isr(); });

  if (errorCode == 0) {
    Serial.println("CAN OK");
  } else {
    Serial.print("CAN init error 0x");
    Serial.println(errorCode, HEX);
  }

  // SPI1 pour communication Basys
  SPI1.setSCK(PIN_SCK);
  SPI1.setTX(PIN_MOSI);
  SPI1.setRX(PIN_MISO);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI1.begin();
  SPI1.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
}

// ------------------------- SPI COMMANDE -------------------------
uint8_t sendSPICommand(uint8_t cmd) {
  digitalWrite(PIN_CS, LOW);
  SPI1.transfer(cmd);
  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(100);

  digitalWrite(PIN_CS, LOW);
  uint8_t response = SPI1.transfer(0x00);
  digitalWrite(PIN_CS, HIGH);
  return response;
}

// ------------------------- LOOP -------------------------
void loop() {
  CANMessage frame;
  
  // Vérifiez si un message CAN est disponible
  if (can.available()) {
    can.receive(frame);  // Recevoir le message CAN
    gReceivedFrameCount++;
    Serial.print("CAN reçu #");
    Serial.println(gReceivedFrameCount);

    // Vérifiez si le message CAN contient une commande 'on' (frame.data[0] == 1)
    if (frame.data[0] == 1) {
      // Commande "on" reçue, allumer la LED
      uint8_t response = sendSPICommand(0x01); // Exemple de commande SPI pour allumer la LED
      frame.data[0] = response;  // Réponse de la commande SPI
      Serial.print("Commande 0x01 → Réponse 0x");
      Serial.println(response, HEX);
      can.tryToSend(frame);  // Envoyer la réponse via CAN
    }
    // Vérifiez si le message CAN contient une commande "off" (frame.data[0] == 0)
    else if (frame.data[0] == 0) {
      // Commande "off" reçue, éteindre la LED
      Serial.println("Commande OFF reçue. Éteindre LED.");
      
      // Exemple de commande SPI pour éteindre la LED
      uint8_t response = sendSPICommand(0x02);
      frame.data[0] = response;  // Réponse de la commande SPI
      can.tryToSend(frame);  // Envoyer la réponse via CAN
    }
  }
}
