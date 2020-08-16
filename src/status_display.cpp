#include <Arduino.h>
#include "config.h"

#include "status_display.h"

void StatusDisplay::begin() {
  _u8.begin();
  //_u8.setFlipMode(1);
  _u8.setPowerSave(0);
  _u8.clearDisplay();
}

void StatusDisplay::temperature(double value) {
  _u8.setFont(u8x8_font_px437wyse700b_2x2_f);
  _u8.setCursor(0, 0);
  _u8.printf("%4.1f\xb0", value);
}

void StatusDisplay::time(int hr, int min) {
  _u8.setFont(u8x8_font_px437wyse700b_2x2_f);
  _u8.setCursor(0, 2);
  _u8.printf("%2d:%02d", hr, min);
}

void StatusDisplay::relay(bool show) {
  _u8.setFont(u8x8_font_open_iconic_embedded_2x2);
  _u8.drawString(12, 0, show ? "\x43" : " ");
}

void StatusDisplay::valve(bool show) {
  _u8.setFont(u8x8_font_open_iconic_thing_2x2);
  _u8.drawString(14, 0, show ? "\x48" : " ");
}

void StatusDisplay::wifi(display_status_t status) {
  const uint8_t* font = u8x8_font_open_iconic_embedded_2x2;
  char s[2] = " ";
  switch (status) {
  case STATUS_DISPLAY_INACTIVE:
    font = u8x8_font_open_iconic_check_2x2;
    s[0] = '\x44';
    break;
  case STATUS_DISPLAY_ACTIVE:
    s[0] = '\x50';
    break;
  case STATUS_DISPLAY_ACTIVATING:
    font = u8x8_font_open_iconic_arrow_2x2;
    s[0] = '\x56';
    break;
  case STATUS_DISPLAY_NONE:
  default:
      break;
  }
  _u8.setFont(font);
  _u8.drawString(12, 2, s);
}

void StatusDisplay::mqtt(display_status_t status) {
  const uint8_t* font = u8x8_font_open_iconic_thing_2x2;
  char s[2] = " ";
  switch (status) {
  case STATUS_DISPLAY_INACTIVE:
    font = u8x8_font_open_iconic_check_2x2;
    s[0] = '\x44';
    break;
  case STATUS_DISPLAY_ACTIVE:
    s[0] = '\x49';
    break;
  case STATUS_DISPLAY_ACTIVATING:
    font = u8x8_font_open_iconic_arrow_2x2;
    s[0] = '\x56';
    break;
  case STATUS_DISPLAY_NONE:
  default:
    break;
  }
  _u8.setFont(font);
  _u8.drawString(14, 2, s);
}