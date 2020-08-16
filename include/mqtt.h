#ifndef __MQTT_H__
#define __MQTT_H__

#include "config.h"

#include <functional>
#include <unordered_map>
#include "string_util.h"
#include <utility>
#include "ring_buffer.h"

#include <WiFi.h>
#include <PubSubClient.h>

#include "task_scheduler.h"
#include "status_display.h"

typedef std::function<void(const char* topic, String payload)> MQTTCallback;

class MQTT {
public:
  static constexpr int DEFAULT_PORT = 1883;

  MQTT(StatusDisplay& display, const char* const server_hostname, int server_port = DEFAULT_PORT) :
    _display(display),
    _serverHostname(server_hostname), _serverPort(server_port),
    _mqttWifi(), _mqttClient(_mqttWifi),
    _clientId(),
    _connCheckTask(MQTT_CONNCHECK_INTERVAL_SEC * 1000, TASK_FOREVER, std::bind(&MQTT::_connCheckCb, this)),
    _connectTask(MQTT_CONNECT_RETRY_INTERVAL_MILLIS, MQTT_CONNECT_RETRIES, std::bind(&MQTT::_connectCb, this)),
    _msgQueueFlushTask(MQTT_PUBLISH_RETRY_INTERVAL_MILLIS, TASK_FOREVER, std::bind(&MQTT::_msgQueueFlushCb, this))
  { }
  ~MQTT() { }

  void begin(Scheduler* sched);
  inline void loop() {
    _mqttClient.loop();
  }

  void connect();
  inline bool connected() {
    return _mqttClient.connected();
  }

  bool publish(const char* topic, const char* payload, bool retained = false, bool retry = true);

  bool add_subscription(const char* topic, MQTTCallback callback, uint8_t qos = 0);
  inline bool add_subscription(const String& topic, MQTTCallback callback, uint8_t qos = 0) {
    return add_subscription(topic.c_str(), callback, qos);
  }

  bool remove_subscription(const char* topic);
  inline bool remove_subscription(const String& topic) {
    return remove_subscription(topic.c_str());
  }

private:
  // Element type for message retry queue
  class Message {
  public:
    String topic;
    String payload;
    bool retained;

    Message() :
      topic((const char *)nullptr), payload((const char*)nullptr), retained(false)
    { } 

    Message(const char* const topic_, const char* const payload_, bool retained_) :
      topic(topic_), payload(payload_), retained(retained_) 
    { }

    Message(const String& topic_, const String& payload_, bool retained_) :
      topic(topic_), payload(payload_), retained(retained_)
    { }

    Message(String&& topic_, String&& payload_, bool retained_) :
      topic(std::forward<String>(topic_)), payload(std::forward<String>(payload_)), retained(retained_)
    { }

    // Deque elements must be CopyAssignable and CopyConstructible; we also add MoveAssignable
    Message(const Message& msg) = default;
    Message(Message&& msg) = default;
    Message& operator= (const Message& msg) = default;
    Message& operator= (Message&& msg) = default;
  };

  StatusDisplay& _display;

  const char* const _serverHostname;
  const int _serverPort;

  WiFiClient _mqttWifi;
  PubSubClient _mqttClient;
  String _clientId;

  Task _connCheckTask;
  Task _connectTask;
  Task _msgQueueFlushTask;

  std::unordered_map<String, std::pair<MQTTCallback, uint8_t> > _subscriptionsMap;
  ring_buffer<Message, MQTT_PUBLISH_RETRY_QUEUE_CAPACITY> _msgQueue;

  void _connCheckCb();
  void _connectCb();

  void _msgQueueFlushCb();

  void _mqttMessageCb(const char *topic, const byte *payload, unsigned int length);
};

#endif  /* __MQTT_H__ */