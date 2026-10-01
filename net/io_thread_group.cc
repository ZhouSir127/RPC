#include "io_thread_group.h"
#include "../common/log.h"


namespace rocket {

IOThreadGroup::IOThreadGroup(int size):m_io_thread_groups(std::make_unique<IOThread[]>(size) ),m_size(size){}

void IOThreadGroup::start() const{
  for (int i = 0 ;i<m_size; ++i)
    m_io_thread_groups[i].start();
}

void IOThreadGroup::stop() const{
    for (int i = 0 ;i<m_size; ++i)
      m_io_thread_groups[i].stop();
} 

IOThread* IOThreadGroup::getIOThread() {
  if (m_index == m_size )
    m_index = 0;
  
  return &m_io_thread_groups[m_index++];
}

}