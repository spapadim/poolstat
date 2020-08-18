#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <vector>
#include <functional>

#include "config.h"

#include <WiFi.h>
#include <PubSubClient.h>

#include "sigslot.h"

#include "task_scheduler.h"
#include "status_display.h"

class WiFiManager {
public:
  typedef enum {EVENT_CONNECTED, EVENT_DISCONNECTED} event_t;
  using signal_t = ch::signal<event_t>;
  using callback_t = signal_t::slot_t;

  WiFiManager(StatusDisplay& display) : 
    _display(display),
    _connCheckTask(WIFI_CONNCHECK_INTERVAL_SEC * 1000, TASK_FOREVER, std::bind(&WiFiManager::_connCheckCb, this)),
    _wifiConnectTask(WIFI_CONNECT_INTERVAL_MILLIS, WIFI_CONNECT_ITERATIONS + 1, std::bind(&WiFiManager::_wifiConnectCb, this))
  { }
  ~WiFiManager() { }

  void begin(Scheduler* sched);

  void connect();

  signal_t connectSignal;
  signal_t disconnectSignal;

private:
  StatusDisplay& _display;

  Task _connCheckTask;    // Should start once wifi is connected
  Task _wifiConnectTask;  // Initiated by either connect() or by connection check task

  void _connCheckCb();
  void _wifiConnectCb();
};

#endif  /* __WIFI_MANAGER_H__ */