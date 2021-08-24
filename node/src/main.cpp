#include "Arduino.h"
#include "LoRaWan_APP.h"
#include "MB7389.h"
#include "VH400.h"
#include "common.h"
#include <CayenneLPP.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <softSerial.h>

#define ROW 0
#define ROW_OFFSET 100
// CY_FLASH_SIZEOF_ROW is 256 , CY_SFLASH_USERBASE is 0x0ffff400
#define addr CY_SFLASH_USERBASE + CY_FLASH_SIZEOF_ROW *ROW + ROW_OFFSET

// set to BATTERY_SEND_INTERVAL to send on start
uint32_t battery_send_counter = BATTERY_SEND_INTERVAL;

// send every minute
uint32_t appTxDutyCycle = 60000;

// use port 2
uint8_t appPort = 2;

#ifdef RAIN_GAUGE_PIN
// union to store directly on EEPROM
union {
  float number;
  uint8_t bytes[4];
} mm_per_tip;

// the tip counter and mm, reset after each cycle
static volatile uint8_t tips = 0;
static unsigned long last_switch;
#endif

#ifdef ONEWIRE_PIN
OneWire onewire(ONEWIRE_PIN);
DallasTemperature onewire_sensor(&onewire);
uint32_t onewire_send_counter = 0;
float onewire_total = 0.0;
#endif

#ifdef VH400_PIN
uint32_t vh400_send_counter = 0;
float vh400_total = 0.0;
#endif

#ifdef SONAR_RX_PIN
uint32_t sonar_send_counter = 0;
uint32_t sonar_total = 0;
softSerial sonarSerial(0, SONAR_RX_PIN);
#endif

// prepare the outgoing package
static void prepareTxFrame(uint8_t port) {
  // contains the LoRa frame
  CayenneLPP lpp(LORAWAN_APP_DATA_MAX_SIZE);

  // send battery status only every "BATTERY_SEND_INTERVAL" minutes
  if (battery_send_counter >= BATTERY_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add battery volt\n", battery_send_counter,
                  BATTERY_SEND_INTERVAL);
    lpp.addVoltage(1, getBatteryVoltage() / 1000.0);
    battery_send_counter = 0;

    lpp.addDigitalOutput(1, VERSION);
  } else {
    Serial.printf("[%i/%i] Skip battery volt\n", battery_send_counter,
                  BATTERY_SEND_INTERVAL);
    battery_send_counter++;
  }

#ifdef SONAR_RX_PIN
  sonar_total += get_distance(sonarSerial);
  sonar_send_counter++;
  if (sonar_send_counter == SONAR_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add sonar\n", sonar_send_counter,
                  SONAR_SEND_INTERVAL);
    lpp.addDistance(1, ((float)sonar_total / sonar_send_counter) / 100.0);
    sonar_send_counter = 0;
    sonar_total = 0;
  } else {
    Serial.printf("[%i/%i] Skip sonar\n", sonar_send_counter,
                  SONAR_SEND_INTERVAL);
  }
#endif

#ifdef RAIN_GAUGE_PIN
  if (tips > 0) {
    Serial.printf("Tips = %i\n", tips);
    lpp.addAnalogInput(1, mm_per_tip.number * tips);
    tips = 0;
  } else {
    Serial.printf("Skip rain gauge\n");
  }
#endif

#ifdef VH400_PIN
  vh400_total += readVH400(VH400_PIN);
  vh400_send_counter++;
  if (vh400_send_counter >= VH400_SEND_INTERVAL) {
    Serial.printf("[%i/%i] Add VH400\n", vh400_send_counter,
                  VH400_SEND_INTERVAL);
    lpp.addAnalogInput(2, vh400_total / vh400_send_counter);
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
    lpp.addTemperature(1, onewire_total / onewire_send_counter);
    onewire_send_counter = 0;
    onewire_total = 0.0;
  } else {
    Serial.printf("[%i/%i] Skip OneWire\n", onewire_send_counter,
                  ONEWIRE_SEND_INTERVAL);
  }
#endif

  appDataSize = lpp.getSize();
  Serial.printf("appDataSize = %i\n", appDataSize);
  lpp.getBuffer(), memcpy(appData, lpp.getBuffer(), appDataSize);
}

// custom commands to provision device
bool checkUserAt(char *cmd, char *content) {
#ifdef RAIN_GAUGE_PIN
  // this command is used to set the seesaw size
  if (strcmp(cmd, "mmPerTip") == 0) {
    if (atof(content) != mm_per_tip.number) {
      mm_per_tip.number = atof(content);
      Serial.print("+OK Set mm per tip to ");
      Serial.print(mm_per_tip.number, 4);
      Serial.println("mm");
      Serial.println();
      FLASH_update(addr, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
    } else {
      Serial.println("+OK Same mm per tip as before");
    }
    return true;
  }
#endif
  return false;
}

#ifdef RAIN_GAUGE_PIN
// run on each interrupt aka tip of the seesaw
void interrupt_handler() {
  tips++;
  delay(250);
}
#endif

void setup() {
  Serial.begin(115200);
  Serial.println("So it begins...");

#ifdef RAIN_GAUGE_PIN
  // reach mm per tip from EEPROM
  Serial.println("Enable rain gauge sensor");
  FLASH_read_at(addr, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
  if (mm_per_tip.number == 0.0) {
    mm_per_tip.number = DEFAULT_MM_PER_COUNT;
  }
  Serial.printf("Current mm per tip is ");
  Serial.print(mm_per_tip.number, 4);
  Serial.println("mm");

  // enable interrupt pin
  PINMODE_INPUT_PULLUP(RAIN_GAUGE_PIN);
  attachInterrupt(RAIN_GAUGE_PIN, interrupt_handler, FALLING);
#endif

#ifdef ONEWIRE_PIN
  Serial.println("Enable OneWire sensor");
  onewire_sensor.begin();
  onewire_sensor.requestTemperatures();
  Serial.print("temperature = ");
  Serial.print(onewire_sensor.getTempCByIndex(0), 2);
  Serial.println();
#endif

  // enable user input via Serial
  enableAt();
  deviceState = DEVICE_STATE_INIT;

#ifdef VH400_PIN
  Serial.println("Enable VH400 sensor");
  pinMode(VH400_PIN, INPUT);
#endif

#ifdef SONAR_RX_PIN
  Serial.println("Enable sonar sensor");
#endif

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
    prepareTxFrame(appPort);

    // don't send empty packages
    if (appDataSize > 0) {
      LoRaWAN.send();
    } else {
      Serial.println("Package is empty, don't send anything");
    }
    deviceState = DEVICE_STATE_CYCLE;
    break;
  }
  case DEVICE_STATE_CYCLE: {
#ifdef SONAR_RX_PIN
    LoRaWAN.sleep();
    deviceState = DEVICE_STATE_SEND;
#else
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
