#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <sstream>
#include <sys/timerfd.h>
#include "eventloop.h"
#include "../common/log.h"
#include "../common/util.h"

namespace rocket {

constexpr int g_epoll_max_timeout = 10000;
constexpr int g_epoll_max_events = 10;

EventLoop& EventLoop::GetCurrentEventLoop() {
  thread_local static EventLoop instance;
  return instance;
}

inline void EventLoop::add(FdEvent* fdEvent) {
  int fd = fdEvent -> getFd();
  m_listen_fds.insert(fd);
  
  int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD , fd , fdEvent->getEpollEvent() );
  
  if (rt == -1){
    ERRORLOG("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno));
  }else{ 
    DEBUGLOG("add event success, fd[%d]", fd);
  }
}

inline void EventLoop::modify(FdEvent* fdEvent){
  int fd = fdEvent -> getFd();
  
  int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD , fd , fdEvent->getEpollEvent() );
  
  if (rt == -1){
    ERRORLOG("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno));
  }else{ 
    DEBUGLOG("add event success, fd[%d]", fd);
  }
}

EventLoop::EventLoop() 
    : m_thread_id(std::this_thread::get_id() ), 
      m_epoll_fd(epoll_create(1) ),
      m_wakeup_fd ( eventfd(0, 0) ),
      m_timer_fd (timerfd_create(CLOCK_MONOTONIC,0) ) 
{
  if (m_epoll_fd < 0 ) {
    ERRORLOG("failed to create event loop, epoll_create error, error info[%d]", errno);
    exit(1);
  }
  if (m_wakeup_fd < 0 ) {
    ERRORLOG("failed to create event loop, m_wakeup_fd create error, error info[%d]", errno);
    exit(1);
  }
  if(m_timer_fd < 0){
    ERRORLOG("failed to create event loop, m_timer_fd create error, error info[%d]", errno);
    exit(1);
  }

  INFOLOG("wakeup fd = %d", m_wakeup_fd);

  m_wakeup_fd_event = std::make_unique<WakeUpFdEvent>(m_wakeup_fd);
  add(m_wakeup_fd_event.get() );

  m_timer = std::make_unique<Timer>(m_timer_fd);
  add(m_timer.get() );

  std::stringstream ss;
  ss << m_thread_id;
  INFOLOG("succ create event loop in thread %s", ss.str().c_str());
}

EventLoop::~EventLoop() {
  close(m_epoll_fd);
  close(m_wakeup_fd);
  close(m_timer_fd);
}


void EventLoop::addTimerEvent(TimerEvent::s_ptr event) {
  m_timer->addTimerEvent(event);
}

void EventLoop::loop() {
  m_is_looping = true;
  while (!m_stop_flag) {
    
    // 1. 处理异步任务队列
    std::queue<std::function<void()>> tmp_tasks;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_pending_tasks.swap(tmp_tasks);
    } // 锁的作用域被精确控制，尽早释放

    while (!tmp_tasks.empty()) {
      auto cb = std::move(tmp_tasks.front()); // 减少 std::function 拷贝开销
      tmp_tasks.pop();
      if (cb) {
        cb();
      }
    }

    // 2. epoll 等待 IO 事件
    epoll_event result_events[g_epoll_max_events];
    int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, g_epoll_max_timeout);

    if (rt < 0) {
      if (errno == EINTR) { continue; } // 被信号打断，正常继续
      ERRORLOG("epoll_wait error, errno=%d, error=%s", errno, strerror(errno));
    } else {
      for (int i = 0; i < rt; ++i) {
        epoll_event trigger_event = result_events[i];
        FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
        if (fd_event == nullptr) {
          continue;
        }

        // 直接提取对应底层宏的回调
        if (trigger_event.events & EPOLLIN) {
          addTask(fd_event->handler(EPOLLIN));
        }
        if (trigger_event.events & EPOLLOUT) {
          addTask(fd_event->handler(EPOLLOUT));
        }

        // 包含 EPOLLHUP 与 EPOLLERR 异常情况的安全清理
        if (trigger_event.events & (EPOLLERR | EPOLLHUP)) {
          DEBUGLOG("fd %d trigger EPOLLERROR/EPOLLHUP event", fd_event->getFd());
          deleteEpollEvent(fd_event);
          
          if (fd_event->handler(EPOLLERR) != nullptr) {
            addTask(fd_event->handler(EPOLLERR));
          }
        }
      }
    }
  }
  m_is_looping = false;
}

inline void EventLoop::wakeup() {
  INFOLOG("WAKE UP");
  m_wakeup_fd_event->wakeup();
}

void EventLoop::stop() {
  m_stop_flag = true;
  wakeup();
}


void EventLoop::deleteEpollEventTask(FdEvent* event) {
  auto it = m_listen_fds.find(event->getFd());
  if (it == m_listen_fds.end()) {
    return;
  }

  int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event->getFd(), nullptr);
  if (rt == -1) {
    ERRORLOG("failed epoll_ctl when delete fd, errno=%d, error=%s", errno, strerror(errno));
  } else {
    m_listen_fds.erase(event->getFd());
    DEBUGLOG("delete event success, fd[%d]", event->getFd());
  }
}

void EventLoop::addEpollEvent(FdEvent* fdEvent) {
  if (std::this_thread::get_id() == m_thread_id)
    addEpollEventTask(fdEvent);
  else  {
    auto cb = [this, fdEvent]() { addEpollEventTask(fdEvent); };
    addTask(cb, true);
  }
}

void EventLoop::deleteEpollEvent(FdEvent* event) {
  if (std::this_thread::get_id() == m_thread_id) {
    deleteEpollEventTask(event);
  } else {
    auto cb = [this, event]() { deleteEpollEventTask(event); };
    addTask(cb, true);
  }
}

void EventLoop::addTask(std::function<void()> cb, bool is_wake_up /*=false*/) {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    // 使用 std::move 避免 function 对象的深拷贝
    m_pending_tasks.push(std::move(cb)); 
  }
  if (is_wake_up) {
    wakeup();
  }
}

bool EventLoop::isLooping() const {
  return m_is_looping;
}

}