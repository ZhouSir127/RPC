
#ifndef ROCKET_NET_FDEVENT_H
#define ROCKET_NET_FDEVENT_H

#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

namespace rocket {
class FdEvent {
public:
  using TriggerEvent = uint32_t;

  FdEvent(int fd);
  FdEvent()=default;

  const std::function<void()>& handler(TriggerEvent event_type) const;

  void cancel(TriggerEvent event_type);

  int getFd() const { return m_fd; }
  epoll_event* getEpollEvent() { return &m_listen_events; }

protected:
  int m_fd{-1};

  epoll_event m_listen_events;
  
  void setFd(int fd);
  void setCallback(TriggerEvent event_type, std::function<void()> callback);

  std::function<void()> m_read_callback{nullptr};
  std::function<void()> m_write_callback{nullptr};
  std::function<void()> m_error_callback{nullptr};
};

}

#endif