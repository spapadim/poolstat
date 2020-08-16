#include <Arduino.h>
#include "config.h"

#include "clock.h"

void Clock::begin(Scheduler* sched) {
  _ntp.begin();
  //_ntp.setTimeOffset(_time_offset);
  DEBUG_MSG("Clock initialized");

  // Add and enable update tasks
  sched->addTask(_updateTask);
  _updateTask.enable();
  DEBUG_MSG("Clock task enabled");
}

void Clock::update() {
  int hr = _ntp.getHours();
  int min = _ntp.getMinutes();
  //DEBUG_MSG("Clock::update() -> %2d:%02d", hr, min)
  _display.time(hr, min);
}