#ifndef __CONFIG_H__
#define __CONFIG_H__

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
static constexpr const char* MQTT_TOPIC_THERMOSTAT_STATE = CONFIG_MQTT_REALM "/thermostat/state";  // R
static constexpr const char* MQTT_TOPIC_THERMOSTAT_CONTROL = CONFIG_MQTT_REALM "/thermostat/control";  // W
static constexpr const char* MQTT_TOPIC_HEATER_STATE = CONFIG_MQTT_REALM "/heater/state";  // R 
static constexpr const char* MQTT_TOPIC_HEATER_CONTROL = CONFIG_MQTT_REALM "/heater/control";  // W
static constexpr const char* MQTT_TOPIC_POOL_TEMP = CONFIG_MQTT_REALM "/pool/temperature";  // R
static constexpr const char* MQTT_TOPIC_POOL_SETPOINT = CONFIG_MQTT_REALM "/pool/setpoint";  // W
#ifdef CONFIG_HAS_HEAT_EXCHANGER
static constexpr const char* MQTT_TOPIC_EXCHANGER_TEMP = CONFIG_MQTT_REALM "/exchanger/temperature";  // R
static constexpr const char* MQTT_TOPIC_EXCHANGER_SETPOINT = CONFIG_MQTT_REALM "/exchanger/setpoint";  // W
#endif
#ifdef CONFIG_HAS_WATER_SENSOR
static constexpr const char* MQTT_TOPIC_AQUASTAT_STATE = CONFIG_MQTT_REALM "/refill/state";  // R
static constexpr const char* MQTT_TOPIC_AQUASTAT_CONTROL = CONFIG_MQTT_REALM "/refill/control";  // W
static constexpr const char* MQTT_TOPIC_WATERLEVEL = CONFIG_MQTT_REALM "/pool/waterlevel";  // R
#endif
#ifdef CONFIG_HAS_WATER_VALVE
static constexpr const char* MQTT_TOPIC_VALVE_STATE = CONFIG_MQTT_REALM "/valve/state";  // R
static constexpr const char* MQTT_TOPIC_VALVE_CONTROL = CONFIG_MQTT_REALM "/valve/control";  // W
#endif


// Thermostat/heater configuration  -- all temperatures in Fahrenheit
static constexpr int HEATER_RELAY_PIN = 4;
static constexpr int HEATER_TOGGLE_THRESHOLD_SEC = 60;  // TODO - Unused !!

#ifdef CONFIG_USE_ADS1115
static constexpr int THERMISTOR_POOL_CHANNEL = -1;  // TODO
#else
static constexpr int THERMISTOR_POOL_PIN = -1;  // TODO
#endif

#ifdef HAS_HEAT_EXCHANGER
#  ifdef CONFIG_USE_ADS1115
static constexpr int THERMISTOR_EXCHANGER_CHANNEL = -1;  // TODO
#  else
static constexpr int THERMISTOR_EXCHANGER_PIN = -1;  // TODO
#  endif
#endif  // HAS_HEAT_EXCHANGER

static constexpr int THERMOSTAT_UPDATE_INTERVAL_SEC = 5;

static constexpr int THERMOSTAT_DEFAULT_AUTO_OFF_SEC = 8*3600;  // 8 hours

static const double THERMOSTAT_SETPOINT_POOL_DEFAULT = 89.0;
static const double THERMOSTAT_SETPOINT_POOL_OVERSHOOT = 0.3;
static const double THERMOSTAT_SETPOINT_POOL_UNDERSHOOT = 0.5;
#ifdef CONFIG_HAS_HEAT_EXCHANGER 
static const double THERMOSTAT_SETPONT_EXCHANGER_DEFAULT = 195.0;
static const double THERMOSTAT_SETPOINT_EXCHANGER_OVERSHOOT = 0.5;
static const double THERMOSTAT_SETPOINT_EXCHANGER_UNDERSHOOT = 1.0;
#endif


// Aquastat/refill configuration
#ifdef CONFIG_HAS_WATER_VALVE
static constexpr int VALVE_SOLENOID_PIN = -1;
static constexpr int VALVE_TOGGLE_THRESHOLD_SEC = 60;  // TODO - Unused !!
#endif

#ifdef CONFIG_HAS_WATER_SENSOR

#  ifdef CONFIG_USE_ADS1115
static constexpr int WATERLEVEL_CHANNEL = -1;  // TODO
#  else
static constexpr int WATERLEVEL_PIN = -1;  // TODO
#  endif

static constexpr int AQUASTAT_UPDATE_INTERVAL_SEC = 10;

static constexpr int AQUASTAT_DEFAULT_AUTO_OFF_SEC = 15*60;  // 15 minutes

static constexpr double AQUASTAT_SETPOINT_DEFAULT = 0.0;
#endif  // CONFIG_HAS_WATER_SENSOR

/*************************************************************************
 * Derived, non-editable settings                                        */


#endif  /* __CONFIG_H__ */