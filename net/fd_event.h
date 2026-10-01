
#ifndef ROCKET_NET_FDEVENT_H
#define ROCKET_NET_FDEVENT_H

#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

namespace rocket {
  class EventLoop;

class FdEvent {

public:

  using TriggerEvent = uint32_t;

  FdEvent(int fd,EventLoop*event_loop);
  virtual ~FdEvent();
  const std::function<void()>& getCallBack(TriggerEvent event_type) const;

  int getFd() const { return m_fd; }
  epoll_event* getEpollEvent() { return &m_listen_events; }

protected:
  EventLoop*m_event_loop;
  int m_fd{-1};
  epoll_event m_listen_events;
  
  void setCallback(TriggerEvent event_type, std::function<void()> callback);
  void cancel(TriggerEvent event_type);

  std::function<void()> m_read_callback{nullptr};
  std::function<void()> m_write_callback{nullptr};
  std::function<void()> m_error_callback{nullptr};
};

}

#endif