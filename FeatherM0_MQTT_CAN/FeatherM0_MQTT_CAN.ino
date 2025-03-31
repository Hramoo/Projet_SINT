//——————————————————————————————————————————————————————————————————————————————
//  ACAN2515 Demo in loopback mode for Adafruit Feather M0
//——————————————————————————————————————————————————————————————————————————————

#include <ACAN2515.h>
#include <wiring_private.h>
#include <LiquidCrystal.h>

// LCD pins : RS, E, D4, D5, D6, D7
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
#include <SPI.h>
#include <WiFi101.h>
#include <PubSubClient.h>

#define MQTT_BROKER "192.168.1.110"
#define MQTT_PORT 1883

WiFiClient wifiClient;
PubSubClient client(wifiClient);
//char ssid[] = "raspi-webgui2";
//char pass[] = "ChangeMe";
char ssid[] = "Bbox-25CDE8E1";
char pass[] = "Omar2002";
int status = WL_IDLE_STATUS;

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 connections: aapt theses settings to your design
//  This sketch is designed for an Adafruit Feather M0, using SERCOM1
//  Standard Adafruit Feather M0 SPI pins are not used
//    SCK input of MCP2515 is connected to pin #12
//    SI input of MCP2515 is connected to pin #11
//    SO output of MCP2515 is connected to pin #10
//  - output pin for CS (MCP2515_CS)
//  - interrupt input pin for INT (MCP2515_INT)
//——————————————————————————————————————————————————————————————————————————————

// https://learn.adafruit.com/using-atsamd21-sercom-to-add-more-spi-i2c-serial-ports/overview

static const byte MCP2515_SCK = 12 ; // SCK pin, SCK input of MCP2515
static const byte MCP2515_SI  = 11 ; // MOSI pin, SI input of MCP2515
static const byte MCP2515_SO  = 10 ; // MISO pin, SO output of MCP2515

SPIClass mySPI (&sercom1, MCP2515_SO, MCP2515_SI, MCP2515_SCK, SPI_PAD_0_SCK_3, SERCOM_RX_PAD_2);

static const byte MCP2515_CS  =  6 ; // CS input of MCP2515
static const byte MCP2515_INT =  5 ; // INT output of MCP2515

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 Driver object
//——————————————————————————————————————————————————————————————————————————————

ACAN2515 can (MCP2515_CS, mySPI, MCP2515_INT) ;

//——————————————————————————————————————————————————————————————————————————————
//  MCP2515 Quartz: adapt to your design
//——————————————————————————————————————————————————————————————————————————————

static const uint32_t QUARTZ_FREQUENCY = 16 * 1000 * 1000 ; // 16 MHz

//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————

void setup () {

//__________88SET UP MQTT DEBUT
  WiFi.setPins(8, 7, 4, 2);
  Serial.begin(115200);
  lcd.begin(20, 4);
  lcd.setCursor(0, 0);
  lcd.print("Omar Montino");
  lcd.setCursor(0, 1);
  lcd.print("Topic : tructruc");
  while (!Serial) {
    ; // wait for serial port to connect
  }
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  Serial.println("Connected to the network");
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  if (client.connect("FeatherM0-MontMont", "tructruc", 1, true, "adios")) {
    Serial.println("Connected to MQTT broker");
    client.subscribe("tructruc");
  } else {
    Serial.print("MQTT connection failed, rc=");
    Serial.println(client.state());
  }
//________SETUP MQTT FIN_______________

//--- Switch on builtin led
  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;
//--- Start serial
  Serial.begin (115200) ;
//--- Wait for serial (blink led at 10 Hz during waiting)
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
//--- Configure SPI
  mySPI.begin () ;
//--- Define alternate pins for SPI
  pinPeripheral (MCP2515_SI, PIO_SERCOM);
  pinPeripheral (MCP2515_SCK, PIO_SERCOM);
  pinPeripheral (MCP2515_SO, PIO_SERCOM);
//--- Configure ACAN2515
  Serial.println ("Configure ACAN2515") ;
  ACAN2515Settings settings (QUARTZ_FREQUENCY, 125 * 1000) ; // CAN bit rate 125 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode ; // Select loopback mode
  const uint32_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
  if (errorCode == 0) {
    Serial.print ("Bit Rate prescaler: ") ;
    Serial.println (settings.mBitRatePrescaler) ;
    Serial.print ("Propagation Segment: ") ;
    Serial.println (settings.mPropagationSegment) ;
    Serial.print ("Phase segment 1: ") ;
    Serial.println (settings.mPhaseSegment1) ;
    Serial.print ("Phase segment 2: ") ;
    Serial.println (settings.mPhaseSegment2) ;
    Serial.print ("SJW:") ;
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
  }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t gBlinkLedDate = 0 ;
static uint32_t gReceivedFrameCount = 0 ;
static uint32_t gSentFrameCount = 0 ;

//——————————————————————————————————————————————————————————————————————————————

void loop () {
  CANMessage frame ;
  client.loop();

  if (can.available ()) {
    can.receive (frame) ;
    gReceivedFrameCount ++ ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedFrameCount) ;
    Serial.println(frame.data[0]);
    
    if (frame.data[0]==0){
    client.publish("tructruc","0");
    }else{
    client.publish("tructruc","1");
    }
  }

}
void callback(char* topic, byte* payload, unsigned int length) {
  // Convertir le message payload en chaîne de caractères
  char message[length + 1];  // +1 pour le caractère de fin de chaîne '\0'
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';  // Assurez-vous que la chaîne est terminée
  CANMessage ordre ;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (strcmp(message, "on") == 0) {
   
    ordre.id = 0x01;  // Id du message CAN (choisissez un id approprié)
    ordre.len = 1;     // La longueur du message CAN
    ordre.data[0] = 1; // Contenu du message CAN pour "on"

  
    if (can.tryToSend(ordre)) {
      Serial.println("Message CAN 'on' envoyé");
    } else {
      Serial.println("Échec de l'envoi du message CAN");
    }
  }else if (strcmp(message, "off") == 0){
        ordre.id = 0x01;  // Id du message CAN (choisissez un id approprié)
    ordre.len = 1;     // La longueur du message CAN
    ordre.data[0] = 0; // Contenu du message CAN pour "on"

  
    if (can.tryToSend(ordre)) {
      Serial.println("Message CAN 'off' envoyé");
    } else {
      Serial.println("Échec de l'envoi du message CAN");
    }
  }
  
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("arduinoClient")) {
      Serial.println("connected");
      client.subscribe("tructruc");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//——————————————————————————————————————————————————————————————————————————————
