#ifndef ROCKET_NET_IO_THREAD_GROUP_H
#define ROCKET_NET_IO_THREAD_GROUP_H

#include <vector>
#include <memory>
#include "../common/log.h"
#include "io_thread.h"

namespace rocket {

class IOThreadGroup {

public:
  IOThreadGroup(int size);

  void start() const;

  void join() const;

  IOThread* getIOThread();

private:
  std::vector<std::unique_ptr<IOThread>> m_io_thread_groups;
  int m_index{0};
};

}


#endif