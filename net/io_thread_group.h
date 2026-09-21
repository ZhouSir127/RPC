#ifndef ROCKET_NET_IO_THREAD_GROUP_H
#define ROCKET_NET_IO_THREAD_GROUP_H

#include <vector>
#include "../common/log.h"
#include "io_thread.h"

namespace rocket {

class IOThreadGroup {

public:
  IOThreadGroup(int size);

  ~IOThreadGroup();

  void start() const;

  void join() const;

  IOThread* getIOThread();

private:
  int m_size;
  std::vector<IOThread*> m_io_thread_groups;
  int m_index;
};

}


#endif