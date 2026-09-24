#include <fcntl.h>
#include "fd_event.h"
#include "../common/log.h"

namespace rocket {

FdEvent::FdEvent(int fd) {
  setFd(fd);
}

const std::function<void()>& FdEvent::handler(TriggerEvent event) const {
  switch (event) { 
    case EPOLLIN:
      return m_read_callback;
    case EPOLLOUT:
      return m_write_callback;
    default:
      return m_error_callback;
  }
}

void FdEvent::setFd(int fd){
    m_fd = fd;
    fcntl(fd, F_SETFL,fcntl(fd, F_GETFL, 0) | O_NONBLOCK );
    fcntl(fd, F_SETFL,fcntl(fd, F_GETFL, 0) | FD_CLOEXEC );
    memset(&m_listen_events, 0, sizeof(m_listen_events) );
    m_listen_events.data.ptr = this;
}

void FdEvent::setCallback(TriggerEvent event_type, std::function<void()> callback) {
    m_listen_events.events |= event_type;
    
    switch(event_type){
      case EPOLLIN:
        m_read_callback = std::move(callback);
        break;
      case EPOLLOUT:
        m_write_callback = std::move(callback);
        break;
      case EPOLLERR:
        m_error_callback = std::move(callback);
        break;
    }
}

inline void FdEvent::cancel(TriggerEvent event_type) {
    m_listen_events.events &= ~event_type;
}

}