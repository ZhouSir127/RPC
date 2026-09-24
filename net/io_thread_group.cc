#include "io_thread_group.h"
#include "../common/log.h"


namespace rocket {


IOThreadGroup::IOThreadGroup(int size) {
  m_io_thread_groups.resize(size);
  for (int i = 0; i < size; ++i)
    m_io_thread_groups[i] = std::make_unique<IOThread>();
}

void IOThreadGroup::start() const{
  for (const std::unique_ptr<IOThread>&p:m_io_thread_groups)
    p->start();
}

void IOThreadGroup::join() const{
  for (const std::unique_ptr<IOThread>&p:m_io_thread_groups)
    p->join();
} 

IOThread* IOThreadGroup::getIOThread() {
  if (m_index == m_io_thread_groups.size() )
    m_index = 0;
  
  return m_io_thread_groups[m_index++].get();
}

}