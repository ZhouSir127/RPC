#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include <map>
#include <mutex>
#include "fd_event.h"
#include "timer_event.h"

namespace rocket {

class Timer : public FdEvent {
public:
  Timer();
  ~Timer();
  void addTimerEvent(std::shared_ptr<TimerEvent> event);
  void deleteTimerEvent(std::shared_ptr<TimerEvent> event);
  void onTimer(); // 当发生了 IO 事件后，EventLoop 会执行这个回调函数

private:
  void resetArriveTime();

private:
  std::multimap<int64_t, std::shared_ptr<TimerEvent>> m_pending_events;
  std::mutex m_mutex; // 替换为标准库互斥锁
};

}

#endif