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
      m_epoll_fd(epoll_create(1) ),
      m_wakeup_fd_event(this),
      m_timer(this)
{
  // if (m_epoll_fd < 0 ) {
  //   ERRORLOG("failed to create event loop, epoll_create error, error info[%d]", errno);
  //   exit(1);
  // }
  // std::stringstream ss;
  // ss << m_thread_id;
  // INFOLOG("succ create event loop in thread %s", ss.str().c_str());
}

EventLoop::~EventLoop() {
  close(m_epoll_fd);
}

void EventLoop::addTimerEvent(TimerEvent event) {
  m_timer.addTimerEvent(std::move(event) );
  m_timer.resetTimer();
}

void EventLoop::loop() {
  while (m_stop_flag == false) {
    // 2. epoll 等待 IO 事件
    epoll_event result_events[10];
    int rt = epoll_wait(m_epoll_fd, result_events, 10 , 10000);

    // if (rt < 0) {
    //   if (errno == EINTR) { continue; } // 被信号打断，正常继续
    //   ERRORLOG("epoll_wait error, errno=%d, error=%s", errno, strerror(errno));
    // } else 
    //{
      for (int i = 0;i < rt;++i) {
        FdEvent* fd_event = static_cast<FdEvent*>(result_events[i].data.ptr);
        // if (fd_event == nullptr)
        //   continue;
        // 直接提取对应底层宏的回调
        if (result_events[i].events & EPOLLIN)
          addTask(fd_event->getCallBack(EPOLLIN));
        
        if (result_events[i].events & EPOLLOUT)
          addTask(fd_event->getCallBack(EPOLLOUT));
        
        // 包含 EPOLLHUP 与 EPOLLERR 异常情况的安全清理
        if (result_events[i].events & (EPOLLERR | EPOLLHUP))
          //DEBUGLOG("fd %d trigger EPOLLERROR/EPOLLHUP event", fd_event->getFd());
      //    Delete(fd_event);
          addTask(fd_event->getCallBack(EPOLLERR));
      }
          // 1. 处理异步任务队列
      std::queue<std::function<void()>> tmp_tasks;
      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pending_tasks.swap(tmp_tasks);
      } // 锁的作用域被精确控制，尽早释放

      while (tmp_tasks.empty() == false) {
        tmp_tasks.front()();
        tmp_tasks.pop();
      }
  }
}

void EventLoop::stop() {
  m_stop_flag = true;
  m_wakeup_fd_event.wakeup();
}

void EventLoop::addEpollEvent(FdEvent* event) {
  if (std::this_thread::get_id() == m_thread_id)
    add(event);
  else{
    addTask([this,event]() { add(event); });
    m_wakeup_fd_event.wakeup();
  }
}

void EventLoop::deleteEpollEvent(FdEvent* event) {
  if (std::this_thread::get_id() == m_thread_id)
    Delete(event);
  else{
    addTask([this, event]() { Delete(event); });
    m_wakeup_fd_event.wakeup();
  }
}

void EventLoop::addTask(std::function<void()> cb) {
    std::unique_lock<std::mutex> lock(m_mutex);
    // 使用 std::move 避免 function 对象的深拷贝
    m_pending_tasks.push(std::move(cb) ); 
}

}