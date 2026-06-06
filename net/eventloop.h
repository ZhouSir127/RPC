#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H

#include <unordered_set>
#include <functional>
#include <queue>
#include <mutex>
#include <thread>
#include <memory>   // For std::unique_ptr
#include "fd_event.h"
#include "wakeup_fd_event.h"
#include "timer.h"

namespace rocket {

class EventLoop {
 public:
  // 核心：提供获取当前线程独占单例的静态入口
  static EventLoop& GetCurrentEventLoop();

  // 禁用拷贝和移动语义，捍卫单例的唯一性
  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;
  EventLoop(EventLoop&&) = delete;
  EventLoop& operator=(EventLoop&&) = delete;

  void loop();
  void wakeup();
  void stop();

  void addEpollEvent(FdEvent* event);
  void deleteEpollEvent(FdEvent* event);

  void addTask(std::function<void()> cb, bool is_wake_up = false);
  void addTimerEvent(TimerEvent::s_ptr event);
  bool isLooping() const;

 private:
  // 单例模式规范：将构造和析构函数完全私有化！
  EventLoop();
  ~EventLoop();

  void initWakeUpFdEvent();
  void initTimer();
  void addEpollEventTask(FdEvent* event);
  void deleteEpollEventTask(FdEvent* event);

 private:
  std::thread::id m_thread_id;
  int m_epoll_fd;
  int m_wakeup_fd;

  std::unique_ptr<WakeUpFdEvent> m_wakeup_fd_event;
  std::unique_ptr<Timer> m_timer;

  bool m_stop_flag {false};
  bool m_is_looping {false};

  std::unordered_set<int> m_listen_fds;

  std::queue<std::function<void()>> m_pending_tasks;
  mutable std::mutex m_mutex;
};

}

#endif