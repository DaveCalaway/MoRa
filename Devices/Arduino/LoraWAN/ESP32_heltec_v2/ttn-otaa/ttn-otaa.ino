/*******************************************************************************
   Edited for ESP32 Heltec LoRa v2 + Debug over Oled
   Remeber to select the specific board from the Board Manager!
   DaveCalaway
  
  Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman

  Permission is hereby granted, free of charge, to anyone
  obtaining a copy of this document and accompanying files,
  to do whatever they want with them without any restriction,
  including, but not limited to, copying, modification and redistribution.
  NO WARRANTY OF ANY KIND IS PROVIDED.

  This example sends a valid LoRaWAN packet with payload "Hello,
  world!", using frequency and encryption settings matching those of
  the The Things Network.

  This uses OTAA (Over-the-air activation), where where a DevEUI and
  application key is configured, which are used in an over-the-air
  activation procedure where a DevAddr and session keys are
  assigned/generated for use with all further communication.

  Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
  g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
  violated by this sketch when left running for longer)!

  To use this sketch, first register your application and device with
  the things network, to set or generate an AppEUI, DevEUI and AppKey.
  Multiple devices can use the same AppEUI, but each device has its own
  DevEUI and AppKey.

  Do not forget to define the radio type correctly in config.h.
  For the WiF LoRa 32(V2):
   .../Documents/Arduino/libraries/arduino-lmic-master/src/lmic/config.h --> #define CFG_sx1276_radio 1


*******************************************************************************/

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <U8x8lib.h>

#define DEBUG_OLED // Uncomment to Enable Oled debug

// ==== OLED / SERIAL DEBUG ====
#ifdef DEBUG_OLED
#define DEBUG_PRINT(x)     oledPrint (x)
#define DEBUG_PRINTLN(x)     oledPrintln (x)
#else
#define DEBUG_PRINT(x)     Serial.print (x)
#define DEBUG_PRINTLN(x)     Serial.println (x)
#endif

// Oled definition
U8X8_SSD1306_128X64_NONAME_SW_I2C display(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
byte row = 0;

// This EUI must be in little-endian format, so least-significant-byte
// first. When copying an EUI from ttnctl output, this means to reverse
// the bytes. For TTN issued EUIs the last bytes should be 0xD5, 0xB3,
// 0x70.
static const u1_t PROGMEM APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getArtEui (u1_t* buf) {
  memcpy_P(buf, APPEUI, 8);
}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getDevEui (u1_t* buf) {
  memcpy_P(buf, DEVEUI, 8);
}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
// The key shown here is the semtech default key.
static const u1_t PROGMEM APPKEY[16] = { 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
void os_getDevKey (u1_t* buf) {
  memcpy_P(buf, APPKEY, 16);
}

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Pin mapping LoRa
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = { /*dio0*/ 26, /*dio1*/ 35, /*dio2*/ 34}
};


// === OLED PRINT ===
void oledPrint (const char* s) {
  display.drawString(0, row, s);
}


// === OLED PRINTLN ===
void oledPrintln (const char* s) {
  display.drawString(0, row, s);
  if (row >= 8) {
    row = 0;
    display.clearDisplay();
  }
  else row += 1;
}


void onEvent (ev_t ev) {
  char osTime = os_getTime();
  DEBUG_PRINT(&osTime);
  DEBUG_PRINT(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      DEBUG_PRINTLN("EV_SCAN_TIMEOUT");
      break;
    case EV_BEACON_FOUND:
      DEBUG_PRINTLN("EV_BEACON_FOUND");
      break;
    case EV_BEACON_MISSED:
      DEBUG_PRINTLN("EV_BEACON_MISSED");
      break;
    case EV_BEACON_TRACKED:
      DEBUG_PRINTLN("EV_BEACON_TRACKED");
      break;
    case EV_JOINING:
      DEBUG_PRINTLN("EV_JOINING");
      break;
    case EV_JOINED:
      DEBUG_PRINTLN("EV_JOINED");

      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      break;
    case EV_RFU1:
      DEBUG_PRINTLN("EV_RFU1");
      break;
    case EV_JOIN_FAILED:
      DEBUG_PRINTLN("EV_JOIN_FAILED");
      break;
    case EV_REJOIN_FAILED:
      DEBUG_PRINTLN("EV_REJOIN_FAILED");
      break;
      break;
    case EV_TXCOMPLETE:
      DEBUG_PRINTLN("EV_TXCOMPLETE (includes waiting for RX windows)");
      if (LMIC.txrxFlags & TXRX_ACK)
        DEBUG_PRINTLN("Received ack");
      if (LMIC.dataLen) {
        DEBUG_PRINTLN("Received ");
        char dataLen = LMIC.dataLen;
        DEBUG_PRINTLN(&dataLen);
        DEBUG_PRINTLN(" bytes of payload");
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      DEBUG_PRINTLN("EV_LOST_TSYNC");
      break;
    case EV_RESET:
      DEBUG_PRINTLN("EV_RESET");
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      DEBUG_PRINTLN("EV_RXCOMPLETE");
      break;
    case EV_LINK_DEAD:
      DEBUG_PRINTLN("EV_LINK_DEAD");
      break;
    case EV_LINK_ALIVE:
      DEBUG_PRINTLN("EV_LINK_ALIVE");
      break;
    default:
      DEBUG_PRINTLN("Unknown event");
      break;
  }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    DEBUG_PRINTLN("OP_TXRXPEND, not sending");
  } else {
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, mydata, sizeof(mydata) - 1, 0);
    DEBUG_PRINTLN("Packet queued");
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  Serial.begin(115200);
  display.begin();
  display.setFont(u8x8_font_chroma48medium8_r);
  DEBUG_PRINTLN("Starting");

#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
