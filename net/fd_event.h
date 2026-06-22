
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

  std::function<void()> handler(TriggerEvent event_type);

  void setCallback(TriggerEvent event_type, std::function<void()>&& callback);
  void cancel(TriggerEvent event_type);

  int getFd() const { return m_fd; }
  epoll_event* getEpollEvent() { return &m_listen_events; }

protected:
  const int m_fd;

  epoll_event m_listen_events;

  std::function<void()> m_read_callback ;
  std::function<void()> m_write_callback ;
  std::function<void()> m_error_callback ;
};

}

#endif