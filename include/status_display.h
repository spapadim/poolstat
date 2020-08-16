#ifndef __STATUS_DISPLAY_H__
#define __STATUS_DISPLAY_H__

#include <U8x8lib.h>

// Enum for multi-state indicators. DISPLAY_NONE should always correspond to a blank icon.
// The meaning of other enum values are incator-dependent; e.g., for wifi indicator we'd have:
//   inactive: disconnected icon
//   active: connected icon
//   activating: connecting icon
typedef enum { DISPLAY_INACTIVE, DISPLAY_ACTIVE, DISPLAY_ACTIVATING, DISPLAY_NONE } display_status_t;

class StatusDisplay {
public:
  StatusDisplay() : _u8(U8X8_PIN_NONE, SCL, SDA) { }
  ~StatusDisplay() { }

  void begin();

  void temperature(double value);
  void time(int hr, int min);

  void relay(bool show);
  void valve(bool show);

  // TODO - Make next two tri-state (disconnected/connected/connecting) or quad-state (../failed)
  void wifi(bool show);
  void mqtt(bool show);

private:
  U8X8_SSD1306_128X32_UNIVISION_HW_I2C _u8;
};


#endif  /* __STATUS_DISPLAY_H__ */