#include <fcntl.h>
#include "fd_event.h"
#include "../common/log.h"
#include "eventloop.h"

namespace rocket {

FdEvent::FdEvent(int fd,EventLoop*event_loop):m_fd(fd),m_event_loop(event_loop){
    fcntl(m_fd, F_SETFL,fcntl(m_fd, F_GETFL, 0) | O_NONBLOCK );
    fcntl(m_fd, F_SETFD, fcntl(m_fd, F_GETFD) | FD_CLOEXEC);
    memset(&m_listen_events, 0, sizeof(m_listen_events) );
    m_listen_events.data.ptr = this;
    m_event_loop->addEpollEvent(this);
}

FdEvent::~FdEvent(){
  close(m_fd);
  m_event_loop->deleteEpollEvent(this);
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

void FdEvent::setCallback(TriggerEvent event_type, std::function<void()> callback) {
    m_listen_events.events |= event_type;
    
    switch(event_type){
      case EPOLLIN:
        m_read_callback = std::move(callback);
        break;
      case EPOLLOUT:
        m_write_callback = std::move(callback);
        break;
      default:
        m_error_callback = std::move(callback);
    }
}

inline void FdEvent::cancel(TriggerEvent event_type) {
    m_listen_events.events &= ~event_type;
}

}