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
  void addTimerEvent(TimerEvent event);
  void deleteTimerEvent(const TimerEvent& event);
  void onTimer(); // 当发生了 IO 事件后，EventLoop 会执行这个回调函数
  void resetTimer();

private:
  std::multimap<int64_t, TimerEvent> m_pending_events;
  std::mutex m_mutex; // 替换为标准库互斥锁
  int64_t m_arrive_time {0};
};

}

#endif