#include <Arduino.h>
#include "config.h"

#include <WiFi.h>
#include <PubSubClient.h>

#include "wifi_manager.h"


static const char* WIFI_SSID = SECRET_WIFI_SSID;
static const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;


void WiFiManager::begin(Scheduler* sched) {
  DEBUG_MSG("WiFiManager::begin()");
  
  // Set basic wifi config parameters
  WiFi.persistent(false);
  //WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);

  // Display off (no connection attempt yet)
  _display.wifi(STATUS_DISPLAY_NONE);

  // Add all tasks; need to be explicitly enable by calling eg ::connect()
  sched->addTask(_connCheckTask);
  sched->addTask(_wifiConnectTask);
}

void WiFiManager::connect() {
  // Do nothing if already connected
  if (WiFi.isConnected())
    return;

  DEBUG_MSG("WiFiManager::connect() starting");

  // Do nothing if already started connection attempt
  if (!_wifiConnectTask.isEnabled())
    _wifiConnectTask.restart();
}

void WiFiManager::_connCheckCb() {
  DEBUG_MSG("WiFiManager::_connCheckCb()");

  if (!WiFi.isConnected()) {
    DEBUG_MSG("WiFiManager detected disconnection; calling handlers");

    _display.wifi(STATUS_DISPLAY_INACTIVE);
    // Invoke _disconnected callbacks (remember, connection check task
    //  is only started upon a successful connection earlier)
    disconnectSignal.emit(EVENT_DISCONNECTED);

    // Initiate (re-)connection
    connect();
  }
}

void WiFiManager::_wifiConnectCb() {
  int count = _wifiConnectTask.getRunCounter() - 1;

  DEBUG_MSG("WiFiManager::_wifiConnectCb() count=%d", count);

  if (count == 0) {
    // Disable connection check task (if necessary) and update display
    _display.wifi(STATUS_DISPLAY_ACTIVATING);
    if (_connCheckTask.isEnabled()) {
      _connCheckTask.disable();
    }

    // Initiate wifi connection
    DEBUG_MSG("WiFiManager issuing WiFi.begin");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    return;  // No point checking status immediately..
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (count == _wifiConnectTask.getIterations() - 1) {
      _display.wifi(STATUS_DISPLAY_INACTIVE);  // Failed to connect, make sure indicator reflects this
    }
  } else {
    DEBUG_MSG("WiFi connected");
    String ip = WiFi.localIP().toString();
    DEBUG_MSG("IP address: %s", ip.c_str());

    // Update display
    _display.wifi(STATUS_DISPLAY_ACTIVE);

    DEBUG_MSG("WiFiManager (re-)connected; calling handlers");
    // Invoke _connected callbacks
    connectSignal.emit(EVENT_CONNECTED);

    // Finally, disable ourselves and (re-)enable connection check task
    _connCheckTask.enable();
    _wifiConnectTask.disable();
  }
}
