#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include "config.h"

#include "button.h"

void MomentaryButton::begin(Scheduler* sched) {
  pinMode(_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(_pin), std::bind(&MomentaryButton::_isr, this), FALLING);
  sched->addTask(_debounceTask);
}

void MomentaryButton::_isr() {
  _debounceTask.restartDelayed(_debounceMillis);
}

void MomentaryButton::_debounceTaskCb() {
  if (digitalRead(_pin) == LOW) {
    _cb();
  }
}
