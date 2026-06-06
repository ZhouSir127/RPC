#include "fd_event_group.h"
#include "../common/log.h"

namespace rocket {

static FdEventGroup* g_fd_event_group = NULL;

FdEventGroup* FdEventGroup::GetFdEventGroup() {
  if (g_fd_event_group != NULL) {
    return g_fd_event_group;
  }

  g_fd_event_group = new FdEventGroup(128);
  return g_fd_event_group;
}

FdEventGroup::FdEventGroup(int size) :m_size(size) {
  for (int i = 0; i < m_size; i++)
    m_fd_group.emplace_back(i);
}

FdEvent& FdEventGroup::getFdEvent(int fd) {
  std::unique_lock<std::mutex>lock(m_mutex);
  if (fd >= m_fd_group.size() ){
    m_size<<=1;
    for (int i = m_fd_group.size(); i < m_size; ++i)
      m_fd_group.emplace_back(i);
  }
  return m_fd_group[fd];
}

}