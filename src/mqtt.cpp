#include <Arduino.h>
#include "config.h"

#include "mqtt.h"


void MQTT::begin(Scheduler* sched) {
  DEBUG_MSG("MQTT::begin()");

  using namespace std::placeholders;

  // Set MQTT config parameters
  _mqttClient.setServer(_serverHostname, _serverPort);
  _mqttClient.setCallback(std::bind(&MQTT::_mqttMessageCb, this, _1, _2, _3));

  randomSeed(micros());
  _clientId = MQTT_CLIENT_ID_PREFIX;
  _clientId += String(random(0xffff), HEX);

  // Display off (no connection attempt yet)
  _display.mqtt(STATUS_DISPLAY_NONE);

  // Add all tasks; need to explicitly call ::connect() to get things started
  sched->addTask(_connCheckTask);
  sched->addTask(_connectTask);
  sched->addTask(_msgQueueFlushTask);
}

void MQTT::connect() {
  // Do nothing if already connected
  if (_mqttClient.connected())
    return;

  DEBUG_MSG("MQTT::connect() starting");

  // Do nothing if already started connection attempt
  if (!_connectTask.isEnabled())
    _connectTask.restart();
}


bool MQTT::publish(const char* topic, const char* payload, bool retained, bool retry) {
  DEBUG_MSG("MQTT::publish topic %s / payload %s [retain %d; retry %d]", topic, payload, int(retained), int(retry));

  // Make one immediate attempt
  bool success = _mqttClient.publish(topic, payload, retained);
  
  // If necessary, store for retry in the future
  if (!success && retry) {
    DEBUG_MSG("MQTT publish failed, adding to retry queue");
    //_msgQueue.push(Message(topic, payload, retained));
    _msgQueue.emplace(topic, payload, retained);
    if (!_msgQueue.empty() && !_msgQueueFlushTask.isEnabled() && _mqttClient.connected()) {
      DEBUG_MSG("MQTT enabling retry queue flush task");
      _msgQueueFlushTask.enableDelayed(MQTT_PUBLISH_RETRY_INTERVAL_MILLIS);
    }
  }

  // TODO XXX - Should we add a simple queue size threshold?
  //   Also, if this ever becomes a library component, we should also add payload size limit...

  return success;
}


bool MQTT::add_subscription(const char* topic, MQTTCallback callback, uint8_t qos) {
  String topicStr(topic);

  if (_subscriptionsMap.count(topicStr) > 0) {
    return false;
  }

  _subscriptionsMap[topicStr] = std::make_pair(callback, qos);
  if (_mqttClient.connected()) {
    _mqttClient.subscribe(topic, qos);
  }

  return true;
}

bool MQTT::remove_subscription(const char* topic) {
  String topicStr(topic);
  auto it = _subscriptionsMap.find(topicStr);
  if (it == _subscriptionsMap.end()) {
    return false;
  }

  if (_mqttClient.connected()) {
    _mqttClient.unsubscribe(topic);
  }
  _subscriptionsMap.erase(it);

  return true;
}


// private
void MQTT::_connCheckCb() {
  DEBUG_MSG("MQTT::_connCheckCb()");

  if (!_mqttClient.connected()) {
    DEBUG_MSG("MQTT detected disconnection");
    _display.mqtt(STATUS_DISPLAY_INACTIVE);
    connect();
  }
}

// private
void MQTT::_connectCb() {
  int count = _connectTask.getRunCounter() - 1;
  DEBUG_MSG("MQTT::_connectCb() count=%d", count);

  if (count == 0) {
     // Disable connection check task (if necessary) and update display
    _display.mqtt(STATUS_DISPLAY_ACTIVATING);
    if (_connCheckTask.isEnabled()) {
      _connCheckTask.disable();
    }
  }

  // Attempt to connect
  if (!_mqttClient.connect(_clientId.c_str())) {
    DEBUG_MSG("MQTT connect failed, rc=%d", _mqttClient.state());

    if (count == _connectTask.getIterations() - 1) {
      _display.mqtt(STATUS_DISPLAY_INACTIVE);  // Last attempt failed, update indicator accordingly
    }
  } else {
    DEBUG_MSG("MQTT connected as %s", _clientId.c_str());

    _display.mqtt(STATUS_DISPLAY_ACTIVE);
    _connectTask.disable();

    // Subscribe to all registered topic subscriptions
    for ( auto it = _subscriptionsMap.begin(); it != _subscriptionsMap.end(); ++it ) {
      DEBUG_MSG("MQTT subscribing to %s", it->first.c_str());
      _mqttClient.subscribe(it->first.c_str(), it->second.second);
    }

    _connCheckTask.enable();

    // Enable msg resend queue flush task, if necessary
    // XXX - There's two points where this is done, flow is hard to follow, consider re-factoring?
    if (!_msgQueue.empty() && !_msgQueueFlushTask.isEnabled()) {
      DEBUG_MSG("MQTT enabling retry queue flush task (after re-connection)");
      _msgQueueFlushTask.enable();
    }

  }
}

// private
void MQTT::_msgQueueFlushCb() {
  // PubSubClient library only supports publishing with qos=0 (at most once),
  // hence we implement re-transmission ourselves.
  // This effectively does qos=1 (at least once), but no idea if it complies
  // with all MQTT protocol details.  It works in practice though.
  DEBUG_MSG("MQTT::_msgQueueFlushCb");

  // Try to re-send up to MAX_BATCH queued messages
  for (int nbatch = 0;  !_msgQueue.empty() && nbatch < MQTT_PUBLISH_RETRY_MAX_BATCH;  nbatch++) {
    MQTT::Message& msg = _msgQueue.front();
    DEBUG_MSG("MQTT re-transmit topic %s / payload %s [retain %d]", msg.topic.c_str(), msg.payload.c_str(), int(msg.retained));
    if (!_mqttClient.publish(msg.topic.c_str(), msg.payload.c_str(), msg.retained)) {
      break;  // Back-off if transmission fails..
    }
    _msgQueue.pop();
  }

  // Stop task if queue is empty
  if (_msgQueue.empty()) {
    DEBUG_MSG("MQTT retry queue empty, disabling flush task");
    _msgQueueFlushTask.disable();
  }
}

// private
void MQTT::_mqttMessageCb(const char *topic, const byte *payload, unsigned int length) {
  // Look up subscriptions map
  String topicStr(topic);
  auto map_entry = _subscriptionsMap.find(topicStr);

  // Sanity check
  if (map_entry == _subscriptionsMap.end()) {
    DEBUG_MSG("WHOOPS: MQTT received message on topic '%s' not in subscriptions map!", topic);
    return;
  }

  // Safely copy payload bytes into a string
  String data((const char *)NULL);  // Needs cast, otherwise treats arg as (int)0 ?!
  data.reserve(length);  // We do not need to +1 for '\0'
  for (int i = 0;  i < length; i++)
    data.concat((char)(payload[i]));

  // Process MQTT message
  DEBUG_MSG("MQTT received topic: '%s' / payload: '%s'", topic, data.c_str());
  map_entry->second.first(topic, data);
}
