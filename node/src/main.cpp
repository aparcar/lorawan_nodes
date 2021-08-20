#include "Arduino.h"
#include "LoRaWan_APP.h"
#include "common.h"
#include <CayenneLPP.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "VH400.h"

#define VERSION 3

#define ROW 0
#define ROW_OFFSET 100
//CY_FLASH_SIZEOF_ROW is 256 , CY_SFLASH_USERBASE is 0x0ffff400
#define addr CY_SFLASH_USERBASE + CY_FLASH_SIZEOF_ROW* ROW + ROW_OFFSET


#ifdef ONE_WIRE_PIN
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
#endif

#ifdef SHT1x_DATA_PIN
#include <SHT1x.h>
// sht1x(data, clock)
SHT1x sht1x(SHT1x_DATA_PIN, SHT1x_CLOCK_PIN);
//SHT1x sht2x(GPIO2, GPIO1);
#endif

uint8_t battery_send_counter = 0;
uint8_t temperature_send_counter = 0;

// send every minute
uint32_t appTxDutyCycle = 60000;

// use port 2
uint8_t appPort = 2;

// union to store directly on EEPROM
union {
    float number;
    uint8_t bytes[4];
} mm_per_tip;

// the tip counter and mm, reset after each cycle
static volatile uint8_t tips = 0;

static unsigned long last_switch;

// prepare the outgoing package
static void prepareTxFrame(uint8_t port)
{
    // contains the LoRa frame
    CayenneLPP lpp(LORAWAN_APP_DATA_MAX_SIZE);

    // send battery status only every "BATTERY_SEND_INTERVAL_MINUTES" minutes
    if (battery_send_counter <= 0) {
	lpp.addVoltage(1, getBatteryVoltage() / 1000.0);
	battery_send_counter = BATTERY_SEND_INTERVAL_MINUTES;

	lpp.addDigitalOutput(1, VERSION);
    } else {
	Serial.println("Skip adding batter volt");
	battery_send_counter--;
    }

#ifdef RAIN_GAUGE_PIN
    if (tips > 0) {
	Serial.printf("Tips = %i\n", tips);
	lpp.addAnalogInput(1, mm_per_tip.number * tips);
	tips = 0;
    } else {
	Serial.println("Skip adding rain fall");
    }
#endif

    // send temperature status only every "TEMPERATURE_SEND_INTERVAL_MINUTES" minutes
    if (temperature_send_counter <= 0) {
#ifdef SHT1x_DATA_PIN
	float tempC = sht1x.readTemperatureC();
	lpp.addTemperature(2, tempC);
	lpp.addRelativeHumidity(2, sht1x.readHumidity());
	//lpp.addTemperature(3, sht2x.readTemperatureC());
	//lpp.addRelativeHumidity(3, sht2x.readHumidity());
#endif

#ifdef ONE_WIRE_enalbed
	sensors.requestTemperatures();
	lpp.addTemperature(1, sensors.getTempCByIndex(0));
#endif

#ifdef VH400_PIN
	lpp.addAnalogInput(2, readVH400(VH400_PIN));
#endif

	temperature_send_counter = TEMPERATURE_SEND_INTERVAL_MINUTES;
    } else {
	Serial.println("Skip adding temperature");
	temperature_send_counter--;
    }

    appDataSize = lpp.getSize();
    Serial.printf("appDataSize = %i\n", appDataSize);
    lpp.getBuffer(), memcpy(appData, lpp.getBuffer(), appDataSize);
}

// custom commands to provision device
bool checkUserAt(char* cmd, char* content)
{
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
    return false;
}

// run on each interrupt aka tip of the seesaw
void interrupt_handler()
{
	tips++;
	//Serial.print(mm_per_tip.number * tips);
	//Serial.println("mm");
	delay(250);
}

void setup()
{
    Serial.begin(115200);

    // reach mm per tip from EEPROM
    FLASH_read_at(addr, mm_per_tip.bytes, sizeof(mm_per_tip.bytes));
    if (mm_per_tip.number == 0.0) {
	mm_per_tip.number = DEFAULT_MM_PER_COUNT;
    }
    Serial.printf("Current mm per tip is ");
    Serial.print(mm_per_tip.number, 4);
    Serial.println("mm");

#ifdef ONE_WIRE_PIN
    sensors.begin();
    sensors.requestTemperatures();
    Serial.print("Temp = ");
    Serial.print(sensors.getTempCByIndex(0), 2);
    Serial.println();
#endif

    // enable user input via Serial
    enableAt();
    deviceState = DEVICE_STATE_INIT;

#ifdef VH400_PIN
    pinMode(VH400_PIN, INPUT);
#endif

#ifdef RAIN_GAUGE_PIN
    // enable interrupt pin
    PINMODE_INPUT_PULLUP(RAIN_GAUGE_PIN);
    attachInterrupt(RAIN_GAUGE_PIN, interrupt_handler, FALLING);
#endif

    LoRaWAN.ifskipjoin();
}

void loop()
{
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
	txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
	LoRaWAN.cycle(txDutyCycleTime);
	deviceState = DEVICE_STATE_SLEEP;
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
