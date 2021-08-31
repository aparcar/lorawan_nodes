#include "Arduino.h".
#include "LoRaWan_APP.h"
#include "config.h"
#include <CayenneLPP.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <SPI.h>
#include <Wire.h>
#include <softSerial.h>
#include "MB7389.h"
#include "VH400.h"

// This section allows to store data on the device. Specifically this allows to
// store the rain gauge *mm per tip* value.
#define ROW 0
#define ROW_OFFSET 100
// CY_FLASH_SIZEOF_ROW is 256 , CY_SFLASH_USERBASE is 0x0ffff400
#define addr_rain_gauge                                                        \
  CY_SFLASH_USERBASE + CY_FLASH_SIZEOF_ROW *ROW + ROW_OFFSET

CayenneLPP lpp(LORAWAN_APP_DATA_MAX_SIZE);

// Set to BATTERY_SEND_INTERVAL to send on start
uint32_t battery_send_counter = BATTERY_SEND_INTERVAL;

// Run cycle every minute
uint32_t appTxDutyCycle = 60000;

// Send messages on specific port. For this application all data is transmitted
// on port 2. The port is used to distinguish between packet types, however
// since this implementation uses CayenneLPP which is parsed by the backend, all
// data can be sent over the same port.
uint8_t appPort = 2;

// Initialize variables required for rain gauge use
#ifdef RAIN_GAUGE_PIN
// union to store directly on EEPROM
union {
  float number;
  uint8_t bytes[4];
} mm_per_tip;

// The tip counter and mm, must reset after each cycle
static volatile uint8_t tips = 0;
static unsigned long last_switch;
#endif

// Initialize variables required for OneWire temperature
//
// The current setup only supports a single connected sensor while it is easily
// possible to monitor multiple OneWire sensors at the same time.
#ifdef ONEWIRE_PIN
OneWire onewire(ONEWIRE_PIN);
DallasTemperature onewire_sensor(&onewire);
uint32_t onewire_send_counter = 0;
float onewire_total = 0.0;
#endif

// Initialize variables required for VH400 moisture sensor
#ifdef VH400_PIN
uint32_t vh400_send_counter = 0;
float vh400_total = 0.0;
#endif

// Initialize variables for MB7389 ultrasonic distance sensor
#ifdef SONIC_RX_PIN
uint32_t sonic_send_counter = 0;
uint32_t sonic_total = 0;
softSerial sonicSerial(0, SONIC_RX_PIN);
#endif

// Prepare the outgoing packet stored in CayenneLPP format
//
// The packet might be empty and sending is omitted for that cycle.
static void prepareTxFrame() {
  lpp.reset();

  // Send battery status only every "BATTERY_SEND_INTERVAL" minutes
  if (battery_send_counter >= BATTERY_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add battery volt\n", battery_send_counter,
                  BATTERY_SEND_INTERVAL);

    // Battery voltage is stored as voltage with ID 1
    lpp.addVoltage(1, getBatteryVoltage() / 1000.0);

    // Running version is stored as digital output with ID 1
    lpp.addDigitalOutput(1, VERSION);

    // Reset counter
    battery_send_counter = 0;
  } else {
    Serial.printf("[%i/%i] Skip battery volt\n", battery_send_counter,
                  BATTERY_SEND_INTERVAL);
    battery_send_counter++;
  }

#ifdef SONIC_RX_PIN
  sonic_total += get_sonic_distance(sonicSerial, 60);
  sonic_send_counter++;
  if (sonic_send_counter == SONIC_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add sonic\n", sonic_send_counter,
                  SONIC_SEND_INTERVAL);

    // Sea level is stored as distance with ID 1
    lpp.addDistance(1, ((float)sonic_total / sonic_send_counter) / 100.0);

    // Reset counters
    sonic_send_counter = 0;
    sonic_total = 0;
  } else {
    Serial.printf("[%i/%i] Skip sonic\n", sonic_send_counter,
                  SONIC_SEND_INTERVAL);
  }
#endif

#ifdef RAIN_GAUGE_PIN
  if (tips > 0) {
    Serial.printf("Tips = %i\n", tips);

    // Rain fall is stored as analog with ID 1
    // The CayenneLPP specification do not support rain fall directly
    lpp.addAnalogInput(1, mm_per_tip.number * tips);

    // Reset counter
    tips = 0;
  } else {
    Serial.printf("Skip rain gauge\n");
  }
#endif

#ifdef VH400_PIN
  vh400_total += read_VH400();
  vh400_send_counter++;
  if (vh400_send_counter >= VH400_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add VH400\n", vh400_send_counter,
                  VH400_SEND_INTERVAL);

    // Moisture is stored as analog with ID 2
    // The CayenneLPP specification do not support moisture directly
    lpp.addAnalogInput(2, vh400_total / vh400_send_counter);

    // Reset counters
    vh400_send_counter = 0;
    vh400_total = 0.0;
  } else {
    Serial.printf("[%i/%i] Skip VH400\n", vh400_send_counter,
                  VH400_SEND_INTERVAL);
  }
#endif

#ifdef ONEWIRE_PIN
  onewire_sensor.requestTemperatures();
  onewire_total += onewire_sensor.getTempCByIndex(0);
  onewire_send_counter++;
  if (onewire_send_counter >= ONEWIRE_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add OneWire\n", onewire_send_counter,
                  ONEWIRE_SEND_INTERVAL);

    // Temperature is stored as temperature with ID 1
    lpp.addTemperature(1, onewire_total / onewire_send_counter);

    // Reset counters
    onewire_send_counter = 0;
    onewire_total = 0.0;
  } else {
    Serial.printf("[%i/%i] Skip OneWire\n", onewire_send_counter,
                  ONEWIRE_SEND_INTERVAL);
  }
#endif

  // Copy CayenneLPP frame to appData, which will be send
  lpp.getBuffer(), appDataSize = lpp.getSize();
  memcpy(appData, lpp.getBuffer(), appDataSize);
}

// Custom commands to provision device
//
// At this point only the rain gauge command `mmPerTip` is supported, however it
// is easily possible to extend this to e.g. support height setting of the tide
// gauges or other user specific commands.
bool checkUserAt(char *cmd, char *content) {
#ifdef RAIN_GAUGE_PIN
  // This command is used to set the tipping bucket size
  // - HOBO RG3: 0.254mm/t
  // - Misol WH-SP-RG: 0.3851mm/t
  if (strcmp(cmd, "mmPerTip") == 0) {
    if (atof(content) != mm_per_tip.number) {
      mm_per_tip.number = atof(content);
      Serial.print("+OK Set mm per tip to ");
      Serial.print(mm_per_tip.number, 4);
      Serial.println("mm");
      Serial.println();
      FLASH_update(addr_rain_gauge, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
    } else {
      Serial.println("+OK Same mm per tip as before");
    }
    return true;
  }
#endif
  return false;
}

#ifdef RAIN_GAUGE_PIN
// Run on each interrupt aka tip of the tipping bucket
void interrupt_handler() {
  tips++;
  delay(250);
}
#endif

// The Arduino setup() function run on every boot
void setup() {
  // Set baudrate to 115200
  Serial.begin(115200);
  Serial.println("So it begins...");

#ifdef RAIN_GAUGE_PIN
  // Read mm/t from EEPROM
  Serial.println("Enable rain gauge sensor");
  FLASH_read_at(addr_rain_gauge, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
  if (mm_per_tip.number == 0.0) {
    mm_per_tip.number = DEFAULT_MM_PER_COUNT;
  }
  Serial.printf("Current mm per tip is ");
  Serial.print(mm_per_tip.number, 4);
  Serial.println("mm");

  // Enable interrupt pin for tips
  PINMODE_INPUT_PULLUP(RAIN_GAUGE_PIN);
  attachInterrupt(RAIN_GAUGE_PIN, interrupt_handler, FALLING);
#endif

#ifdef ONEWIRE_PIN
  // Setup OneWire and print single test measurement
  Serial.println("Enable OneWire sensor");
  onewire_sensor.begin();
  onewire_sensor.requestTemperatures();
  Serial.print("temperature = ");
  Serial.print(onewire_sensor.getTempCByIndex(0), 2);
  Serial.println();
#endif

  // Enable user input via Serial for AT commands
  enableAt();

#ifdef VH400_PIN
  // Setup VH400 sensor and print single test measurement
  Serial.println("Enable VH400 sensor");
  pinMode(VH400_PIN, INPUT);
  Serial.print("moisture = ");
  Serial.print(readVH400(VH400_PIN), 2) Serial.println();
#endif

#ifdef SONIC_RX_PIN
  // Print single test measurement
  Serial.println("Enable sonic sensor");
  Serial.print("distance = ");
  Serial.print(get_distance(sonicSerial, 1));
  Serial.println();
#endif

  // Set Arduino into the initialization state
  deviceState = DEVICE_STATE_INIT;

  LoRaWAN.ifskipjoin();
}

void loop() {
  switch (deviceState) {
  case DEVICE_STATE_INIT: {
    getDevParam();
    printDevParam();
    LoRaWAN.init(loraWanClass, loraWanRegion);
    deviceState = DEVICE_STATE_JOIN;
    break;
  }
  case DEVICE_STATE_JOIN: {
    LoRaWAN.join();
    break;
  }
  case DEVICE_STATE_SEND: {
    prepareTxFrame();

    // Do not send empty packages
    if (appDataSize > 0) {
      LoRaWAN.send();
    } else {
      Serial.println("Package is empty, don't send anything");
    }
    deviceState = DEVICE_STATE_CYCLE;
    break;
  }
  case DEVICE_STATE_CYCLE: {
// If sonic sensor is enabled it will measure 60 times a minute and return the
// average. This way waves don't influence tide estimations as much. Since a
// measurement is required every second the duty cycle is skipped and instead
// the get_distance function runs, which blocks for 60 seconds.
// If no sonic sensor is attached, use the
#ifdef SONIC_RX_PIN
    LoRaWAN.sleep();
    // Add random delay so sensors don't repetitively send at the same time.
    // This is done do avoid a situation where many sensors start at the same
    // time and collectively overload the gateway every 60 seconds.
    delay(randr(0, APP_TX_DUTYCYCLE_RND));
    deviceState = DEVICE_STATE_SEND;
#else
    // Same random delay is applied here.
    txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
    LoRaWAN.cycle(txDutyCycleTime);
    deviceState = DEVICE_STATE_SLEEP;
#endif
    break;
  }
  case DEVICE_STATE_SLEEP: {
    LoRaWAN.sleep();
    break;
  }
  default: {
    deviceState = DEVICE_STATE_INIT;
    break;
  }
  }
}
