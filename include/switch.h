#ifndef __SWITCH_H__
#define __SWITCH_H__

#include "config.h"
#include "task_scheduler.h"
#include "mqtt.h"

#include <functional>
#include <vector>

#include "string_util.h"
#include "sigslot.h"


class Switch {
public:
  typedef enum { EVENT_OFF = 0, EVENT_ON } event_t;
  using signal_t = ch::signal<event_t>;
  using callback_t = signal_t::slot_t;

  static constexpr int DELAYED_OFF_DISABLE = -1;

  static constexpr const char* MQTT_TOPIC_SUFFIX_STATE = "/state";
  static constexpr const char* MQTT_TOPIC_SUFFIX_CONTROL = "/control";
  static constexpr const char* MQTT_TOPIC_SUFFIX_STATE_AUTO_OFF = "/state/autoff";
  static constexpr const char* MQTT_TOPIC_SUFFIX_CONTROL_AUTO_OFF = "/control/autoff";

  Switch(
    int pin, 
    MQTT& mqtt, const char* const mqtt_state_topic, const char* const mqtt_control_topic,
    const char* const mqtt_state_auto_off_topic = nullptr, const char* const mqtt_control_auto_off_topic = nullptr,
    int default_delayed_off_sec = DELAYED_OFF_DISABLE,
    bool active_high = true
  ) :
    _pin(pin), _mqtt(mqtt),
    _mqttStateTopic(mqtt_state_topic), _mqttControlTopic(mqtt_control_topic),
    _mqttStateAutoOffTopic(mqtt_state_auto_off_topic), _mqttControlAutoOffTopic(mqtt_control_auto_off_topic),
    _defaultDelayedOffSec(default_delayed_off_sec), 
    _activeHigh(active_high),
    _delayedOffTask(TASK_IMMEDIATE, TASK_ONCE, std::bind(&Switch::off, this))
  { }
  ~Switch() { }

  // Convenience constructor to specify MQTT topics more concisely
  Switch(
    int pin,
    MQTT& mqtt, const char* const mqtt_topic_prefix,
    bool mqtt_auto_off_support = false,
    int default_delayed_off_sec = DELAYED_OFF_DISABLE,
    bool active_high = true
  ) :
    Switch(
      pin, mqtt, 
      concat_into(mqtt_topic_prefix, MQTT_TOPIC_SUFFIX_STATE).c_str(), 
      concat_into(mqtt_topic_prefix, MQTT_TOPIC_SUFFIX_CONTROL).c_str(),
      mqtt_auto_off_support ? concat_into(mqtt_topic_prefix, MQTT_TOPIC_SUFFIX_STATE_AUTO_OFF).c_str() : nullptr,
      mqtt_auto_off_support ? concat_into(mqtt_topic_prefix, MQTT_TOPIC_SUFFIX_CONTROL_AUTO_OFF).c_str() : nullptr,
      default_delayed_off_sec, active_high)
  { }

  void begin(Scheduler* sched, bool on = false);

  // Switch control
  inline bool is_on()  { 
    return digitalRead(_pin) == (_activeHigh ? HIGH : LOW);
  }
  void set_on(bool on);
  inline void on() {
    set_on(true); 
  }
  inline void off() {
    set_on(false);
  }

  // Delayed off control
  inline bool is_delayed_off_pending() {
    // XXX - Should we be able to  distinguish between 
    //  explicitly requested (via direct call to ::delayed_off()) vs. 
    //  automatically started (by call to ::set_on(true))?  Meh...?
    return _delayedOffTask.isEnabled();
  }
  void delayed_off(int sec);
  inline void delayed_off_cancel() {
    delayed_off(DELAYED_OFF_DISABLE); 
  }

  // Default delayed off
  inline int get_default_delayed_off() {
    return _defaultDelayedOffSec;
  }
  void set_default_delayed_off(int sec, bool reset_if_pending = true);

  signal_t signal;

private:
  const int _pin;

  MQTT& _mqtt;
  const String _mqttStateTopic;    // Publish state updates
  const String _mqttControlTopic;  // Subscribe for state change requests
  const String _mqttStateAutoOffTopic;    // (Optionally) Publish default delayed off changes
  const String _mqttControlAutoOffTopic;  // (Optionally) Subscribe for default delayed off configuration requests

  int _defaultDelayedOffSec;  // -1 to disable; otherwise an set_on(true) will automaticallt register a delayedOff
  const bool _activeHigh;  // aka. NO (normally off / open)

  Task _delayedOffTask;

  void _mqttCb(const char* topic, String payload);
};

#endif  /* __SWITCH_H__ */