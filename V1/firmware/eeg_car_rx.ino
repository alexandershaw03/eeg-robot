#include <RF24.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <Arduino_LED_Matrix.h>

// NRF24L01+ setup
RF24 radio(9, 8);                             // CE, CSN
const byte address[6] = "00001";

// LED matrix
ArduinoLEDMatrix matrix;
// ON frame
uint8_t matrixAllOn[8][12] = {
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1},
  {1,1,1,1,1,1,1,1,1,1,1,1}
};
// OFF frame
uint8_t matrixAllOff[8][12] = {
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0}
};

// TB6612FNG pins
const int PWMB = 6;
const int BIN2 = 2;
const int BIN1 = 7;
const int AIN1 = 5;
const int AIN2 = 4;
const int PWMA = 3;

// motor controls
const int speedHi = 180;
const int speedMed = 100;
const int speedLo = 10;
// Soft start settings
const unsigned long rampInterval = 30;  // ms between speed updates
const int rampStep = 1;                 // PWM step, smaller=softer

unsigned long lastRampTime = 0;

// Current and target motor speeds
int currentLeftSpeed = 0;
int currentRightSpeed = 0;
int targetLeftSpeed = 0;
int targetRightSpeed = 0;


//command constants
const uint8_t CMD_STOP = 0;
const uint8_t CMD_FORWARD = 1;
const uint8_t CMD_LEFT = 2;
const uint8_t CMD_RIGHT = 3;

//packet settings
const uint8_t PACKET_VALID_MARKER = 1;

// radio timeout and failsafe
unsigned long lastReceivedTime = 0;
unsigned long lastRetryTime = 0;
unsigned long signalLostStartTime = 0;

const unsigned long timeout = 200;            // no packet .2s = signal lost
const unsigned long retryInterval = 500;      // .5s = light recovery
const unsigned long restartThreshold = 2500;  // 2.5s = full restart
const unsigned long restartCooldown = 5000;   // 5s between restarts

unsigned long lastRestartTime = 0;

bool signalLost = true;

// matrix flashing timing
const unsigned long matrixFlashInterval = 250;
unsigned long lastMatrixFlashTime = 0;
bool matrixFlashState = false;
//  matrix spinning
const unsigned long matrixSpinInterval = 120;
unsigned long lastMatrixSpinTime = 0;
int matrixSpinIndex = 0;

// packet/struct structure
struct ControlPacket {
  uint8_t command;         // command constant
  uint8_t isValid;         // simple packet validity marker
  uint8_t sequenceNumber;  // increments on transmitter each new packet
};
ControlPacket receivedPacket = {CMD_STOP, 0, 0};
// track packet ages
bool hasReceivedFirstPacket = false;
uint8_t lastSequenceNumber = 0;

// Connected animation path around outer edge
const int spinPointCount = 36;
const int spinRows[spinPointCount] = {
  0,0,0,0,0,0,0,0,0,0,0,0,
  1,2,3,4,5,6,
  7,7,7,7,7,7,7,7,7,7,7,7,
  6,5,4,3,2,1
};
const int spinCols[spinPointCount] = {
  0,1,2,3,4,5,6,7,8,9,10,11,
  11,11,11,11,11,11,
  11,10,9,8,7,6,5,4,3,2,1,0,
  0,0,0,0,0,0
};

// helper functions+soft start
int rampTowards(int currentValue, int targetValue, int stepSize) {
  if (currentValue < targetValue) {
    currentValue += stepSize;
    if (currentValue > targetValue) {
      currentValue = targetValue;
    }
  } else if (currentValue > targetValue) {
    currentValue -= stepSize;
    if (currentValue < targetValue) {
      currentValue = targetValue;
    }
  }

  return currentValue;
}
void applyMotorOutputs(int leftSpeed, int rightSpeed) {
  // left motor = A side
  if (leftSpeed <= 0) {
    analogWrite(PWMA, 0);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
  } else {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, leftSpeed);
  }
  // right motor = B side
  if (rightSpeed <= 0) {
    analogWrite(PWMB, 0);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);
  } else {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
    analogWrite(PWMB, rightSpeed);
  }
}
void setTargetSpeeds(int leftSpeed, int rightSpeed) {
  targetLeftSpeed = constrain(leftSpeed, 0, 255);
  targetRightSpeed = constrain(rightSpeed, 0, 255);
}

void stopMotors() {
  setTargetSpeeds(0, 0);

}
void moveForward() {
  setTargetSpeeds(speedHi, speedHi);

}
void moveForwardLeft() {
  setTargetSpeeds(speedLo, speedMed);

}
void moveForwardRight() {
  setTargetSpeeds(speedMed, speedLo);

}
void failSafe() {
  setTargetSpeeds(0, 0);

}

void recoverRadio() {
  radio.stopListening();
  delay(10);
  radio.startListening();

}  // restart listening mode in case rx state glitched

void radioRestart(){
  radio.powerDown();
  delay(20);
  radio.powerUp();
  delay(20);
  radio.startListening();

}  // full radio reboot

bool isPacketValid(const ControlPacket &packet) {
  return (packet.isValid == PACKET_VALID_MARKER) &&
         (packet.command == CMD_STOP ||
          packet.command == CMD_FORWARD ||
          packet.command == CMD_LEFT ||
          packet.command == CMD_RIGHT);

}
bool isNewPacket(const ControlPacket &packet) {
  if (!hasReceivedFirstPacket) {
    return true;
  }
  return packet.sequenceNumber != lastSequenceNumber;

}
void handleCommand(const ControlPacket &packet) {
  switch (packet.command) {
    case CMD_LEFT:
      moveForwardLeft();
      break;

    case CMD_RIGHT:
      moveForwardRight();
      break;

    case CMD_FORWARD:
      moveForward();
      break;

    case CMD_STOP:
    default:
      stopMotors();
      break;
  }
} //forward, right, left command handling

void updateMotorRamp() {
  unsigned long currentTime = millis();
  if (currentTime - lastRampTime >= rampInterval) {
    lastRampTime = currentTime;

    currentLeftSpeed = rampTowards(currentLeftSpeed, targetLeftSpeed, rampStep);
    currentRightSpeed = rampTowards(currentRightSpeed, targetRightSpeed, rampStep);

    applyMotorOutputs(currentLeftSpeed, currentRightSpeed);
  }

}

/*void renderSpinFrame(int pointIndex) {
  uint8_t frame[8][12] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  };

  frame[spinRows[pointIndex]][spinCols[pointIndex]] = 1;
  matrix.renderBitmap(frame, 8, 12);

}*/ //one dot spin

void renderSpinFrame(int headIndex) {
  uint8_t frame[8][12] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  };

  const int snakeLength = 3;

  for (int i = 0; i < snakeLength; i++) {
    int index = headIndex - i;
    if (index < 0) {
      index += spinPointCount;
    }
    frame[spinRows[index]][spinCols[index]] = 1;
  }
  matrix.renderBitmap(frame, 8, 12);

} // three dot spin

/*void renderSpinFrame(int offsetIndex) {
  uint8_t frame[8][12] = {
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0}
  };

  const int dotCount = 10;

  for (int i = 0; i < dotCount; i++) {
    int index = (offsetIndex + (i * spinPointCount / dotCount)) % spinPointCount;
    frame[spinRows[index]][spinCols[index]] = 1;
  }
  matrix.renderBitmap(frame, 8, 12);

}*/ //outer ring spin

void updateMatrixStatus() {
  unsigned long currentTime = millis();
  if (signalLost) {
    if (currentTime - lastMatrixFlashTime >= matrixFlashInterval) {
      lastMatrixFlashTime = currentTime;
      matrixFlashState = !matrixFlashState;

      if (matrixFlashState) {
        matrix.renderBitmap(matrixAllOn, 8, 12);
      } else {
        matrix.renderBitmap(matrixAllOff, 8, 12);
      }
    }
    } else {
    if (currentTime - lastMatrixSpinTime >= matrixSpinInterval) {
      lastMatrixSpinTime = currentTime;
      renderSpinFrame(matrixSpinIndex);
      matrixSpinIndex++;
      if (matrixSpinIndex >= spinPointCount) {
        matrixSpinIndex = 0;
      }
    }
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);

  matrix.begin();
  matrix.renderBitmap(matrixAllOff, 8, 12);

  stopMotors();
  applyMotorOutputs(0, 0);

  if (!radio.begin()) {
    Serial.println("nRF24 not responding on receiver. Check wiring/power.");
    while (1) {}
  }

  radio.setAutoAck(true);
  radio.setPALevel(RF24_PA_LOW);
  radio.setChannel(76);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(0, address);
  radio.startListening();

  signalLostStartTime = millis();

  Serial.println("Receiver ready and listening.");
}

void loop() {
  bool gotPacket = false;
  ControlPacket latestPacket;

  // drain RX buffer so newest packet used
  while (radio.available()) {
    radio.read(&latestPacket, sizeof(latestPacket));
    gotPacket = true;  

  }
  // use newest packet only
  if (gotPacket) {
    if (isPacketValid(latestPacket)) {
      if (isNewPacket(latestPacket)) {
        receivedPacket = latestPacket;
        lastReceivedTime = millis();

        if (signalLost) {
          signalLost = false;
          Serial.println("Signal restored");
        }

        hasReceivedFirstPacket = true;
        lastSequenceNumber = receivedPacket.sequenceNumber;

        Serial.print("Received command: ");
        Serial.print(receivedPacket.command);
        Serial.print(" | Seq: ");
        Serial.println(receivedPacket.sequenceNumber);

        handleCommand(receivedPacket);
      } else {
        Serial.print("Duplicate packet ignored | Seq: ");
        Serial.println(latestPacket.sequenceNumber);
      }
    } else {
      Serial.println("Invalid packet ignored");
    }

  }
  // detect signal loss
  if (!signalLost && (millis() - lastReceivedTime > timeout)) {
    signalLost = true;
    signalLostStartTime = millis();
    Serial.println("Signal lost");
    failSafe();

  }
  // soft radio recovery
  if (signalLost && (millis() - lastRetryTime > retryInterval)) {
    lastRetryTime = millis();
    Serial.println("Retrying receiver (light recovery)...");
    recoverRadio();

  }
  // hard radio recovery
  if (signalLost &&
      (millis() - signalLostStartTime > restartThreshold) &&
      (millis() - lastRestartTime > restartCooldown)) {
    lastRestartTime = millis();
    Serial.println("Escalating to full radio restart...");
    radioRestart();
  }
  // ALWAYS RUN :
  updateMotorRamp();
  updateMatrixStatus();

}





