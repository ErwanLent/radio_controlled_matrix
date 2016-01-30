/* ==========================================================================
   Libraries
   ========================================================================== */
#include <Adafruit_GFX.h>   // Core graphics library
#include <RGBmatrixPanel.h> // Hardware-specific library

// Radio Libaries
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

/* ==========================================================================
   Pin Configuration
   ========================================================================== */
   
#define CLK 11
#define OE  9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

RGBmatrixPanel matrix(A, B, C, D, CLK, LAT, OE, false);
RF24 radio(12, 13);

/* ==========================================================================
   Global Variables
   ========================================================================== */

// Primary Colors
static byte _red[3] = {15, 0, 0};
static byte _yellow[3] = {15, 15, 0};
static byte _turquoise[3] = {0, 15, 15};
static byte _green[3] = {0, 15, 0};
static byte _blue[3] = {0, 0, 15};
static byte _pink[3] = {15, 0, 15};

byte _currentRed;
byte _currentGreen;
byte _currentBlue;

boolean _screenEnabled = true;
boolean _animationEnabled = false;

unsigned long _payload = 0;

const uint64_t Pipe = 0xE8E8F0F0E1LL;
const byte UniqueKey = 1;
const byte GlobalKey = 9;
const byte PayloadSize = 4; // sizeof(long) == 4 (32 bits)

void setup() {
  Serial.begin(9600);
  radio.begin();

  // the following statements improve transmission range
  radio.setPayloadSize(PayloadSize); // setting the payload size to the needed value
  radio.setDataRate(RF24_250KBPS); // reducing bandwidth

  radio.openReadingPipe(1, Pipe); // Open one of the 6 pipes for reception

  radio.startListening(); // begin to listen

  matrix.begin();

  _currentRed = _yellow[0];
  _currentGreen = _yellow[1];
  _currentBlue = _yellow[2];

  matrix.fillScreen(matrix.Color444(_currentRed, _currentGreen, _currentBlue));
}

void loop() {
  if (_screenEnabled && _animationEnabled) {
    fadeColor(_yellow);
    fadeColor(_green);
    fadeColor(_turquoise);
    fadeColor(_blue);
    fadeColor(_pink);
    fadeColor(_red);
  } else {
    checkForRadioSignals();
  }
}

void fadeColor(byte color[]) {
  if (_screenEnabled && _animationEnabled) {
    for (byte step = 0; step < 15; step++) {
      if (_animationEnabled) {
        // Red
        if (_currentRed > color[0]) {
          _currentRed--;
        } else if (_currentRed < color[0]) {
          _currentRed++;
        }

        // Green
        if (_currentGreen > color[1]) {
          _currentGreen--;
        } else if (_currentGreen < color[1]) {
          _currentGreen++;
        }

        // Blue
        if (_currentBlue > color[2]) {
          _currentBlue--;
        } else if (_currentBlue < color[2]) {
          _currentBlue++;
        }

        matrix.fillScreen(matrix.Color444(_currentRed, _currentGreen, _currentBlue));
        checkForRadioSignals();
        delay(100);
      } else {
        break;
      }
    }
  }
}

void checkForRadioSignals() {
  if (radio.available()) {
    bool isRadioDone = false;
    while (!isRadioDone) {
      isRadioDone = radio.read(&_payload, sizeof(_payload));
    }

    processPayload();
  }
}

void processPayload() {
  Serial.println(_payload);
  String receivedPayloadString = String(_payload);
  byte receivedPayloadSize = receivedPayloadString.length();
  byte payloadTargetKey = receivedPayloadString.substring(0, 1).toInt();

  if (payloadTargetKey == UniqueKey || payloadTargetKey == GlobalKey) {
    if (receivedPayloadSize == 7) {
      // Custom color RGB
      byte receivedRed = receivedPayloadString.substring(1, 3).toInt();
      byte receivedGreen = receivedPayloadString.substring(3, 5).toInt();
      byte receivedBlue = receivedPayloadString.substring(5, 7).toInt();

      matrix.fillScreen(matrix.Color444(receivedRed, receivedGreen, receivedBlue));

      _animationEnabled = false;
      _screenEnabled = true;
    } else if (receivedPayloadSize == 3) {
      byte command = receivedPayloadString.substring(1, 3).toInt();
      switch (command) {
        case 10:
          // 10 OFF
          disableScreen();
          break;
        case 20:
          // 20 ON
          _screenEnabled = true;
          _animationEnabled = false;
          delay(100);
          matrix.fillScreen(matrix.Color444(15, 15, 0));
          break;
        case 30:
          // 30 ON & ANIMATE
          _screenEnabled = true;
          _animationEnabled = true;
          break;
      }
    }
  }
}

void disableScreen()  {
  if (_screenEnabled) {
    _screenEnabled = false;
    matrix.fillScreen(0); // Equivalent to matrix.clear()
  }
}


