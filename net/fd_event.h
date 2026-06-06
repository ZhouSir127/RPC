
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
  FdEvent();

  ~FdEvent()=default;

  std::function<void()> handler(TriggerEvent event_type);

  void setCallback(TriggerEvent event_type, std::function<void()>&& callback);
  void cancel(TriggerEvent event_type);

  int getFd() const { return m_fd; }
  const epoll_event& getEpollEvent() const{ return m_listen_events; }

 protected:
  int m_fd;

  epoll_event m_listen_events;

  std::function<void()> m_read_callback ;
  std::function<void()> m_write_callback ;
  std::function<void()> m_error_callback ;

  void initEvent() {
    memset(&m_listen_events, 0, sizeof(m_listen_events) );
    m_listen_events.data.ptr = this;
  }
};

}

#endif