#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include <map>
#include <mutex>
#include "fd_event.h"
#include "timer_event.h"

namespace rocket {

class Timer : public FdEvent {
public:
  Timer(int fd);

  void addTimerEvent(TimerEvent::s_ptr event);
  void deleteTimerEvent(TimerEvent::s_ptr event);
  void onTimer(); // 当发生了 IO 事件后，EventLoop 会执行这个回调函数

private:
  void resetArriveTime();

private:
  std::multimap<int64_t, TimerEvent::s_ptr> m_pending_events;
  std::mutex m_mutex; // 替换为标准库互斥锁
};

}

#endif