// Global configuration settings
// *MUST* be included immediately after "Arduino.h"

#define MQTT_REALM "pool"
//#define MQTT_REALM "test"

// #define USE_REMOTEDEBUG
#define HAS_WATER_REFILL
//#define USE_ADS1115


/***************************************************************************
 *  Derived, non-editable settings
 ***************************************************************************/

#ifdef USE_ADS1115
#define THERMISTOR_CONFIG_ADS1115  // For "Thermistor.h"
#endif

#ifdef USE_REMOTEDEBUG
#  include <RemoteDebug.h>
// TODO FIXME Avoid double-eval of args!
#  define DEBUG_BEGINGROUP        rdbg.print("DBG:  ")
#  define DEBUG_PRINT(val)        rdbg.print(val)
#  define DEBUG_PRINTF(fmt, ...)  rdbg.printf(fmt, ##__VA_ARGS__)
#  define DEBUG_ENDGROUP          rdbg.println()
#  define DEBUG_MSG(fmt, ...)   { rdbg.print("DBG:  "); rdbg.printf(fmt, ##__VA_ARGS__); rdbg.println(); }

#  define INFO_MSG(fmt, ...)    { rdbg.print("INFO: "); rdbg.printf(fmt, ##__VA_ARGS__); rdbg.println(); }
#else
#  define DEBUG_BEGINGROUP        Serial.print("DBG:  ")
#  define DEBUG_PRINT(val)        Serial.print(val)
#  define DEBUG_PRINTF(fmt, ...)  Serial.printf(fmt, ##__VA_ARGS__)
#  define DEBUG_ENDGROUP          Serial.println()
#  define DEBUG_MSG(fmt, ...)   { Serial.print("DBG:  "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); }

#  define INFO_MSG(fmt, ...)    { Serial.print("INFO: "); Serial.printf(fmt, ##__VA_ARGS__); Serial.println(); }
#endif
