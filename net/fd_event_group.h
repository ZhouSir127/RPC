#ifndef ROCKET_NET_FD_EVENT_GROUP_H
#define ROCKET_NET_FD_EVENT_GROUP_H

#include <mutex>
#include <deque>
#include "../net/fd_event.h"

namespace rocket {

class FdEventGroup {

 public:
  

  FdEvent& getFdEvent(int fd);

  static FdEventGroup* GetFdEventGroup();

 private:
  FdEventGroup(int size);
  FdEventGroup(const FdEventGroup&)=delete;
  FdEventGroup(FdEventGroup&&)=delete;
  FdEventGroup& operator=(const FdEventGroup&)=delete;
  FdEventGroup&& operator=(FdEventGroup&&)=delete;

  std::deque<FdEvent> m_fd_group;
  std::mutex m_mutex;
};

}

#endif