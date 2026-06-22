#include <unistd.h>
#include "wakeup_fd_event.h"
#include "../common/log.h"

namespace rocket {

WakeUpFdEvent::WakeUpFdEvent(int fd) : FdEvent(fd) {
  setCallback(EPOLLIN, [fd]() {
    uint64_t dummy;
    if (read(fd, &dummy, sizeof(dummy)) == -1 && errno != EAGAIN)
      DEBUGLOG("read full bytes from wakeup fd[%d]", fd);
  });
}

void WakeUpFdEvent::wakeup() {
  uint64_t buf = 1;
  int rt = write(m_fd, &buf, 8 );
  if (rt != 8)
    ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]", m_fd);

  DEBUGLOG("success read 8 bytes");
}

}