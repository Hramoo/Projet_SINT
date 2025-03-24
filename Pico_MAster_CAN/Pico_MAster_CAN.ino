//——————————————————————————————————————————————————————————————————————————————
//  ACAN2515 Demo in loopback mode, for the Raspberry Pi Pico
//  Thanks to Duncan Greenwood for providing this sample sketch
//——————————————————————————————————————————————————————————————————————————————

#ifndef ARDUINO_ARCH_RP2040
#error "Select a Raspberry Pi Pico board"
#endif

//——————————————————————————————————————————————————————————————————————————————

#include <ACAN2515.h>
#include <SPI.h>
//——————————————————————————————————————————————————————————————————————————————
// The Pico has two SPI peripherals, SPI and SPI1. Either (or both) can be used.
// The are no default pin assignments so they must be set explicitly.
// Testing was done with Earle Philhower's arduino-pico core:
// https://github.com/earlephilhower/arduino-pico
//——————————————————————————————————————————————————————————————————————————————

static const byte MCP2515_SCK  = 2 ; // SCK input of MCP2515
static const byte MCP2515_MOSI = 3; // SDI input of MCP2515
static const byte MCP2515_MISO = 4 ; // SDO output of MCP2515

static const byte MCP2515_CS  = 5;  // CS input of MCP2515 (adapt to your design)
static const byte MCP2515_INT = 1 ;  // INT output of MCP2515 (adapt to your design)

//———————SPI1——————————————————————————————————————————————————————————————————————
const int PIN_SCK  = 10; // SCK1 → JA1
const int PIN_MOSI = 11; // MOSI1 → JA3
const int PIN_MISO = 12; // MISO1 ← JA4
const int PIN_CS   = 13; // CS1   → JA2

//————————SP1 finito—————————————————————————————————————————————————————————————————————




//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 Driver object
//——————————————————————————————————————————————————————————————————————————————

ACAN2515 can (MCP2515_CS, SPI, MCP2515_INT) ;

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 Quartz: adapt to your design
//——————————————————————————————————————————————————————————————————————————————

static const uint32_t QUARTZ_FREQUENCY = 16UL * 1000UL * 1000UL ; // 16MHz

//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————

void setup () {
  //--- Switch on builtin led

  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;
  //--- Start serial
  Serial.begin (115200) ;
  Serial.print ("pitié ") ;
  //--- Wait for serial (blink led at 10 Hz during waiting)
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
  //--- There are no default SPI pins so they must be explicitly assigned
  SPI.setSCK(MCP2515_SCK);
  SPI.setTX(MCP2515_MOSI);
  SPI.setRX(MCP2515_MISO);
  SPI.setCS(MCP2515_CS);
  //--- Begin SPI
  SPI.begin () ;
  //--- Configure ACAN2515
  Serial.println ("Configure ACAN2515") ;
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125UL * 1000UL) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode ; // Select loopback mode
  const uint16_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
  if (errorCode == 0) {
    Serial.print ("Bit Rate prescaler: ") ;
    Serial.println (settings.mBitRatePrescaler) ;
    Serial.print ("Propagation Segment: ") ;
    Serial.println (settings.mPropagationSegment) ;
    Serial.print ("Phase segment 1: ") ;
    Serial.println (settings.mPhaseSegment1) ;
    Serial.print ("Phase segment 2: ") ;
    Serial.println (settings.mPhaseSegment2) ;
    Serial.print ("SJW: ") ;
    Serial.println (settings.mSJW) ;
    Serial.print ("Triple Sampling: ") ;
    Serial.println (settings.mTripleSampling ? "yes" : "no") ;
    Serial.print ("Actual bit rate: ") ;
    Serial.print (settings.actualBitRate ()) ;
    Serial.println (" bit/s") ;
    Serial.print ("Exact bit rate ? ") ;
    Serial.println (settings.exactBitRate () ? "yes" : "no") ;
    Serial.print ("Sample point: ") ;
    Serial.print (settings.samplePointFromBitStart ()) ;
    Serial.println ("%") ;
  } else {
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
//----------------------------------------------------------------------------------------------------------------------
  SPI1.setSCK(PIN_SCK);
  SPI1.setTX(PIN_MOSI);
  SPI1.setRX(PIN_MISO);

  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH); // Désactive CS au repos

  SPI1.begin();
  SPI1.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0)); // 500 kHz, MSB first

//----------------------------------------------------------------------------------------------------------------------
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t gBlinkLedDate = 0 ;
static uint32_t gReceivedFrameCount = 0 ;
static uint32_t gSentFrameCount = 0 ;

//——————————————————————————————————————————————————————————————————————————————
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
}
//——————————————————————————————————————————————————————
void loop () {
CANMessage frame ;
frame.id= 0x123;
frame.len=1;
  //allumage LED
  digitalWrite(PIN_CS, LOW);
  SPI1.transfer(0x01);
  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(100); // court délai pour laisser la Basys traiter

  // Étape 2 : lecture de la réponse
  digitalWrite(PIN_CS, LOW);
  uint8_t response = SPI1.transfer(0x00);
  digitalWrite(PIN_CS, HIGH);
 frame.data[0]=response;
 Serial.print(response);
 Serial.println ("LED allumé") ;  
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 2000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    const bool ok = can.tryToSend (frame) ;
    if (ok) {
      gSentFrameCount += 1 ;
      Serial.print ("Sent: ") ;
      Serial.println (gSentFrameCount) ;
    } else {
      Serial.println ("Send failure") ;
    }
  }
  if (can.available ()) {
    can.receive (frame) ;
    gReceivedFrameCount ++ ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedFrameCount) ;
  }
 //allumage LED
 delay(1000);
  digitalWrite(PIN_CS, LOW);
  SPI1.transfer(0x02);
  digitalWrite(PIN_CS, HIGH);
  delayMicroseconds(100); // court délai pour laisser la Basys traiter
  
  // Étape 2 : lecture de la réponse
  digitalWrite(PIN_CS, LOW);
  response = SPI1.transfer(0x00);
  digitalWrite(PIN_CS, HIGH);
  frame.data[0]=response;
  Serial.print(response);
  Serial.println ("LED éteinte") ;
    if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 2000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    const bool ok = can.tryToSend (frame) ;
    if (ok) {
      gSentFrameCount += 1 ;
      Serial.print ("Sent: ") ;
      Serial.println (gSentFrameCount) ;
    } else {
      Serial.println ("Send failure") ;
    }
  }
  if (can.available ()) {
    can.receive (frame) ;
    gReceivedFrameCount ++ ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedFrameCount) ;
  }
  delay(1000);
}

//——————————————————————————————————————————————————————————————————————————————