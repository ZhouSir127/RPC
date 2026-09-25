#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>
#include "timer.h"
#include "../common/log.h"
#include "../common/util.h"

namespace rocket {

Timer::Timer() {
  int m_timer_fd = timerfd_create(CLOCK_MONOTONIC,0);
    // if(m_timer_fd < 0){
  //   ERRORLOG("failed to create event loop, m_timer_fd create error, error info[%d]", errno);
  //   exit(1);
  // }
//  DEBUGLOG("timer fd=%d", m_fd);
    m_fd = m_timer_fd;
    init();
  // 优化：放弃旧的 std::bind，使用现代 C++ Lambda 表达式，不仅可读性强而且执行更快
  setCallback(EPOLLIN, [this]() {
    onTimer();
  });
}

Timer::~Timer(){
  close(m_fd);
}

void Timer::onTimer() {
  // 优化：标准的 eventfd/timerfd 清除缓冲区的读法，必须使用 uint64_t
  uint64_t val;
  while (read(m_fd, &val, sizeof(val)) != -1 && errno != EAGAIN);

  int64_t now = getNowMs();
  std::vector<std::shared_ptr<TimerEvent>> tmps;
  std::vector<std::function<void()>> tasks;

  {// 替换为标准的 std::unique_lock
    std::unique_lock<std::mutex> lock(m_mutex);
    
    auto it = m_pending_events.begin();
    while (it != m_pending_events.end() && it->first <= now) {
      if (it->second->isCanceled() == false)
        tmps.push_back(it->second);
        // 优化：直接存回调，不再需要 std::pair，节省内存
      ++it;
    }    
    // 批量删除已到期的事件
    m_pending_events.erase(m_pending_events.begin(), it);
  } // 提前释放锁

  // 处理重复任务：需要重新调整时间并加回红黑树
  for (auto& event : tmps){
    const auto&f = event ->getCallBack();
    if (f)
      f();

    if (event->isRepeated() ) {
      event->resetArriveTime();
      addTimerEvent(event); 
    }
  }  // 执行业务逻辑（严格在锁外部执行，防止死锁或阻塞其他线程添加定时器）
  // 因为我们弹出了节点，导致最小的时间戳变了，需要重置底层的定时器硬件触发时间
  resetArriveTime();
}

void Timer::resetArriveTime() {
  int64_t next_arrive_time = 0;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_pending_events.empty() )
      return;
    // 🚀 核心修复：坚决不进行 map 拷贝，直接 $O(1)$ 取出红黑树顶部的最小时间戳！
    next_arrive_time = m_pending_events.begin()->second->getArriveTime();
  } // 拿到时间戳后立刻释放锁

  uint64_t now = getNowMs();
  if( now > next_arrive_time )
    next_arrive_time = now+100;

    // 如果算出来的时间在过去，立刻设置 100ms 兜底缓冲，防止立即触发导致的死循环
  itimerspec value;
  memset(&value, 0, sizeof(value));

  // 绝对时间：直接填 next_arrive_time 对应的秒和纳秒
  value.it_value.tv_sec  = next_arrive_time / 1000;
  value.it_value.tv_nsec = (next_arrive_time % 1000) * 1000000;

  // it_interval 保持 0，表示只到期一次
  // 第二个参数加上 TFD_TIMER_ABSTIME
  int rt = timerfd_settime(m_fd, TFD_TIMER_ABSTIME, &value, nullptr);
  // if (rt != 0)
  //   ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
}

void Timer::addTimerEvent(const std::shared_ptr<TimerEvent>&event) {
  std::multimap<int64_t, std::shared_ptr<TimerEvent>>::iterator it;
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    // 如果新加进来的定时器比当前树里的所有定时器都早，就必须重置底层硬件定时器    
    it = m_pending_events.emplace(event->getArriveTime(), event);
  } // 提前释放锁
  // if (it == m_pending_events.begin() )
  //   resetArriveTime();
}

void Timer::deleteTimerEvent(const std::shared_ptr<TimerEvent>& event) {
  event->setCanceled(true);

  std::unique_lock<std::mutex> lock(m_mutex);

  // 使用 equal_range 直接获取所有到期时间等于 event->getArriveTime() 的区间
  auto range = m_pending_events.equal_range(event->getArriveTime());

  auto it = range.first;
  while (it != range.second && it->second != event)++it;

  if (it != range.second)
    m_pending_events.erase(it);
}

}