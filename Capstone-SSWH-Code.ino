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
#define PIN_RELAY_PUMP 26
#define PUN_RELAY_ACUTATOR 24



// RTC SETUP
bool RTC_SYNCED = false;
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
#define gmtOffset_sec (-6 * 3600)
#define daylightOffset_sec 3600

// OneWire Setup
#define ONEWIRE_MAX_DEVICES = 5;
// OneWire32 onewire(PIN_TEMP_ONEWIRE);
// Temp sensor OneWire indices (placeholder)
#define TEMP_INPUT 0
#define TEMP_COLLECTOR 1
#define TEMP_TANK 2
#define TEMP_AIR 3



void setup() {
  if( DEBUG ) {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n[STARTING].................................");
    Serial.printf("[WIFI] Connecting to '%s' with password '%s'\n", WIFI_SSID, WIFI_PASS);
  }
  // Manual wifi, later let matter setup wifi and use it when available.
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  wifi_connected_prev = false;

  // list onewire addresses
  // Serial.println("[OneWire] Addresses:");
  // uint64_t ow_addr[ONEWIRE_MAX_DEVICES];
  // uint8_t ow_num_devs = onewire.search(addr, ONEWIRE_MAX_DEVICES);
	// for (uint8_t i = 0; i < ow_num_devs; i += 1) {
	// 	Serial.printf("%d: 0x%llx,\n", i, addr[i]);
  // }

  // init matter
}

void loop() {
  delay(2000);
  if( DEBUG ) {
    Serial.println();
    printLocalTime();
  }

  bool wifi_connected = checkWifiStatus();

  // If we have a WiFi connection AND the RTC has not already been updated, then try to update the RTC
  if( !RTC_SYNCED && wifi_connected ) {
    Serial.println("Starting RTC sync.");
    rtc_sync();
  }
  

  // Temp sensor data
  int temp_input;
  int temp_collector;
  int temp_tank;
  int temp_air;

	// read temp sensors
  // random data until sensors connected
  temp_input = random(15, 19);
  temp_collector = random(24, 29);
  temp_tank = random(35, 39);
  temp_air = random(15, 25);


  // send sensor data over matter
  if( DEBUG ) {
    Serial.printf("Temp Input:    %d C\n", temp_input);
    Serial.printf("Temp Collect:  %d C\n", temp_collector);
    Serial.printf("Temp Tank:     %d C\n", temp_tank);
    Serial.printf("Temp Air:      %d C\n", temp_air);
  }
  return;

  // enable/disable pump if needed (check time since last toggle)


  // read angle from inclinometer

  // read or calculate solar noon for today if not already found.

  // calculate offset from solar noon and desired angle

  // set acutator to correct position based on desired angle and current angle
  
}

bool checkWifiStatus() {
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


void printLocalTime() {
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
  sntp_set_time_sync_notification_cb(rtc_sync_callback); // set callback func to update RTC_SYNCED and print to debug.
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

void rtc_sync_callback(struct timeval *t) {
  if( DEBUG ) {
    Serial.print("Got time adjustment from NTP, time is: ");
    printLocalTime();
  }
  RTC_SYNCED = true;
}
