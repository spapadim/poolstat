#include <Arduino.h>
#include "config.h"

#include "switch.h"

void Switch::begin(Scheduler* sched, bool on) {
  pinMode(_pin, OUTPUT);

  using namespace std::placeholders;

  sched->addTask(_delayedOffTask);
  _mqtt.add_subscription(_mqttControlTopic, std::bind(&Switch::_mqttCb, this, _1, _2), 1);
  if (_mqttControlAutoOffTopic) {
    _mqtt.add_subscription(_mqttControlAutoOffTopic, std::bind(&Switch::_mqttCb, this, _1, _2), 1);
  }

  // Set initial state (remember, this will also publish MQTT message)
  set_on(on);
  // Set default_delayed_off back to it's current value just so it's published on MQTT (if topic set, of course)
  set_default_delayed_off(get_default_delayed_off());
}

void Switch::set_on(bool on) {
  // Cancel any pending delayed off task
  delayed_off_cancel();

  // Set GPIO state
  if (on) {
    digitalWrite(_pin, _activeHigh ? HIGH : LOW);
  } else {
    digitalWrite(_pin, _activeHigh ? LOW : HIGH);
  }

  // Enable delayed off, as necessary
  if (on && _defaultDelayedOffSec != DELAYED_OFF_DISABLE) {
    delayed_off(_defaultDelayedOffSec);
  }

  // Publish current state
  _mqtt.publish(_mqttStateTopic.c_str(), on ? "on" : "off", true);  // Retained message

  DEBUG_MSG("Controller::set_switch_on(%d)", int(on));

  // Invoke any registered callbacks
  for (auto it = _callbacks.begin();  it != _callbacks.end();  it++) {
    (*it)(on ? SWITCH_EVENT_ON : SWITCH_EVENT_OFF);
  }
}

void Switch::delayed_off(int sec) {
  if (sec == DELAYED_OFF_DISABLE) {
    _delayedOffTask.disable();
  } else {
    _delayedOffTask.restartDelayed(sec * 1000);
  }
}

void Switch::set_default_delayed_off(int sec, bool reset_if_pending) {
  _defaultDelayedOffSec = sec;
  // If a delayed off is pending, also change it's interval (includes cancelling)
  if (reset_if_pending && is_delayed_off_pending()) {
    delayed_off(sec);
  }
  // (Optionally) publish change on MQTT
  if (_mqttStateAutoOffTopic) {
    _mqtt.publish(_mqttStateAutoOffTopic.c_str(), String(sec).c_str(), true);  // Retained message
  }
}

// private
void Switch::_mqttCb(const char* topic, String payload) {
  if (!_mqttControlAutoOffTopic || _mqttControlTopic == topic) {  // Note hacky optimization...
    bool on = (payload == "on");
    DEBUG_MSG("MQTT Switch::set_on to %d", int(on));
    set_on(on);
  } else if (_mqttControlAutoOffTopic == topic) {
    int sec = atoi(payload.c_str());
    DEBUG_MSG("MQTT Switch::set_default_delayed_off to %d", sec);
    set_default_delayed_off(sec, true);
  } else {
    DEBUG_MSG("WHOOPS: Switch got unknown MQTT topic %s", topic);
  }
}
