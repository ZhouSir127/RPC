#include "fd_event_group.h"
#include "../common/log.h"
#include <algorithm> 

namespace rocket {

FdEventGroup* FdEventGroup::GetFdEventGroup() {
  static FdEventGroup g_fd_event_group(128);
  return &g_fd_event_group;
}

FdEventGroup::FdEventGroup(int size) {
  for (int i = 0; i < size; i++)
    m_fd_group.emplace_back(i);
}

FdEvent& FdEventGroup::getFdEvent(int fd) {
  std::unique_lock<std::mutex>lock(m_mutex);
  if (fd >= m_fd_group.size() ){
    int size = std::max(fd+1,int(m_fd_group.size()<<1) );
    for (int i = m_fd_group.size(); i < size; ++i)
      m_fd_group.emplace_back(i);
  }
  return m_fd_group[fd];
}

}