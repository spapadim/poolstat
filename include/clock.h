#ifndef __CLOCK_H__
#define __CLOCK_H__

#include "task_scheduler.h"
#include <functional>

#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include "status_display.h"

class Clock {
public:
  static const int DEFAULT_TIME_OFFSET = -14400;  // UTC-4 (includes DST)

  Clock (StatusDisplay& display, int time_offset = DEFAULT_TIME_OFFSET) :
    _display(display), _time_offset(time_offset), 
    _udp(), _ntp(_udp, time_offset),
    _updateTask(1000, TASK_FOREVER, std::bind(&Clock::update, this))
  { }

  ~Clock() { }

  void begin(Scheduler* sched);
  inline void loop() {
    _ntp.update();
  }

  void update();

private:
  StatusDisplay& _display;
  int _time_offset;

  WiFiUDP _udp;
  NTPClient _ntp;

  Task _updateTask;
};

#endif  /* __CLOCK_H__ */