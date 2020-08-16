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


static Scheduler sched;

static StatusDisplay display;
static WiFiManager wifiManager(display);
static Clock ntpClock(display, NTP_TIME_OFFSET);
static MQTT mqtt(display, MQTT_SERVER_HOSTNAME, MQTT_SERVER_PORT);

// TODO - Remove once MQTT trial is done
static constexpr int LED_PIN = 2;
static constexpr const char* MQTT_LED_TOPIC_PREFIX = "esp32/led";
static Switch led(LED_PIN, mqtt, MQTT_LED_TOPIC_PREFIX, true, 10);


void setup() {
  Serial.begin(9600);

  sched.init();
  DEBUG_MSG("Scheduler initialized");

  display.begin();
  wifiManager.begin(&sched);
  wifiManager.connect();
  ntpClock.begin(&sched);  // Ok to start before wifi connects...
  mqtt.begin(&sched);  
  mqtt.connect();  // Ditto, but would be better to start after wifi connects?

  led.begin(&sched);  // TODO - Remove after MQTT trial
}

void loop() {
  ntpClock.loop();
  mqtt.loop();
  sched.execute();
}