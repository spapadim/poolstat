#ifndef __BUTTON_H__
#define __BUTTON_H__

#include <functional>

#include "config.h"
#include "task_scheduler.h"

typedef std::function<void ()> MomentaryButtonCallback;

// TODO - Only supports NO momentary buttons
class MomentaryButton {
public:
  static const int DEFAULT_DEBOUNCE_MILLIS = 300;
  MomentaryButton(int pin, MomentaryButtonCallback callback, int debounce_millis = DEFAULT_DEBOUNCE_MILLIS) :
    _pin(pin), _debounceMillis(debounce_millis), _cb(callback),
    _debounceTask(debounce_millis, TASK_ONCE, std::bind(&MomentaryButton::_debounceTaskCb, this))
  { }
  ~MomentaryButton() { }

  void begin(Scheduler* sched);

private:
  const int _pin;
  const int _debounceMillis;

  MomentaryButtonCallback _cb;

  Task _debounceTask;

  void _isr();
  void _debounceTaskCb();
};

#endif  /* __BUTTON_H___ */