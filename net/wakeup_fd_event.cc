#include <unistd.h>
#include "wakeup_fd_event.h"
#include "../common/log.h"
#include <sys/eventfd.h>

namespace rocket {

WakeUpFdEvent::WakeUpFdEvent() {
    int m_wakeup_fd( eventfd(0, 0) );
  // if (m_wakeup_fd < 0 ) {
  //   ERRORLOG("failed to create event loop, m_wakeup_fd create error, error info[%d]", errno);
  //   exit(1);
  // }
  setFd(m_wakeup_fd);
  setCallback(EPOLLIN, [this]() {
    uint64_t dummy;
    if (read(m_fd, &dummy, sizeof(dummy)) == -1 && errno != EAGAIN)
      DEBUGLOG("read full bytes from wakeup fd[%d]", m_fd);
  });
}

WakeUpFdEvent::~WakeUpFdEvent(){
  close(m_fd);
}

void WakeUpFdEvent::wakeup() {
  uint64_t buf = 1;
  int rt = write(m_fd, &buf, 8 );
  // if (rt != 8)
  //   ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]", m_fd);

  // DEBUGLOG("success read 8 bytes");
}



}