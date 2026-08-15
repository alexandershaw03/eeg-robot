#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <HITIComm.h>

//HITI metadata
const char code_name[]    PROGMEM = "EEG Robot Transmitter";
const char code_version[] PROGMEM = "1.0.0";

// NRF24L01+ setup
RF24 radio(9, 8);                             // CE, CSN
const byte address[6] = "00001";

// input pins
const int hitiForwardPin = 2;
const int hitiLeftPin = 3;
const int hitiRightPin = 4;

// HITI input polarity - V IMPORTANT!!
const bool HITI_ACTIVE_HIGH = true;

// command constants
const uint8_t CMD_STOP    = 0;
const uint8_t CMD_FORWARD = 1;
const uint8_t CMD_LEFT  = 2;
const uint8_t CMD_RIGHT = 3;

// packet settings
const uint8_t PACKET_VALID_MARKER = 1;

// TX timing
const unsigned long transmitInterval = 50;   // send every 50ms
unsigned long lastTransmitTime = 0;

// packet structure (must match reviever)
struct ControlPacket {
  uint8_t command;
  uint8_t isValid;
  uint8_t sequenceNumber;
};
ControlPacket packet = {CMD_STOP, PACKET_VALID_MARKER, 0};

// helper for HITI activity
bool isHitiInputActive(int pin) {
  int pinState = digitalRead(pin);
  if (HITI_ACTIVE_HIGH) {
    return (pinState == HIGH);
  } else {
    return (pinState == LOW);
  }
}
// command selection
uint8_t determineCommand(bool forwardActive, bool leftActive, bool rightActive) {
  int activeCount = 0;
  if (forwardActive) activeCount++;
  if (leftActive)    activeCount++;
  if (rightActive)   activeCount++;

  if (activeCount == 0) {
    return CMD_STOP;
  }
  if (activeCount == 1) {
    if (forwardActive) return CMD_FORWARD;
    if (leftActive)    return CMD_LEFT;
    if (rightActive)   return CMD_RIGHT;
  }
  return CMD_STOP;
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(hitiForwardPin, INPUT_PULLUP);
  pinMode(hitiLeftPin, INPUT_PULLUP);
  pinMode(hitiRightPin, INPUT_PULLUP);

  // HITI setup
  HC_begin();
  HC_codeName(code_name);
  HC_codeVersion(code_version);

  if (!radio.begin()) {
    Serial.println("nRF24 not responding on transmitter. Check wiring/power.");
    while (1) {}
  }

  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("EEG transmitter ready.");
  Serial.print("HITI active state is set to: ");
  Serial.println(HITI_ACTIVE_HIGH ? "HIGH" : "LOW");

}
void loop() {
  HC_communicate();
  
  unsigned long currentTime = millis();

  // read HITI commands
  bool forwardActive = isHitiInputActive(hitiForwardPin);
  bool leftActive    = isHitiInputActive(hitiLeftPin);
  bool rightActive   = isHitiInputActive(hitiRightPin);
  
  packet.command = determineCommand(forwardActive, leftActive, rightActive);

  // Send heartbeat/control packet at fixed interval
  if (currentTime - lastTransmitTime >= transmitInterval) {
    lastTransmitTime = currentTime;

    packet.isValid = PACKET_VALID_MARKER;

    bool success = radio.write(&packet, sizeof(packet));

    // debug output
    Serial.print("F:");
    Serial.print(forwardActive);
    Serial.print(" L:");
    Serial.print(leftActive);
    Serial.print(" R:");
    Serial.print(rightActive);
    Serial.print(" | CMD:");
    Serial.print(packet.command);
    Serial.print(" | Seq:");
    Serial.print(packet.sequenceNumber);
    Serial.print(" | ");
    Serial.println(success ? "OK" : "FAIL");

    packet.sequenceNumber++;   // auto-wraps at 255
    
  }
}




