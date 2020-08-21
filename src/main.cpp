#include <Arduino.h>
#include "config.h"

#include <WiFi.h>

#include <functional>

#include "task_scheduler.h"
#include "status_display.h"
#include "wifi_manager.h"
#include "clock.h"
#include "mqtt.h"

#include "switch.h"
#include "thermostat.h"

#ifdef CONFIG_USE_ADS1115
#include <Adafruit_ADS1015.h>
#endif

#ifdef CONFIG_HAS_AMBIENT_SENSOR
#include <uFire_SHT20.h>
#include "sht_sensor.h"
#endif


static Scheduler sched;

static StatusDisplay display;
static WiFiManager wifiManager(display);
static Clock ntpClock(display, NTP_TIME_OFFSET);
static MQTT mqtt(display, MQTT_SERVER_HOSTNAME, MQTT_SERVER_PORT);

// TODO - Remove once MQTT trial is done
static constexpr int LED_PIN = 2;
static constexpr const char* MQTT_LED_TOPIC_PREFIX = "esp32/led";
static Switch led(LED_PIN, mqtt, MQTT_LED_TOPIC_PREFIX, true, 10);

// Thermistor sensor configuration
#ifdef CONFIG_USE_ADS1115
// TODO
#else
static MCUPort _poolThermistorPort(THERMISTOR_POOL_PIN, ESP32_ADC_ATTENUATION_FACTOR);
#endif
//static Thermistor _poolThermistor(_poolThermistorPort, 10000.0, 10000.0, 25.0, 3950.0, 0.98);
static Thermistor _poolThermistor(_poolThermistorPort, 1000.0, 10000.0, 25.0, 3950.0, 0.98);
static ThermistorSensor poolThermistorSensor(
  _poolThermistor, THERMISTOR_UPDATE_INTERVAL_MILLIS,
  mqtt, MQTT_TOPIC_POOL_TEMP);


#ifdef CONFIG_HAS_AMBIENT_SENSOR
static uFire_SHT20 sht20;
static SHTTemperatureSensor ambientTemperatureSensor(sht20, AMBIENT_UPDATE_INTERVAL_MILLIS, mqtt, MQTT_TOPIC_AMBIENT_TEMPERATURE);
static SHTHumiditySensor ambientHumiditySensor(sht20, AMBIENT_UPDATE_INTERVAL_MILLIS, mqtt, MQTT_TOPIC_AMBIENT_HUMIDITY);
#endif

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // TODO Should we do all this just before we start sensors and thermostats?
  //   Eg, might ADS .begin() set I2C to a speed too slow?
  //   Here should be ok, but double-check
  //   Also, maybe move all #ifdef-needing bits to separate static function
#ifdef CONFIG_USE_ADS1115
  // TODO more?
  ads1115.begin();
  ads1115.setGain(ADS1115_ADC_GAIN);
#else
  analogReadResolution(ESP32_ADC_RESOLUTION_BITS);
  analogSetAttenuation(ESP32_ADC_ATTENUATION);  // For all pins
#endif
#ifdef CONFIG_HAS_AMBIENT_SENSOR
  sht20.begin();
#endif
  DEBUG_MSG("Low-level initialization complete");

  sched.init();
  DEBUG_MSG("Scheduler initialized");

  display.begin();
  wifiManager.begin(&sched);
  wifiManager.connect();
  ntpClock.begin(&sched);  // Ok to start before wifi connects...
  mqtt.begin(&sched);  
  mqtt.connect();  // Ditto, but would be better to start after wifi connects?

  // TODO - Remove after MQTT trial
  led.begin(&sched);
  led.signal.connect([](Switch::event_t evt){ DEBUG_MSG("LED callback, evt %d", (int)evt); });

  poolThermistorSensor.begin(&sched);
  poolThermistorSensor.updateSignal.connect([](double value){ display.temperature(value); });

#ifdef CONFIG_HAS_AMBIENT_SENSOR
  ambientTemperatureSensor.begin(&sched);
  ambientHumiditySensor.begin(&sched);
#endif
}

void loop() {
  ntpClock.loop();
  mqtt.loop();
  sched.execute();
}