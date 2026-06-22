#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>
#include "timer.h"
#include "../common/log.h"
#include "../common/util.h"

namespace rocket {

Timer::Timer(int fd) : FdEvent(fd) {
  DEBUGLOG("timer fd=%d", m_fd);
  // 优化：放弃旧的 std::bind，使用现代 C++ Lambda 表达式，不仅可读性强而且执行更快
  setCallback(EPOLLIN, [this]() {
    onTimer();
  });
}

void Timer::onTimer() {
  // 优化：标准的 eventfd/timerfd 清除缓冲区的读法，必须使用 uint64_t
  uint64_t val;
  while (read(m_fd, &val, sizeof(val)) != -1 && errno != EAGAIN) {
  }

  int64_t now = getNowMs();
  std::vector<TimerEvent::s_ptr> tmps;
  std::vector<std::function<void()>> tasks;

  {
    // 替换为标准的 std::unique_lock
    std::unique_lock<std::mutex> lock(m_mutex);
    
    auto it = m_pending_events.begin();
    while (it != m_pending_events.end() && it->first <= now) {
      if (!it->second->isCanceled()) {
        tmps.push_back(it->second);
        // 优化：直接存回调，不再需要 std::pair，节省内存
        tasks.push_back(it->second->getCallBack()); 
      }
      ++it;
    }
    // 批量删除已到期的事件
    m_pending_events.erase(m_pending_events.begin(), it);
  } // 提前释放锁

  // 处理重复任务：需要重新调整时间并加回红黑树
  for (auto& event : tmps) {
    if (event->isRepeated()) {
      event->resetArriveTime();
      addTimerEvent(event); 
    }
  }

  // 因为我们弹出了节点，导致最小的时间戳变了，需要重置底层的定时器硬件触发时间
  resetArriveTime();

  // 执行业务逻辑（严格在锁外部执行，防止死锁或阻塞其他线程添加定时器）
  for (auto& task : tasks) {
    if (task) {
      task();
    }
  }
}

void Timer::resetArriveTime() {
  int64_t next_arrive_time = 0;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_pending_events.empty()) {
      return;
    }
    // 🚀 核心修复：坚决不进行 map 拷贝，直接 $O(1)$ 取出红黑树顶部的最小时间戳！
    next_arrive_time = m_pending_events.begin()->second->getArriveTime();
  } // 拿到时间戳后立刻释放锁

  int64_t now = getNowMs();
  int64_t interval = 0;
  if (next_arrive_time > now) {
    interval = next_arrive_time - now;
  } else {
    // 如果算出来的时间在过去，立刻设置 100ms 兜底缓冲，防止立即触发导致的死循环
    interval = 100; 
  }

  itimerspec value;
  memset(&value, 0, sizeof(value));
  value.it_value.tv_sec = interval / 1000;
  value.it_value.tv_nsec = (interval % 1000) * 1000000;

  int rt = timerfd_settime(m_fd, 0, &value, nullptr);
  if (rt != 0) {
    ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
  }
}

void Timer::addTimerEvent(TimerEvent::s_ptr event) {
  bool is_reset_timerfd = false;

  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_pending_events.empty()) {
      is_reset_timerfd = true;
    } else {
      // 如果新加进来的定时器比当前树里的所有定时器都早，就必须重置底层硬件定时器
      auto it = m_pending_events.begin();
      if (it->second->getArriveTime() > event->getArriveTime()) {
        is_reset_timerfd = true;
      }
    }
    m_pending_events.emplace(event->getArriveTime(), event);
  } // 提前释放锁

  if (is_reset_timerfd) {
    resetArriveTime();
  }
}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event) {
  event->setCanceled(true);

  std::unique_lock<std::mutex> lock(m_mutex);
  
  // 缩小查找范围：利用 upper 和 lower bound 极速锁定时间戳所在的区间
  auto begin = m_pending_events.lower_bound(event->getArriveTime());
  auto end = m_pending_events.upper_bound(event->getArriveTime());

  auto it = begin;
  for (it = begin; it != end; ++it) {
    if (it->second == event) {
      break;
    }
  }

  if (it != end) {
    m_pending_events.erase(it);
  }
}

}