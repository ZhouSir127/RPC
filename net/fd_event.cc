#include <fcntl.h>
#include "fd_event.h"
#include "../common/log.h"

namespace rocket {

FdEvent::FdEvent(int fd) : m_fd(fd),m_read_callback(nullptr), m_write_callback(nullptr), m_error_callback(nullptr) {
  fcntl(fd, F_SETFL,fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
  initEvent();
}

FdEvent::FdEvent():m_fd(-1), m_read_callback(nullptr), m_write_callback(nullptr), m_error_callback(nullptr) {
  initEvent();
}

inline std::function<void()> FdEvent::handler(TriggerEvent event) {
  switch (event) { 
    case (EPOLLIN):
      return m_read_callback;
    case (EPOLLOUT):
      return m_write_callback;
    case (EPOLLERR):
      return m_error_callback;
    default:
      return nullptr;
  }
}

inline void FdEvent::setCallback(TriggerEvent event_type, std::function<void()>&& callback) {
    m_listen_events.events |= event_type;
    
    switch(event_type){
      case (EPOLLIN):
        m_read_callback = std::move(callback);
      case (EPOLLOUT):
        m_write_callback = std::move(callback);
      case (EPOLLERR):
        m_error_callback = std::move(callback);
    }
}

inline void FdEvent::cancel(TriggerEvent event_type) {
    m_listen_events.events &= ~event_type;
}

}