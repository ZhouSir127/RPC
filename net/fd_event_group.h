#ifndef ROCKET_NET_FD_EVENT_GROUP_H
#define ROCKET_NET_FD_EVENT_GROUP_H

#include <mutex>
#include <deque>
#include "../net/fd_event.h"

namespace rocket {

class FdEventGroup {

 public:
  FdEventGroup(int size);

  FdEvent& getFdEvent(int fd);

  static FdEventGroup* GetFdEventGroup();

 private:
  int m_size;
  std::deque<FdEvent> m_fd_group;
  std::mutex m_mutex;
};

}

#endif