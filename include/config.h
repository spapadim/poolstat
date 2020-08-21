#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <cmath>  // for INFINITY

#include "config_secrets.h"  // Constants that should not be committed to repo

// Debugging macros
#define DEBUG_MSG(fmt, ...) { \
  Serial.print("DBG:  "); \
  Serial.printf(fmt, ##__VA_ARGS__); \
  Serial.println(); \
}

// Device hardware configuration options
//#define CONFIG_HAS_HEAT_EXCHANGER
#define CONFIG_HAS_WATER_VALVE
#define CONFIG_HAS_WATER_SENSOR
//#define CONFIG_USE_ADS1115
#define CONFIG_HAS_AMBIENT_SENSOR
#define CONFIG_MQTT_REALM "pool2"


// Network configuration
static constexpr int WIFI_CONNCHECK_INTERVAL_SEC = 5;
static constexpr int WIFI_CONNECT_INTERVAL_MILLIS = 400;
static constexpr int WIFI_CONNECT_ITERATIONS = 14;

// NTP configuration
static const int NTP_TIME_OFFSET = -14400;  // UTC-4 (includes DST)

// MQTT configuration
static constexpr const char* MQTT_SERVER_HOSTNAME = SECRET_MQTT_HOSTNAME;
static constexpr int MQTT_SERVER_PORT = 1883;

static constexpr int MQTT_CONNCHECK_INTERVAL_SEC = 5;
static constexpr int MQTT_CONNECT_RETRY_INTERVAL_MILLIS = 800;
static constexpr int MQTT_CONNECT_RETRIES = 10;
static constexpr int MQTT_PUBLISH_RETRY_INTERVAL_MILLIS = 500;  // Retry task iteration interval
static constexpr int MQTT_PUBLISH_RETRY_MAX_BATCH = 10;  // Maximum number of messages to publish in one iteration of retry task; one iteration should not take too long...
static constexpr int MQTT_PUBLISH_RETRY_QUEUE_CAPACITY = 20;  // Capacity of re-send queue (actually, it's a ring buffer...)

static constexpr const char* MQTT_CLIENT_ID_PREFIX = "poolstat-";

static constexpr const char* MQTT_TOPIC_POOL_TEMP = CONFIG_MQTT_REALM "/pool/temperature";  // R
#ifdef CONFIG_HAS_HEAT_EXCHANGER
static constexpr const char* MQTT_TOPIC_EXCHANGER_TEMP = CONFIG_MQTT_REALM "/exchanger/temperature";  // R
#endif
static constexpr const char* MQTT_TOPIC_THERMOSTAT_STATE = CONFIG_MQTT_REALM "/thermostat/state";  // R
static constexpr const char* MQTT_TOPIC_THERMOSTAT_CONTROL = CONFIG_MQTT_REALM "/thermostat/control";  // W
static constexpr const char* MQTT_TOPIC_THERMOSTAT_SETPOINT = CONFIG_MQTT_REALM "/thermostat/setpoint";  // W
static constexpr const char* MQTT_TOPIC_HEATER_STATE = CONFIG_MQTT_REALM "/heater/state";  // R 
static constexpr const char* MQTT_TOPIC_HEATER_CONTROL = CONFIG_MQTT_REALM "/heater/control";  // W

#ifdef CONFIG_HAS_WATER_SENSOR
static constexpr const char* MQTT_TOPIC_WATERLEVEL = CONFIG_MQTT_REALM "/pool/waterlevel";  // R
static constexpr const char* MQTT_TOPIC_AQUASTAT_STATE = CONFIG_MQTT_REALM "/refill/state";  // R
static constexpr const char* MQTT_TOPIC_AQUASTAT_CONTROL = CONFIG_MQTT_REALM "/refill/control";  // W
static constexpr const char* MQTT_TOPIC_AQUASTAT_SETPOINT = CONFIG_MQTT_REALM "/refill/setpoint"; // W
#endif
#ifdef CONFIG_HAS_WATER_VALVE
static constexpr const char* MQTT_TOPIC_VALVE_STATE = CONFIG_MQTT_REALM "/valve/state";  // R
static constexpr const char* MQTT_TOPIC_VALVE_CONTROL = CONFIG_MQTT_REALM "/valve/control";  // W
#endif

#ifdef CONFIG_HAS_AMBIENT_SENSOR
static constexpr const char* MQTT_TOPIC_AMBIENT_TEMPERATURE = CONFIG_MQTT_REALM "/ambient/temperature";
static constexpr const char* MQTT_TOPIC_AMBIENT_HUMIDITY = CONFIG_MQTT_REALM "/ambient/humidity";
#endif


// Thermostat/heater configuration  -- all temperatures in Fahrenheit
static constexpr int HEATER_RELAY_PIN = 4;
static constexpr int HEATER_TOGGLE_THRESHOLD_SEC = 60;  // TODO - Unused !!

#ifdef CONFIG_USE_ADS1115
static constexpr int THERMISTOR_POOL_CHANNEL = 2;
#else
static constexpr int THERMISTOR_POOL_PIN = 34;  // ESP32 pin D34 == A6
#endif

#ifdef HAS_HEAT_EXCHANGER
#  ifdef CONFIG_USE_ADS1115
static constexpr int THERMISTOR_EXCHANGER_CHANNEL = 0;
#  else
static constexpr int THERMISTOR_EXCHANGER_PIN = 35;  // ESP32 pin D35 == A7
#  endif
#endif  // HAS_HEAT_EXCHANGER

static constexpr unsigned long THERMISTOR_UPDATE_INTERVAL_MILLIS = 1000;

// ADC attenuation parameters
#ifdef CONFIG_USE_ADS1115
static const adsGain_t ADS1115_ADC_GAIN = GAIN_ONE;  // TODO
static const double ADS1115_ADC_ATTENUATION_FACTOR = 1.0;  // TODO -1 
#else
static const uint8_t ESP32_ADC_RESOLUTION_BITS = 12;
static const adc_attenuation_t ESP32_ADC_ATTENUATION = ADC_6db;
// XXX The theoretical attenuation factor for 6db is 1/3 (0.66667), but
//   empirically, the ADC seems to under-shoot by about 10%, at
//   least at the temperature ranges of interest for pool water...
//   Rather than debug circuit, this is q uuick-n-dirty workaround.
static const double ESP32_ADC_ATTENUATION_FACTOR = 0.60351;
#endif

static constexpr int THERMOSTAT_UPDATE_INTERVAL_SEC = 5;

static constexpr int THERMOSTAT_DEFAULT_AUTO_OFF_SEC = 8*3600;  // 8 hours

static const double THERMOSTAT_SETPOINT_DEFAULT = 89.0;
static const double THERMOSTAT_SETPOINT_OVERSHOOT = 0.3;
static const double THERMOSTAT_SETPOINT_UNDERSHOOT = 0.5;


// Aquastat/refill configuration
#ifdef CONFIG_HAS_WATER_VALVE
static constexpr int VALVE_SOLENOID_PIN = 2;
static constexpr int VALVE_TOGGLE_THRESHOLD_SEC = 60;  // TODO - Unused !!
#endif

#ifdef CONFIG_HAS_WATER_SENSOR

#  ifdef CONFIG_USE_ADS1115
static constexpr int WATERLEVEL_CHANNEL = -1;  // TODO
#  else
static constexpr int WATERLEVEL_PIN = -1;  // TODO
#  endif

// TODO Level values
static constexpr const double WATERLEVEL_VALUES[] = { -INFINITY, -4.0, -3.0, -2.0, -1.0, -0.5, 0.0 };
static constexpr int WATERLEVEL_NUM_VALUES = sizeof(WATERLEVEL_VALUES) / sizeof(double*);

static constexpr int AQUASTAT_UPDATE_INTERVAL_SEC = 10;

static constexpr int AQUASTAT_DEFAULT_AUTO_OFF_SEC = 15*60;  // 15 minutes

static constexpr double AQUASTAT_SETPOINT_DEFAULT = 0.0;
#endif  // CONFIG_HAS_WATER_SENSOR


#ifdef CONFIG_HAS_AMBIENT_SENSOR
static constexpr unsigned long AMBIENT_UPDATE_INTERVAL_MILLIS = 1000L * 10;
#endif
/*************************************************************************
 * Derived, non-editable settings                                        */


#endif  /* __CONFIG_H__ */