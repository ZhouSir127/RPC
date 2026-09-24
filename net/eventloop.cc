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


void EventLoop::add(FdEvent* fdEvent) {
  int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD , fdEvent -> getFd() , fdEvent->getEpollEvent() );  
  // if (rt == -1){
  //   ERRORLOG("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno));
  // }else{ 
  //   DEBUGLOG("add event success, fd[%d]", fd);
  // }
}


void EventLoop::modify(FdEvent* fdEvent){
  
  int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD , fdEvent -> getFd() , fdEvent->getEpollEvent() );
  // if (rt == -1){
  //   ERRORLOG("failed epoll_ctl when add fd, errno=%d, error=%s", errno, strerror(errno));
  // }else{ 
  //   DEBUGLOG("add event success, fd[%d]", fd);
  // }
}

void EventLoop::Delete(FdEvent* event) {

  int rt = epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, event->getFd(), nullptr);
  // if (rt == -1) {
  //   ERRORLOG("failed epoll_ctl when delete fd, errno=%d, error=%s", errno, strerror(errno));
  // } else {
  //   DEBUGLOG("delete event success, fd[%d]", event->getFd());
  // }
}


EventLoop::EventLoop() 
    : m_thread_id(std::this_thread::get_id() ), 
      m_epoll_fd(epoll_create(1) )
      //m_timer_fd ( ) 
{
  // if (m_epoll_fd < 0 ) {
  //   ERRORLOG("failed to create event loop, epoll_create error, error info[%d]", errno);
  //   exit(1);
  // }



  add(&m_wakeup_fd_event);

  add(&m_timer);
  // std::stringstream ss;
  // ss << m_thread_id;
  // INFOLOG("succ create event loop in thread %s", ss.str().c_str());
}

EventLoop::~EventLoop() {
  close(m_epoll_fd);
}

void EventLoop::addTimerEvent(std::shared_ptr<TimerEvent> event) {
  m_timer.addTimerEvent(event);
}
//???

void EventLoop::loop() {
  while (m_stop_flag == false) {
    // 1. 处理异步任务队列
    std::queue<std::function<void()>> tmp_tasks;
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_pending_tasks.swap(tmp_tasks);
    } // 锁的作用域被精确控制，尽早释放

    while (tmp_tasks.empty() == false) {
      if (tmp_tasks.front() )
        tmp_tasks.front()();
      tmp_tasks.pop();
    }

    constexpr int g_epoll_max_timeout = 10000;
    constexpr int g_epoll_max_events = 10;

    // 2. epoll 等待 IO 事件
    epoll_event result_events[g_epoll_max_events];
    int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, g_epoll_max_timeout);

    // if (rt < 0) {
    //   if (errno == EINTR) { continue; } // 被信号打断，正常继续
    //   ERRORLOG("epoll_wait error, errno=%d, error=%s", errno, strerror(errno));
    // } else 
    //{
      for (epoll_event&trigger_event:result_events) {
        FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
        // if (fd_event == nullptr)
        //   continue;

        // 直接提取对应底层宏的回调
        if (trigger_event.events & EPOLLIN)
          addTask(fd_event->handler(EPOLLIN));
        
        if (trigger_event.events & EPOLLOUT)
          addTask(fd_event->handler(EPOLLOUT));
        
        // 包含 EPOLLHUP 与 EPOLLERR 异常情况的安全清理
        if (trigger_event.events & (EPOLLERR | EPOLLHUP)) {
          //DEBUGLOG("fd %d trigger EPOLLERROR/EPOLLHUP event", fd_event->getFd());
          Delete(fd_event);
          //if (fd_event->handler(EPOLLERR) != nullptr)
          addTask(fd_event->handler(EPOLLERR));
        }
      }
    //}
  }
}

void EventLoop::stop() {
  m_stop_flag = true;
  m_wakeup_fd_event.wakeup();
}

void EventLoop::addEpollEvent(FdEvent* event) {
  if (std::this_thread::get_id() == m_thread_id)
    add(event);
  else
    addTask([this,event]() { add(event); }, true);
}

void EventLoop::deleteEpollEvent(FdEvent* event) {
  if (std::this_thread::get_id() == m_thread_id)
    Delete(event);
  else 
    addTask([this, event]() { Delete(event); }, true);
}

void EventLoop::addTask(const std::function<void()>&cb, bool is_wake_up /*=false*/) {
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    // 使用 std::move 避免 function 对象的深拷贝
    m_pending_tasks.push(cb); 
  }
  if (is_wake_up)
    m_wakeup_fd_event.wakeup();
}

}