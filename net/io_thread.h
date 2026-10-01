#ifndef ROCKET_NET_IO_THREAD_H
#define ROCKET_NET_IO_THREAD_H

#include <thread>
#include <semaphore>
#include "eventloop.h"

namespace rocket {

class IOThread {
public:
  IOThread();
  ~IOThread();

  EventLoop* getEventLoop() const{
    return m_event_loop;
  }

  void start ();
  void stop();

private:
  void Main();
  EventLoop* m_event_loop {NULL}; // 当前 io 线程的 loop 对象
  std::binary_semaphore m_init_semaphore{0};
  std::binary_semaphore m_start_semaphore{0};
  std::once_flag m_start_once;
  std::thread m_thread;   // 线程句柄
};

}

#endif