#ifndef ROCKET_NET_TIMEREVENT_H
#define ROCKET_NET_TIMEREVENT_H

#include <functional>
#include <memory>
#include <cstdint>

namespace rocket {

class TimerEvent {
 public:
  typedef std::shared_ptr<TimerEvent> s_ptr;

  TimerEvent(int interval, bool is_repeated, std::function<void()> cb);

  int64_t getArriveTime() const { return m_arrive_time; }
  void setCanceled(bool value) { m_is_canceled = value; }
  bool isCanceled() const { return m_is_canceled; }
  bool isRepeated() const { return m_is_repeated; }
  std::function<void()> getCallBack() const { return m_task; }

  void resetArriveTime();

 private:
  int64_t m_arrive_time {0};    // ms
  int64_t m_interval {0};       // ms
  bool m_is_repeated {false};
  bool m_is_canceled {false};   // 修正拼写 Cancled -> Canceled

  std::function<void()> m_task;
};

}

#endif