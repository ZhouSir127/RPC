#ifndef ROCKET_NET_TIMEREVENT_H
#define ROCKET_NET_TIMEREVENT_H

#include <functional>
#include <memory>
#include <cstdint>
#include "../common/util.h"

namespace rocket {

class TimerEvent {
public:
  TimerEvent(int interval, bool is_repeated, std::function<void()>cb)
  :m_interval(interval), m_is_repeated(is_repeated), m_task(std::move(cb)),m_arrive_time(getNowMs()+interval){}

  int64_t getArriveTime() const { return m_arrive_time; }
  void setCanceled(bool value) { m_is_canceled = value; }
  bool isCanceled() const { return m_is_canceled; }
  bool isRepeated() const { return m_is_repeated; }
  const std::function<void()>& getCallBack() const { return m_task; }
  void resetArriveTime() {m_arrive_time = getNowMs() + m_interval;}

 private:
  int64_t m_interval {0};       // ms
  bool m_is_repeated {false};
  std::function<void()> m_task{nullptr};
  int64_t m_arrive_time {0};    // ms
  bool m_is_canceled {false};
};

}

#endif