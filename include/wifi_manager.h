#ifndef __WIFI_MANAGER_H__
#define __WIFI_MANAGER_H__

#include <vector>
#include <functional>

#include "config.h"

#include <WiFi.h>
#include <PubSubClient.h>

#include "task_scheduler.h"
#include "status_display.h"

typedef enum {WIFI_EVENT_CONNECTED, WIFI_EVENT_DISCONNECTED} wifi_event_t;

typedef std::function<void (wifi_event_t)> WiFiCallback;

class WiFiManager {
public:
  WiFiManager(StatusDisplay& display) : 
    _display(display),
    _connCheckTask(WIFI_CONNCHECK_INTERVAL_SEC * 1000, TASK_FOREVER, std::bind(&WiFiManager::_connCheckCb, this)),
    _wifiConnectTask(WIFI_CONNECT_INTERVAL_MILLIS, WIFI_CONNECT_ITERATIONS + 1, std::bind(&WiFiManager::_wifiConnectCb, this))
  { }
  ~WiFiManager() { }

  void begin(Scheduler* sched);

  void connect();
  void add_handler(wifi_event_t event, WiFiCallback callback);

private:
  StatusDisplay& _display;

  Task _connCheckTask;    // Should start once wifi is connected
  Task _wifiConnectTask;  // Initiated by either connect() or by connection check task

  std::vector<WiFiCallback> _connectHandlers;
  std::vector<WiFiCallback> _disconnectHandlers;

  void _connCheckCb();
  void _wifiConnectCb();
};

#endif  /* __WIFI_MANAGER_H__ */