#include <WiFi.h> // for checking if WiFi is connected to update RTC
#include "time.h" 
#include "esp_sntp.h"

#include "OneWireESP32.h" // OneWire library

#define DEBUG true // Enable debug output to serial port


// Wifi Setup
#include "env.h" // defines WIFI_SSID and WIFI_PASS 
bool wifi_connected_prev = false;

// INPUT PINS
#define PIN_TEMP_ONEWIRE 4
#define PIN_INCLINOMETER 6

// OUTPUT PINS 
#define PIN_PUMP 31
#define PIN_ARM_ENABLE 26
#define PIN_ARM_EXTEND 25
#define PIN_ARM_RETRACT 24


// RTC SETUP
bool RTC_SYNCED = false;
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
#define gmtOffset_sec (-6 * 3600)
#define daylightOffset_sec 3600

// OneWire Setup
#define ONEWIRE_MAX_DEVICES 5

OneWire32 onewire(PIN_TEMP_ONEWIRE);
uint8_t onewire_num_devices = 0;
uint64_t onewire_active_addrs[ONEWIRE_MAX_DEVICES];
const char *ONEWIRE_ERROR_TYPES[] = {"", "CRC", "BAD","DC","DRV"};

// Temp sensor OneWire addresses

#define TEMP_INPUT 0x400000005e3c8428
#define TEMP_COLLECTOR 0xd60000005d671928
#define TEMP_TANK 0
#define TEMP_AIR 0

float temp_measured[ONEWIRE_MAX_DEVICES];



void setup() {
  if( DEBUG ) {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n\n[STARTUP].............................................");
    Serial.printf("[WIFI] Connecting to '%s' with password '%s'\n", WIFI_SSID, WIFI_PASS);
  }
  // Manual wifi, later let matter setup wifi and use it when available.
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  wifi_connected_prev = false;

  if( DEBUG ) {
    Serial.println("setup() ended");
  }

  // init matter
}

void loop() {
  delay(2000);
  if( DEBUG ) {
    Serial.println();
    time_println();
  }

  bool wifi_connected = wifi_check_status();

  // If we have a WiFi connection AND the RTC has not already been updated, then try to update the RTC
  if( !RTC_SYNCED && wifi_connected ) {
    rtc_sync();
  }
  


  if( onewire_num_devices <= 0 ) { // Find onewire devices if none are detected.
    temp_setup_onewire(); // only run this once to avoid rearranging address indices
  }
  // list onewire addresses for identifying sensors
  // temp_print_onewire_addrs();

	// read temp sensors
  temp_read_sensors();

  // send sensor data over matter
  if( DEBUG ) {
    Serial.printf("Temp Input:    %11.6f C\n", temp_get_by_addr(TEMP_INPUT) );
    Serial.printf("Temp Collect:  %11.6f C\n", temp_get_by_addr(TEMP_COLLECTOR) );
    Serial.printf("Temp Tank:     %11.6f C\n", temp_get_by_addr(TEMP_TANK) );
    Serial.printf("Temp Air:      %11.6f C\n", temp_get_by_addr(TEMP_AIR) );
  }
  // enable/disable pump if needed (check time since last toggle)


  // read angle from inclinometer

  // read or calculate solar noon for today if not already found.

  // calculate offset from solar noon and desired angle

  // set acutator to correct position based on desired angle and current angle
  
}


// WIFI Functions
bool wifi_check_status() {
  bool wifi_connected_current = WiFi.status() == WL_CONNECTED;

  if( wifi_connected_prev != wifi_connected_current) { // If wifi connected status changes, then log it
    if( wifi_connected_current ) {
      Serial.println("[WIFI] Connected");
    }
    else {
      Serial.println("[WIFI] Disconnected");
    }
  }
  wifi_connected_prev = wifi_connected_current;

  return wifi_connected_current;
}


// Time Functions
void time_println() {
  if( DEBUG ) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("No time available (yet)");
      return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
}

void rtc_sync() {
  if( DEBUG ) {
    Serial.println("[RTC] Starting RTC sync.");
  }
  sntp_set_time_sync_notification_cb(rtc_sync_callback); // set callback func to update RTC_SYNCED and print to debug.
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

void rtc_sync_callback(struct timeval *t) {
  if( DEBUG ) {
    Serial.print("Got time adjustment from NTP, time is: ");
    time_println();
  }
  RTC_SYNCED = true;
}


// TEMP SENSOR FUNCTIONS
void temp_setup_onewire() {
  onewire_num_devices = onewire.search(onewire_active_addrs, ONEWIRE_MAX_DEVICES);
}

void temp_print_onewire_addrs() {
  Serial.println("[OneWire] Addresses:");
	for (uint8_t i = 0; i < onewire_num_devices; i += 1) {
		Serial.printf("%d: 0x%llx,\n", i, onewire_active_addrs[i]);
  }
}

void temp_read_sensors() {
  onewire.request();
  delay(750);

  for( uint8_t i = 0; i < onewire_num_devices; i++ ) {
    uint8_t error = onewire.getTemp(onewire_active_addrs[i], temp_measured[i]);
    if( error ) {
      Serial.printf("[OneWire] ERROR: 0x%llx errored: %s\n", onewire_active_addrs[i], ONEWIRE_ERROR_TYPES[error]);
    }
    else {
      Serial.printf("[OneWire] DATA: 0x%llx : %f\n", onewire_active_addrs[i], temp_measured[i]);
    }
  }
}


float temp_get_by_addr( uint64_t addr ) {
  for( uint8_t i = 0; i < onewire_num_devices; i++ ) { // loop through all active addresses
    if( onewire_active_addrs[i] == addr ) { // when current addr matches given addr, return measured value at this index.
      return temp_measured[i];
    }
  }
  return -100.0; // if address is not found, return temp as -100 C
}




