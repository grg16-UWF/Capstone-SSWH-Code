#include <WiFi.h> // for checking if WiFi is connected to update RTC
#include "time.h" 
#include "esp_sntp.h"

#include "OneWireESP32.h" // OneWire library

#include <Adafruit_MPU6050.h> // gyro library
#include <Adafruit_Sensor.h>

#include <Matter.h>

#include "SolarNoon/solar_noon.h"

#define DEBUG true // Enable debug output to serial port
#define DebugLog if(DEBUG) Serial

// Wifi Setup
#include "env.h" // defines WIFI_SSID and WIFI_PASS 
bool wifi_connected_prev = false;

// INPUT PINS
#define PIN_TEMP_ONEWIRE 19 // GIOP 19

// OUTPUT PINS (GIOP #)
#define PIN_PUMP 18
#define PIN_ARM_ENABLE 17
#define PIN_ARM_EXTEND 4
#define PIN_ARM_RETRACT 16


// RTC SETUP
bool RTC_SYNCED = false;
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
#define gmtOffset_sec (-6 * 3600)
#define daylightOffset_sec 3600

// OneWire Setup
#define ONEWIRE_MAX_DEVICES 5
int setup_attempts = 2; // how many times to check for OneWire Devices.

OneWire32 onewire(PIN_TEMP_ONEWIRE);
uint8_t onewire_num_devices = 0;
uint64_t onewire_active_addrs[ONEWIRE_MAX_DEVICES];
const char *ONEWIRE_ERROR_TYPES[] = {"", "CRC", "BAD","DC","DRV"};

// Temp sensor OneWire addresses
#define TEMP_INPUT 0x400000005e3c8428
#define TEMP_COLLECTOR 0xd60000005d671928
#define TEMP_TANK 0x850000005d35e828
#define TEMP_AIR 0x990000005d00e528

float temp_measured[ONEWIRE_MAX_DEVICES];

// Gyro setup
Adafruit_MPU6050 mpu;
#define MPU_ACCEL_RANGE MPU6050_RANGE_4_G // Options: 2_G, 4_G, 8_G, 16_G
#define MPU_GYRO_RANGE MPU6050_RANGE_500_DEG // Options (deg/sec): 250_DEG, 500_DEG, 1000_DEG, 2000_DEG   NOT USED IN THIS PROJECT
#define MPU_FILTER_BANDWIDTH MPU6050_BAND_5_HZ

const float ACCEL_OFFSET[] = {0.141147, 0.147745, 0.643307}; // { X, Y, Z }
float accel_raw_iteravg[] = {0, 0, 0}; // store an iterative average of accel measurements
uint16_t mpu_num_samples = 0;

// Target Angle limits
#define ARM_ENABLED true // allow disabling the arm for testing
#define ARM_ANGLE_MAX 40 // maximum angle for solar tracking (facing west)
#define ARM_ANGLE_MIN -40 // minimum angle for solar tracking (facing east)
#define ARM_ANGLE_THRESHOLD 5 // how far from the target the current angle can be before moving.
#define ARM_MOVE_POLLING_PERIOD 100 // (ms) how often the arm should check angle while moving

// PUMP SETUP
#define PUMP_DUTY_ACTIVE 1    // how many minutes the pump is active per cycle.
#define PUMP_DUTY_INACTIVE 2  // how many minutes the pump is inactive per cycle.

// time thresholds
#define TIME_SUNRISE 330   // (5:30AM) what time the pre-sunrise tasks ocurr. (pump enable)
#define TIME_SUNSET 1230 // (8:30PM) what time the post-sunset tasks ocurr. (pump disable, arm reset)

// PUMP state
bool pump_active = 0;       // 1: pump is active, 0: pump is inactive
int pump_next_active = 0;   // time when the pump should turn on
int pump_next_inactive = 0; // time when the pump should turn off
bool pump_suspended_night = 1; // whether the pump is currently kept off for nighttime

// Matter endpoint setup
MatterTemperatureSensor matter_temp_input; // temp sensor 1
MatterTemperatureSensor matter_temp_collector; // temp sensor 2
MatterTemperatureSensor matter_temp_tank; // temp sensor 3
MatterTemperatureSensor matter_temp_air; // temp sensor 4
MatterContactSensor matter_pump_active; // Contact Sensor 1

void setup() {
  if( DEBUG ) {
    Serial.begin(115200);
    Serial.println("");
    delay(500);
    Serial.println("\n\n[STARTUP].............................................");
    Serial.printf("[WIFI] Connecting to '%s' with password '%s'\n", WIFI_SSID, WIFI_PASS);
  }

  // Manual wifi, later let matter setup wifi and use it when available.
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  wifi_connected_prev = false;

  // Setup MPU
  if( !mpu.begin() ) {
    DebugLog.println("[MPU] ERROR! Failed to init MPU");
  }
  mpu.setAccelerometerRange(MPU_ACCEL_RANGE);
  mpu.setGyroRange(MPU_GYRO_RANGE);
  mpu.setFilterBandwidth(MPU_FILTER_BANDWIDTH);
  DebugLog.println("[MPU] Setup complete");

  // Setup pump and arm pins
  pinMode(PIN_PUMP, OUTPUT);
  digitalWrite(PIN_PUMP, LOW);

  pinMode(PIN_ARM_ENABLE, OUTPUT);
  digitalWrite(PIN_ARM_ENABLE, LOW);

  pinMode(PIN_ARM_EXTEND, OUTPUT);
  digitalWrite(PIN_ARM_EXTEND, LOW);

  pinMode(PIN_ARM_RETRACT, OUTPUT);
  digitalWrite(PIN_ARM_RETRACT, LOW);

  // init matter endpoints
  matter_temp_input.begin();
  matter_temp_collector.begin();
  matter_temp_tank.begin();
  matter_temp_air.begin();
  matter_pump_active.begin();

  // init Matter
  Matter.begin();

  Matter.onEvent([](matterEvent_t event, const void *data) {
    if (event == MATTER_COMMISSIONING_COMPLETE) {
      Serial.println("[MATTER] Successfully commissioned!");
    }
  });

  if (!Matter.isDeviceCommissioned()) {
    Serial.printf("Manual pairing code: %s\n", Matter.getManualPairingCode().c_str());
  }

  DebugLog.println("setup() ended");
}

void loop() {
  DebugLog.println();
  time_println();


  bool wifi_connected = wifi_check_status();

  // If we have a WiFi connection AND the RTC has not already been updated, then try to update the RTC
  if( !RTC_SYNCED && wifi_connected ) {
    rtc_sync();
  }
  


  if( onewire_num_devices <= 0 && setup_attempts > 0 ) { // Find onewire devices if none are detected.
    temp_setup_onewire(); // only run this once to avoid rearranging address indices
  }
  // list onewire addresses for identifying sensors
  // temp_print_onewire_addrs();

	// read temp sensors
  temp_read_sensors();

  // send sensor data over matter
  DebugLog.printf("Temp Input:    %11.6f C\n", temp_get_by_addr(TEMP_INPUT) );
  DebugLog.printf("Temp Collect:  %11.6f C\n", temp_get_by_addr(TEMP_COLLECTOR) );
  DebugLog.printf("Temp Tank:     %11.6f C\n", temp_get_by_addr(TEMP_TANK) );
  DebugLog.printf("Temp Air:      %11.6f C\n", temp_get_by_addr(TEMP_AIR) );
  matter_update_temp_sensors();

  // control the arm.
  armController();

  // control the pump.
  pump_controller();
  
  delay(5000);
}


// WIFI Functions
bool wifi_check_status() {
  bool wifi_connected_current = WiFi.status() == WL_CONNECTED;

  if( wifi_connected_prev != wifi_connected_current) { // If wifi connected status changes, then log it
    if( wifi_connected_current ) {
      DebugLog.println("[WIFI] Connected");
    }
    else {
      DebugLog.println("[WIFI] Disconnected");
    }
  }
  wifi_connected_prev = wifi_connected_current;

  return wifi_connected_current;
}


// Time Functions
void time_println() {
  if( DEBUG ) { // do nothing if not in DEBUG mode.
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("No time available (yet)");
      return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
}

struct tm getLocalTime_no_dst() { // I need non-DST time to avoid having to calculate DST cutoff dates
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    DebugLog.println("[getLocalTime_no_dst] Time not available (yet)");
    return timeinfo;
  }
  
  if( timeinfo.tm_isdst != 0 ) { // if DST is active, remove an hour. 
    if( timeinfo.tm_hour == 0) { // handle day rollover. In the US, DST actiavtes on 2nd Sunday of March @ 2AM, so this *should* never trigger. For this reason I'm not including Month rollover.
      timeinfo.tm_hour = 23;
      timeinfo.tm_mday = timeinfo.tm_mday - 1;
    }
    timeinfo.tm_hour = timeinfo.tm_hour - 1;
  }
  return timeinfo;
}

int time_mins_into_day( struct tm time ) { // How many minutes into the day is this timestamp
  return (60 * time.tm_hour) + time.tm_min;
}

// Get the time as minutes since midnight
int get_time() { 
  return time_mins_into_day(getLocalTime_no_dst());
}


void rtc_sync() {
  DebugLog.println("[RTC] Starting RTC sync.");
  sntp_set_time_sync_notification_cb(rtc_sync_callback); // set callback func to update RTC_SYNCED and print to debug.
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
}

void rtc_sync_callback(struct timeval *t) {
  DebugLog.print("Got time adjustment from NTP, time is: ");
  time_println();
  RTC_SYNCED = true;
}


// TEMP SENSOR FUNCTIONS
void temp_setup_onewire() {
  setup_attempts--;
  onewire_num_devices = onewire.search(onewire_active_addrs, ONEWIRE_MAX_DEVICES);
  DebugLog.printf("[TEMP] onewire.search() found %d devices.\n", onewire_num_devices);
}

void temp_print_onewire_addrs() {
  if (DEBUG && onewire_num_devices > 0 ) { // do nothing if not in DEBUG mode or no devs connected
    Serial.println("[OneWire] Addresses:");
    for (uint8_t i = 0; i < onewire_num_devices; i += 1) {
      Serial.printf("%d: 0x%llx,\n", i, onewire_active_addrs[i]);
    }
  }
}

void temp_read_sensors() {
  if( onewire_num_devices <= 0 ) { // if no temp sensors connected, don't request data. (request sends error if no connected devs).
    return;
  }

  onewire.request();
  delay(750);

  for( uint8_t i = 0; i < onewire_num_devices; i++ ) {
    uint8_t error = onewire.getTemp(onewire_active_addrs[i], temp_measured[i]);
    if( error ) {
      DebugLog.printf("[OneWire] ERROR: 0x%llx errored: %s\n", onewire_active_addrs[i], ONEWIRE_ERROR_TYPES[error]);
      temp_measured[i] = -100.0;
    }
    else {
      // DebugLog.printf("[OneWire] DATA: 0x%llx : %f\n", onewire_active_addrs[i], temp_measured[i]);
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


// Arm "acutator" functions
int arm_get_target_angle() {
  // Calculate the difference between current time and solar noon
  struct tm curr_time = getLocalTime_no_dst();
  int curr_mins = time_mins_into_day(curr_time);
  int day_of_year = curr_time.tm_yday;
  int solar_noon = SOLAR_NOON_MINUTES[day_of_year];
  int mins_diff = curr_mins - solar_noon;
  
  // Calculate the target angle from the time difference.
  int angle = mins_diff / 4;
  // DebugLog.printf("[TargetAngle] time=%d, noon[%d]=%d, raw_angle=%d\n", curr_mins, day_of_year, solar_noon, angle);

  // after sunset timeout, reset arm to morning position.
  if( curr_mins > TIME_SUNSET || curr_mins < TIME_SUNRISE ) {
    angle = ARM_ANGLE_MIN;
    return angle;
  }

  // Clamp the target angle
  if( angle < ARM_ANGLE_MIN ) {
    angle = ARM_ANGLE_MIN;
  }
  else if( angle > ARM_ANGLE_MAX ) {
    angle = ARM_ANGLE_MAX;
  }
  
  return angle;
}

bool arm_check_move_needed(int current_angle, int target_angle) { // how many degrees should the arm move to reach target
  int diff = target_angle - current_angle;

  if( diff < ARM_ANGLE_THRESHOLD && diff > -ARM_ANGLE_THRESHOLD ) { // if diff is less than threshold, dont need move
    return false;
  }
  return true;
}

void arm_move( const int target_angle ) {
  
  // prep pins for arm movement
  digitalWrite(PIN_ARM_RETRACT, LOW);
  digitalWrite(PIN_ARM_EXTEND, LOW);
  digitalWrite(PIN_ARM_ENABLE, HIGH);
  
  int diff; // declare diff for use in while condition
  do {
    // find angles and diff
    int current_angle = mpu_get_current_angle();
    diff = target_angle - current_angle;
    DebugLog.printf("[Arm Move]: Current Angle: %d, Target Angle: %d\n", current_angle, target_angle);

    
    if( diff > 0 ) { // extend
      DebugLog.println("[Arm Move] Extending");
      digitalWrite(PIN_ARM_RETRACT, LOW);
      digitalWrite(PIN_ARM_EXTEND, HIGH);
    }
    else if(diff < 0) { // retract
      DebugLog.println("[Arm Move] Retracting");
      digitalWrite(PIN_ARM_EXTEND, LOW);
      digitalWrite(PIN_ARM_RETRACT, HIGH);
    }
    else { // diff == 0, target angle reached, break loop to skip delay
      break;
    }

    delay(ARM_MOVE_POLLING_PERIOD); // wait for arm to move

  } while( diff != 0 );

  // turn off arm
  digitalWrite(PIN_ARM_ENABLE, LOW);
  digitalWrite(PIN_ARM_RETRACT, LOW);
  digitalWrite(PIN_ARM_EXTEND, LOW);

  DebugLog.printf("[Arm Move] Reached %d\n", target_angle);
}

void armController() {
  
  // get current arm angle from MPU
  int current_angle = mpu_get_current_angle();

  // Find target angle for tracking actuator / arm
  int target_angle = arm_get_target_angle();
  DebugLog.printf("Current Angle: %d, Target Angle: %d\n", current_angle, target_angle);
  
  // don't attempt to move the arm if it's disabled.
  if( !ARM_ENABLED ) {
    DebugLog.println("[ARM] Arm is disabled.");
    return;
  }

  // set acutator to correct position based on desired angle and current angle
  if( arm_check_move_needed(current_angle, target_angle) ) {
    DebugLog.println("Threshold crossed, moving to new angle...");
    arm_move(target_angle);
  }
  else {
    DebugLog.println("No move needed.");
  }
}


// Gyroscope functions
int mpu_get_current_angle() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // log uncalibrated values
  // mpu_calibration(a.acceleration.x, a.acceleration.y, a.acceleration.z);

  // apply calibration offsets
  float ax = a.acceleration.x - ACCEL_OFFSET[0];
  float ay = a.acceleration.y - ACCEL_OFFSET[1];
  float az = a.acceleration.z - ACCEL_OFFSET[2];

  // DebugLog.printf("[MPU] Values: {%f, %f, %f}\n", ax, ay, az);


  // convert to roll and pitch
  // float roll = atan2(ay, az) * 180.0/PI;
  float pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0/PI;

  // DebugLog.printf("[MPU] Roll: %7.3f, Pitch: %7.3f\n", roll, pitch);

  return (int)pitch;
}

void mpu_calibration( float x, float y, float z ) {
  mpu_num_samples++;
  z -= 9.81;
  DebugLog.printf("[MPU] Raw: {%f, %f, %f}\n", x, y, z);

  // calculate moving average
  if(mpu_num_samples == 1 ) {
    accel_raw_iteravg[0] = x;
    accel_raw_iteravg[1] = y;
    accel_raw_iteravg[2] = z;
  }
  else {
    accel_raw_iteravg[0] = ( accel_raw_iteravg[0]*(mpu_num_samples-1) + x ) / mpu_num_samples; // X
    accel_raw_iteravg[1] = ( accel_raw_iteravg[1]*(mpu_num_samples-1) + y ) / mpu_num_samples; // Y
    accel_raw_iteravg[2] = ( accel_raw_iteravg[2]*(mpu_num_samples-1) + z ) / mpu_num_samples; // Z
  }
  
  DebugLog.printf("[MPU] Raw iterative average (%d samples): {%f, %f, %f}\n", mpu_num_samples, accel_raw_iteravg[0], accel_raw_iteravg[1], accel_raw_iteravg[2]);
}


// Pump functions
void pump_controller() {
  int time = get_time(); // time in minutes since midnight

  if( pump_suspended_night && time >= TIME_SUNRISE ) { // if pump is suspended and its time to enable it, then unsuspend, activate, and schedule a deactivation.
    pump_suspended_night = 0;
    digitalWrite(PIN_PUMP, HIGH);
    pump_next_inactive = time + PUMP_DUTY_ACTIVE;
    pump_active = 1;
    matter_pump_active.setContact(pump_active);
    
    DebugLog.printf("[PUMP] Unsuspending and activating, will turn off at %d.\n", pump_next_inactive);
  }

  if( pump_suspended_night ) { // if the pump is suspended, leave it and do nothing.
    return;
  }

  if( (time >= TIME_SUNSET || time < TIME_SUNRISE) && !pump_suspended_night) { // if time is outside of enable times AND pump is not suspended, turn off and suspend the pump.
    digitalWrite(PIN_PUMP, LOW);
    pump_active = 0;
    pump_suspended_night = 1;
    matter_pump_active.setContact(pump_active);
    DebugLog.println("[PUMP] Suspended for the night.");
  }

  // Regular pump operation
  if( pump_active && time >= pump_next_inactive ) { // if pump on AND deactivate time reached, turn off pump and set time for activation.
    digitalWrite(PIN_PUMP, LOW);
    pump_next_active = time + PUMP_DUTY_INACTIVE;
    pump_active = 0;
    DebugLog.printf("[PUMP] Turned off, will turn on at %d.\n", pump_next_active);
  }
  else if( !pump_active && time >= pump_next_active ) { // if pump off AND activation time reached, turn on pump and set deactivate time
    digitalWrite(PIN_PUMP, HIGH);
    pump_next_inactive = time + PUMP_DUTY_ACTIVE;
    pump_active = 1;
    DebugLog.printf("[PUMP] Turned on, will turn off at %d.\n", pump_next_inactive);
  }
  matter_pump_active.setContact(pump_active);
}


// Matter functions
void matter_update_temp_sensors() {
  matter_temp_input.setTemperature(temp_get_by_addr(TEMP_INPUT));
  matter_temp_collector.setTemperature(temp_get_by_addr(TEMP_COLLECTOR));
  matter_temp_tank.setTemperature(temp_get_by_addr(TEMP_TANK));
  matter_temp_air.setTemperature(temp_get_by_addr(TEMP_AIR));
}




