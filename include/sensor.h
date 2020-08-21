#ifndef __SENSOR_H__
#define __SENSOR_H__

#include "config.h"
#include "task_scheduler.h"
#include "mqtt.h"

#include "sigslot.h"

// Base class (interface) for "sensor port" wrapper classes;
//   only need to support a single get/update value pair of methods
class SensorPort {
public:
  virtual ~SensorPort() { }

  virtual void update() = 0;
  inline double value() const {
    return _value;
  }

protected:
  double _value;
};

// Analog sensor with periodic update and MQTT publishing
class SensorBase {
public:
  // MQTT message is published and update callback is triggered only if new value 
  //   differs by at least this much (raw difference) from old value
  //
  // TODO - We may want to make these configurable parameters, with separate values
  //   for MQTT publishing and for callback invocation...
  static constexpr double UPDATE_TRIGGER_THRESHOLD = 5e-2;

  using update_signal_t = ch::signal<double>;
  using update_callback_t = update_signal_t::slot_t;

  SensorBase(
    SensorPort& port, unsigned long update_interval_millis,
    MQTT& mqtt, const char* mqtt_topic
  ) :
    _port(port),
    _mqtt(mqtt), _mqttTopic(mqtt_topic),
    _updateTask(update_interval_millis, TASK_FOREVER, std::bind(&SensorBase::update, this))
  { }
  ~SensorBase() { }

  void begin(Scheduler* sched, bool auto_update = true);

  void update();
  
  inline bool is_auto_update_enabled() {
    return _updateTask.isEnabled();
  }
  void set_auto_update_enabled(bool enabled);
  inline void enable_auto_update() {
    set_auto_update_enabled(true);
  }
  inline void disable_auto_update() {
    set_auto_update_enabled(false);
  }

  inline double value() {
    return _port.value();
  }

  update_signal_t updateSignal;

private:
  SensorPort& _port;

  MQTT& _mqtt;
  const String _mqttTopic;

  Task _updateTask;
};


// Trivial templated subclass that only defines a convenience constructor 
//  (and hides the *SensorPort wrapper class)
//
// XXX Should we just have this class and eliminate SensorBase?
//   However, then we would have to make Controller templated... ugh.
//
// XXX Would be nice if we could omit the PostClassArgs template parameter.
//   Note that SensorPort subclasses should typically have just one constructor,
//   but are not required to (I can't think of actual examples to contrary, but
//   I guess it's conceivable..).
//
//   Following records some further thoughts of the moment for my future self.
//
//   Possible issues with explicit PortClassArgs param:
//   - Obviously, duplication (need to look up PortClass constructor and copy-paste arg types).
//   - If PortClass has more than one constructor and we need sensor objects that use both,
//     we'd need sort-of-redundant template instantiations. Yes, it's a tiny waste of IRAM,
//     but it *is* an MCU (unless if compiler inlines everything anyway? hmm..)
//
//   It would be nice if, when PortClass has one constructor, we could automatically extract
//   it's argument types programmatically, but I doubt C++ would have such a compile-time
//   introspection feature (it'd have to work for general case, i.e. multiple constructors,
//   and how do you design *that* in a sensible way?).
//
//   Possible alternatives, off top of head:
//   - One is to templetize (just) the constructor separately and put PortClassArgs there.
//     This would address 2nd issue above but is probably worse wrt main (ie 1st) issue,
//     as each Sensor instance declaration would have to explicitly specify constructor
//     template params (unless if "using" could alias constructor, without subclassing,
//     but that'd be ambiguous, no?).
//   - Another is to switch to forwarding references (PortClassArgs&&...) *and* also
//     move those params to separate constructor-only template (if we do just first,
//     we don't achieve much, no?). This *might* work, but I'm currently not very confident
//     about rvalue semantics, and also don't want to spend another hour or two on
//     "hello C++ world" trials to reverse-engineer language design. I'm gonna guess that,
//     as long as PortClass doesn't define copy constructor/operator, we *might* get
//     C++ inference to instantiate constructor with a & or const& (which I expect to be
//     common case for PortClass constructor args)--except if there are obscure corner cases
//     that prevent such a rule, so.. ugh.. C++, the language whose design you cannot
//     reverse-engineer with some common sense, no matter your experience. :)
//
//   Hence, this is good enough, at least for now. Moving on..
template<class PortClass, typename... PortClassArgs>
class Sensor : public SensorBase {
public:
  explicit Sensor(
    PortClassArgs... pcArgs, unsigned long update_interval_millis,
    MQTT& mqtt, const char* mqtt_topic
  ) :
    SensorBase(_thePort, update_interval_millis, mqtt, mqtt_topic),
    _thePort(pcArgs...) { 
    // PortClass parameter must be a subclass of SensorPort
    static_assert(
      std::is_base_of<SensorPort, PortClass>::value, 
      "PortClass template parameter is not a subclass of SensorPort");
  }

private:
  PortClass _thePort;  // _port is in base class (and will reference this object)
};

#endif  /* __SENSOR_H__ */