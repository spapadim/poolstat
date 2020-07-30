#include <Arduino.h>
#include "config.h"  // Must be included immediately after "Arduino.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>

#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <U8x8lib.h>

#ifdef USE_ADS1115
#include <Adafruit_ADS1015.h>
#define THERMISTOR_CONFIG_ADS1115   // Must define before including <Thermistor.h>
#endif

#include <Thermistor.h>

#include "secrets.h"

/***************************************************************************
 *
 ***************************************************************************/

static const char* WIFI_SSID = SECRET_WIFI_SSID;
static const char* WIFI_PASSWORD = SECRET_WIFI_PASSWORD;

static const char* MDNS_HOSTNAME = MQTT_REALM;

static const char* MQTT_SERVER = "thor.clusterhack.net";
static const int MQTT_PORT = 1883;
static const char* MQTT_CLIENT_ID_PREFIX = "poolstat-";
static const char* MQTT_TOPIC_HEATER_CONTROL = MQTT_REALM "/heater/control";  // W
static const char* MQTT_TOPIC_RELAY_STATE = MQTT_REALM "/relay/state";  // R 
static const char* MQTT_TOPIC_RELAY_CONTROL = MQTT_REALM "/relay/control";  // W
static const char* MQTT_TOPIC_MAIN_TEMP = MQTT_REALM "/main/temperature";  // R
static const char* MQTT_TOPIC_MAIN_SETPOINT = MQTT_REALM "/main/setpoint";  // W
#ifdef HAS_HEAT_EXCHANGER
static const char* MQTT_TOPIC_EXCHANGER_TEMP = MQTT_REALM "/exchanger/temperature";  // R
static const char* MQTT_TOPIC_EXCHANGER_SETPOINT = MQTT_REALM "/exchanger/setpoint";  // W
#endif
#ifdef HAS_WATER_REFILL
static const char* MQTT_TOPIC_REFILL_CONTROL = MQTT_REALM "/refill/control";  // W
static const char* MQTT_TOPIC_VALVE_STATE = MQTT_REALM "/valve/state";  // R
static const char* MQTT_TOPIC_VALVE_CONTROL = MQTT_REALM "/valve/control";  // R
static const char* MQTT_TOPIC_WATER_LEVEL = MQTT_REALM "/waterlevel";  // R
#endif

static const int NTP_TIME_OFFSET = -14400;  // UTC-4 (includes DST)

#ifdef USE_ADS1115
static const int MAIN_THERMISTOR_CHANNEL = 2;  // on ADS1115
#  ifdef HAS_HEAT_EXCHANGER
static const int EXCHANGER_THERMISTOR_CHANNEL = 0;  // on ADS1115
#  endif
#else  // i.e., not USE_ADS1115
static const int MAIN_THERMISTOR_PIN = 34; // ESP32 pin D34 == A6
#  ifdef HAS_HEAT_EXCHANGER
static const int EXCHANGER_THERMISTOR_PIN = 35;  // ESP32 pin D35 == A7
#  endif

static const uint8_t ADC_RESOLUTION_BITS = 12;
static const adc_attenuation_t ADC_ATTENUATION = ADC_6db;
// XXX The theoretical attenuation factor for 6db is 1/3 (0.66667), but
//   empirically, the ADC seems to under-shoot by about 10%, at
//   least at the temperature ranges of interest for pool water...
//   Rather than debug circuit, this is q uuick-n-dirty workaround.
static const double ADC_ATTENUATION_FACTOR = 0.60351;
#endif  // USE_ADS1115

static const int RELAY_PIN = 4;

static const int RELAY_TOGGLE_THRESHOLD_SEC = 60;

#ifdef HAS_WATER_REFILL
static const int VALVE_PIN = 2;
static const int WATERLEVEL_HI_PIN = 18;  /* TODO */
static const int WATERLEVEL_MID_PIN = 19;  /* TODO */

static const int VALVE_TOGGLE_THRESHOLD_SEC = 300;
#endif

// All temperatures in Fahrenheit
static const float SETPOINT_MAIN_DEFAULT = 87.0;
static const float SETPOINT_MAIN_OVERSHOOT = 0.5;
static const float SETPOINT_MAIN_UNDERSHOOT = 0.3;
#ifdef HAS_HEAT_EXCHANGER 
static const float SETPONT_EXCHANGER_DEFAULT = 195.0;
static const float SETPOINT_EXCHANGER_OVERSHOOT = 0.5;
static const float SETPOINT_EXCHANGER_UNDERSHOOT = 1.0;
#endif

static const int DISPLAY_UPDATE_INTERVAL_SEC = 3;
static const int MQTT_UPDATE_INTERVAL_SEC = 30;
static const int THERMOSTAT_UPDATE_INTERVAL_SEC = 5;

// TODO Use u8x8.get{Rows,Cols}()
// TODO the "16" is implicitly hardcoded in several places below
static const int DISPLAY_WIDTH_CHARS = 16;
static const int DISPLAY_HEIGHT_CHARS = 4;

/***************************************************************************
 *
 ***************************************************************************/

static WiFiClient mqttWifiClient;
static PubSubClient mqttClient(mqttWifiClient);

static WiFiUDP ntpWifiUDP;
static NTPClient ntpClient(ntpWifiUDP);

#ifdef USE_REMOTEDEBUG
RemoteDebug rdbg;
#endif


U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(  /* TODO? */
  U8X8_PIN_NONE,  // reset 
  SCL,  // clock
  SDA  // data
);

#ifdef USE_ADS1115
static Adafruit_ADS1115 ads1115;
#endif

#ifdef USE_ADS1115
static ADS1115Port _mainThermistorPort(ads1115, MAIN_THERMISTOR_CHANNEL);
#else
static MCUPort _mainThermistorPort(MAIN_THERMISTOR_PIN, ADC_ATTENUATION_FACTOR);
#endif
static Thermistor mainTemperatureSensor(_mainThermistorPort, 10000.0, 10000.0, 25.0, 3950.0, 0.98);

#ifdef HAS_HEAT_EXCHANGER
#  ifdef USE_ADS1115
static ADS1115Port _exchangerThermistorPort(ads1115, EXCHANGER_THERMISTOR_CHANNEL);
#  else
static MCUPort _exchangerThermistorPort(EXCHANGER_THERMISTOR_PIN, ADC_ATTENUATION_FACTOR);
#  endif
static Thermistor exchangerTemperatureSensor(_exchangerThermistorPort, 10000.0, 10000.0, 25.0, 3950.0, 0.98);
#endif

/***************************************************************************
 *
 ***************************************************************************/

typedef enum { CONTROL_OFF, CONTROL_MANUAL, CONTROL_AUTO } control_state_t;

static control_state_t _parseControlState(String state) {
  if (state == "on" || state == "auto") {
    return CONTROL_AUTO;
  } else if (state == "manual") {
    return CONTROL_MANUAL;
  } 
  return CONTROL_OFF;
}

static control_state_t heaterControl = CONTROL_OFF;
#ifdef HAS_WATER_REFILL
static control_state_t refillControl = CONTROL_OFF;
#endif

static double mainTemperature;
static double mainSetpointHi = SETPOINT_MAIN_DEFAULT + SETPOINT_MAIN_OVERSHOOT;
static double mainSetpointLo = SETPOINT_MAIN_DEFAULT - SETPOINT_MAIN_UNDERSHOOT;
#ifdef HAS_HEAT_EXCHANGER
static double exchangerTemperature;
static double exchangerSetpointHi = SETPOINT_EXCHANGER_DEFAULT + SETPOINT_EXCHANGER_OVERSHOOT;
static double exchangerSetpointLo = SETPOINT_EXCHANGER_DEFAULT - SETPOINT_EXCHANGER_UNDERSHOOT;
#endif

#ifdef HAS_WATER_REFILL
typedef enum { LEVEL_LO, LEVEL_MID, LEVEL_HI, LEVEL_INVALID } level_t;
static level_t waterLevel;
#endif

/***************************************************************************
 *
 ***************************************************************************/

static void _setRelayOn(bool on, bool publish = true) {
  if (on) {
    digitalWrite(RELAY_PIN, HIGH);
    if (publish) {
      mqttClient.publish(MQTT_TOPIC_RELAY_STATE, "on");
      DEBUG_MSG("Heater relay ON");
    }
  } else {
    digitalWrite(RELAY_PIN, LOW);
    if (publish) {
      mqttClient.publish(MQTT_TOPIC_RELAY_STATE, "off");
      DEBUG_MSG("Heater relay OFF");
    }
  }
}

static inline bool _getRelayOn() {
  return digitalRead(RELAY_PIN);
}

#ifdef HAS_WATER_REFILL
static void _setValveOn(bool on, bool publish = true) {
  if (on) {
    digitalWrite(VALVE_PIN, HIGH);
    if (publish) {
      mqttClient.publish(MQTT_TOPIC_VALVE_STATE, "on");
      DEBUG_MSG("Valve ON");
    }
  } else {
    digitalWrite(VALVE_PIN, LOW);
    if (publish) {
      mqttClient.publish(MQTT_TOPIC_VALVE_STATE, "off");
      DEBUG_MSG("Valve OFF");
    }
  }
}

static inline bool _getValveOn() {
  return digitalRead(VALVE_PIN);
}
#endif
/***************************************************************************
 *
 ***************************************************************************/

static void display_setup() {
  u8x8.begin();
  u8x8.setFlipMode(1);
  u8x8.setPowerSave(0);
  //u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setFont(u8x8_font_pxplusibmcgathin_f);

  // TODO - "HELLO!" message?

  DEBUG_MSG("Display ready");
}

static unsigned long display_last_update = millis();
static void display_update() {
  if (millis() - display_last_update < DISPLAY_UPDATE_INTERVAL_SEC * 1000)
    return;

  display_last_update = millis();

  // Temperature and relay status
  bool relayOn = _getRelayOn();
  u8x8.clearLine(0);
#ifdef HAS_WATER_REFILL
  u8x8.setCursor(2, 0);
  u8x8.printf("%4.1f'F H%c V%c", mainTemperature, relayOn ? '+' : '_', _getValveOn() ? '+' : '_');
#else
  u8x8.setCursor(4, 0);
  u8x8.printf("%4.1f'F H%c", mainTemperature, relayOn ? '+' : '_');
#endif
  u8x8.clearLine(1);
  u8x8.setCursor(3, 1);
  u8x8.printf("SET: %4.1f'F", relayOn ? mainSetpointHi : mainSetpointLo);

  // Current time
  u8x8.clearLine(3);
  u8x8.setCursor(5, 3);
  u8x8.printf("%02d:%02d", ntpClient.getHours(), ntpClient.getMinutes());
}

static const int WIFI_CONNECT_RETRY_INTERVAL_SEC = 5;

static unsigned long wifi_last_reconnect = millis();
static void wifi_reconnect(bool force = false) {
  unsigned long now = millis();
  if (now - wifi_last_reconnect < WIFI_CONNECT_RETRY_INTERVAL_SEC * 1000 && !force)
    return;
  wifi_last_reconnect = now;

  DEBUG_MSG("WiFi connecting to %s ", WIFI_SSID);
  // Display output
  u8x8.clearDisplay();
  u8x8.setCursor(0, 0);
  u8x8.printf("WiFi connecting");
  u8x8.setCursor(max(0, (DISPLAY_WIDTH_CHARS-(int)strlen(WIFI_SSID))/2), 1);
  u8x8.printf(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    DEBUG_MSG("WiFi connected");

    String ip = WiFi.localIP().toString();
    DEBUG_MSG("IP address: %s", ip.c_str());
    // Display output
    u8x8.clearLine(0);
    u8x8.setCursor(1, 0);
    u8x8.printf("WiFi connected");
    u8x8.setCursor(max(0, (DISPLAY_WIDTH_CHARS-(int)ip.length())/2), 2);
    u8x8.printf(ip.c_str());   
  } else {
    DEBUG_MSG("WiFi connection failed!");
    u8x8.clearLine(0);
    u8x8.setCursor(3, 0);
    u8x8.printf("WiFi failed");
  }
}

static void wifi_setup() {
  WiFi.persistent(false);
  //WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_STA);

  delay(100);
  wifi_reconnect(true);
}

static void ota_setup() {
  ArduinoOTA.setHostname(MDNS_HOSTNAME);
  ArduinoOTA.setPassword(SECRET_OTA_PASSWORD);

  // From BasicOTA example code
  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // TODO if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      DEBUG_BEGINGROUP;
      DEBUG_PRINT("OTA start: " + type);
      DEBUG_ENDGROUP;
    })
    .onEnd([]() {
      DEBUG_MSG("End\n");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      DEBUG_MSG("OTA progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      DEBUG_BEGINGROUP;
      DEBUG_PRINTF("OTA error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) DEBUG_PRINT("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) DEBUG_PRINT("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) DEBUG_PRINT("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINT("Receive Failed");
      else if (error == OTA_END_ERROR) DEBUG_PRINT("End Failed");
      DEBUG_ENDGROUP;
    });

  ArduinoOTA.begin();
}

#ifdef USE_REMOTEDEBUG
static void remotedebug_setup() {
  // Assumes MDNS already started; call after ota_setup()
  MDNS.addService("telnet", "tcp", 23);

  rdbg.begin(MDNS_HOSTNAME);
  rdbg.setSerialEnabled(true);
  rdbg.setResetCmdEnabled(true);
	//rdbg.showProfiler(true);
	rdbg.showColors(true);
  DEBUG_MSG("RemoteDebug ready...")
}
#endif

static void ntp_setup() {
  ntpClient.begin();
  ntpClient.setTimeOffset(NTP_TIME_OFFSET);
}

static void mqtt_callback(const char *topic, const byte *payload, unsigned int length) {
  // Safely copy payload bytes into a string
  String data((const char *)NULL);  // Needs cast, otherwise treats arg as (int)0 ?!
  data.reserve(length);  // We do not need to +1 for '\0'
  for (int i = 0;  i < length; i++)
    data.concat((char)(payload[i]));

  DEBUG_MSG("MQTT received: topic: '%s' / payload: '%s'", topic, data.c_str());

  // Process command
  if (!strcmp(topic, MQTT_TOPIC_HEATER_CONTROL)) {
    heaterControl = _parseControlState(data);
    _setRelayOn(false);
  }
  else if (!strcmp(topic, MQTT_TOPIC_RELAY_CONTROL)) {
    if (heaterControl == CONTROL_MANUAL) {
      _setRelayOn(data == "on");
    }
    // else, just ignore command
  }
#ifdef HAS_WATER_REFILL
  else if (!strcmp(topic, MQTT_TOPIC_REFILL_CONTROL)) {
    refillControl = _parseControlState(data);
    _setValveOn(false);
  }
  else if (!strcmp(topic, MQTT_TOPIC_VALVE_CONTROL)) {
    if (refillControl == CONTROL_MANUAL) {
      _setValveOn(data == "on");
    }
  }
#endif
  else if (!strcmp(topic, MQTT_TOPIC_MAIN_SETPOINT)) {
    float value = (float)atof(data.c_str());
    mainSetpointHi = value + SETPOINT_MAIN_OVERSHOOT;
    mainSetpointLo = value - SETPOINT_MAIN_UNDERSHOOT;
  }
#ifdef HAS_HEAT_EXCHANGER
  else if (!strcmp(topic, MQTT_TOPIC_EXCHANGER_SETPOINT)) {
    float value = (float)atof(data.c_str());
    exchangerSetpointHi = value + SETPOINT_EXCHANGER_OVERSHOOT;
    exchangerSetpointLo = value - SETPOINT_EXCHANGER_UNDERSHOOT;
  }
#endif
  else {
    DEBUG_MSG("Unknown topic: %s", topic);
  }
}

static const int MQTT_CONNECT_RETRY_INTERVAL_SEC = 5;

static unsigned long mqtt_last_reconnect = millis();
static void mqtt_reconnect(bool force = false) {
  unsigned long now = millis();
  if (now - mqtt_last_reconnect <= MQTT_CONNECT_RETRY_INTERVAL_SEC * 1000 && !force)
    return;
  mqtt_last_reconnect = now;

  u8x8.clearLine(3);
  u8x8.setCursor(0, 3);
  u8x8.printf("MQTT connecting");
  
  // Attempt to connect
  DEBUG_MSG("Attempting MQTT connection... ");
  String clientId(MQTT_CLIENT_ID_PREFIX);
  clientId += String(random(0xffff), HEX);
  if (mqttClient.connect(clientId.c_str())) {
    DEBUG_MSG("MQTT connected as %s", clientId.c_str());

    mqttClient.subscribe(MQTT_TOPIC_HEATER_CONTROL);
    mqttClient.subscribe(MQTT_TOPIC_RELAY_CONTROL);
    mqttClient.subscribe(MQTT_TOPIC_MAIN_SETPOINT);
#ifdef HAS_HEAT_EXCHANGER
    mqttClient.subscribe(MQTT_TOPIC_EXCHANGER_SETPOINT);
#endif
#ifdef HAS_WATER_REFILL
    mqttClient.subscribe(MQTT_TOPIC_REFILL_CONTROL);
    mqttClient.subscribe(MQTT_TOPIC_VALVE_CONTROL);
#endif

    u8x8.clearLine(3);
    u8x8.setCursor(1, 3);
    u8x8.printf("MQTT connected");
  } else {
    DEBUG_MSG("MQTT connect failed, rc=%d", mqttClient.state());

    u8x8.clearLine(3);
    u8x8.setCursor(3, 3);
    u8x8.printf("MQTT failed");
  }
}

static void mqtt_setup() {
  randomSeed(micros());
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(mqtt_callback);

  mqtt_reconnect(true);
}

static unsigned long mqtt_last_update = millis();
static void mqtt_update_values() {
  //if (!mqttClient.connected())
  //  return;

  // Periodically post current temperature values
  unsigned long now = millis();
  if (now - mqtt_last_update < MQTT_UPDATE_INTERVAL_SEC * 1000)
    return;
  mqtt_last_update = now;

  mqttClient.publish(MQTT_TOPIC_MAIN_TEMP, String(mainTemperature, 2).c_str());
  DEBUG_MSG("Published temperature %5.2fF", mainTemperature);
#ifdef HAS_HEAT_EXCHANGER
  mqttClient.publish(MQTT_TOPIC_EXCHANGER_TEMP, String(exchangerTemperature, 2).c_str());
  DEBUG_MSG("Published exchanger temperature %5.2fF", exchangerTemperature);
#endif

#ifdef HAS_WATER_REFILL
  switch (waterLevel) {
    case LEVEL_LO:
      mqttClient.publish(MQTT_TOPIC_WATER_LEVEL, "lo");
      break;
    case LEVEL_MID:
      mqttClient.publish(MQTT_TOPIC_WATER_LEVEL, "mid");
      break;
    case LEVEL_HI:
      mqttClient.publish(MQTT_TOPIC_WATER_LEVEL, "hi");
      break;
    case LEVEL_INVALID:
      mqttClient.publish(MQTT_TOPIC_WATER_LEVEL, "invalid");
      break;
  }
  DEBUG_MSG("Published water level %d", (int)waterLevel);
#endif
}

static void thermostat_setup() {
#ifdef USE_ADS1115
  ads1115.begin();
  ads1115.setGain(GAIN_ONE);
#else
  analogReadResolution(ADC_RESOLUTION_BITS);
  analogSetAttenuation(ADC_ATTENUATION);  // For all pins
  //analogSetPinAttenuation(A6, ADC_6db);
  //analogSetPinAttenuation(A7, ADC_6db);
  //pinMode(MAIN_THERMISTOR_PIN, ANALOG);
#  ifdef HAS_HEAT_EXCHANGER
  //pinMode(EXCHANGER_THERMISTOR_PIN, ANALOG);
#endif
#endif

  pinMode(RELAY_PIN, OUTPUT);
  _setRelayOn(false, mqttClient.connected());  // Better safe..

  DEBUG_MSG("Thermostat relay and sensors ready");
}

#ifdef HAS_WATER_REFILL
static void refill_setup() {
  // Solenoid valve switch
  pinMode(VALVE_PIN, OUTPUT);
  _setValveOn(false, mqttClient.connected());  // Better safe...

  // Water level sensor pins
  pinMode(WATERLEVEL_HI_PIN, INPUT_PULLUP);
  pinMode(WATERLEVEL_MID_PIN, INPUT_PULLUP);

  DEBUG_MSG("Refill valve and sensors ready")
}

static unsigned long valve_last_toggle = millis();
static void refill_update() {
  // Update water level; water presence will short pullup pin to ground
  bool hiClosed = !digitalRead(WATERLEVEL_HI_PIN);
  bool midClosed = !digitalRead(WATERLEVEL_MID_PIN);
  if (hiClosed && !midClosed)
    waterLevel = LEVEL_INVALID;
  else if (hiClosed)
    waterLevel = LEVEL_HI;
  else if (midClosed)
    waterLevel = LEVEL_MID;
  else
    waterLevel = LEVEL_LO;

  if (refillControl != CONTROL_AUTO)
    return;

  // Limit valve cycling frequency
  unsigned long now = millis();
  if (now - valve_last_toggle < VALVE_TOGGLE_THRESHOLD_SEC * 1000)
    return;

  bool valveOpen = _getValveOn();
  if (valveOpen && waterLevel == LEVEL_HI) {
    _setValveOn(false);
    valve_last_toggle = now;
  } else if (!valveOpen && waterLevel != LEVEL_HI) {
    _setValveOn(true);
    valve_last_toggle = now;
  }
}
#endif

static unsigned long thermostat_last_update = millis();
static unsigned long relay_last_toggle = millis();
static void thermostat_update() {
  // Limit update frequency, since readings take some time, due to averaging
  unsigned long now = millis();
  if (now - thermostat_last_update < THERMOSTAT_UPDATE_INTERVAL_SEC * 1000)
    return;
  thermostat_last_update = now;

  // Update temperature values
  mainTemperature = mainTemperatureSensor.fahrenheit();
#if HAS_HEAT_EXCHANGER
  exchangerTemperature = exchangerTemperatureSensor.fahrenheit();
#endif

  if (heaterControl != CONTROL_AUTO) 
    return;

  // Limit heater relay cycle frequency
  if (now - relay_last_toggle < RELAY_TOGGLE_THRESHOLD_SEC * 1000)
    return;

  // Update heater relay state
  if (_getRelayOn()) {
    bool turnOff = (mainTemperature > mainSetpointHi);
#if HAS_HEAT_EXCHANGER
    turnOff = turnOff || (exchangerTemperature > exchangerSetpointHi);
#endif
    if (turnOff) {
      _setRelayOn(false);
      relay_last_toggle = now;
    }
  } else {
    bool turnOn = (mainTemperature < mainSetpointLo);
#if HAS_HEAT_EXCHANGER
    turnOn = turnOn && (exchangerTemperature < exchangerSetpointLo);
#endif
    if (turnOn) {
      _setRelayOn(true);
      relay_last_toggle = now;
    }
  }
}

/***************************************************************************
 *
 ***************************************************************************/

void setup() {
  Serial.begin(9600);
  Serial.println("BOOT");
  display_setup();
  wifi_setup();
  mqtt_setup();
  thermostat_setup();
#ifdef HAS_WATER_REFILL
  refill_setup();
#endif
  ota_setup();
#ifdef USE_REMOTEDEBUG
  remotedebug_setup();
#endif
  ntp_setup();
}

void loop() {
  if (!WiFi.isConnected())  {
    wifi_reconnect();
    // TODO -- Do we also have to restart NTP and/or OTA?
  }
  if (!mqttClient.connected()) mqtt_reconnect();
  ntpClient.update();
  ArduinoOTA.handle();
#ifdef USE_REMOTEDEBUG
  rdbg.handle();
#endif
  mqttClient.loop();

  thermostat_update();
#ifdef HAS_WATER_REFILL
  refill_update();
#endif
  display_update();
  mqtt_update_values();

  //yield();
}