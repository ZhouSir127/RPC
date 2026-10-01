
#include <thread>
#include <assert.h>
#include "io_thread.h"
#include "../common/log.h"
#include "../common/util.h"


namespace rocket {

IOThread::IOThread(): m_thread(&IOThread::Main, this) {  
  // wait, 直到新线程执行完 Main 函数的前置
  m_init_semaphore.acquire();
  //DEBUGLOG("IOThread [%d] create success", m_thread_id);
}

IOThread::~IOThread() {
  stop();
}

void IOThread::Main() {
  m_event_loop = EventLoop::GetCurrentEventLoop();
  // 唤醒等待的线程
  m_init_semaphore.release();
  // 让IO 线程等待，直到我们主动的启动
  //DEBUGLOG("IOThread %d created, wait start semaphore", thread->m_thread_id);
  m_start_semaphore.acquire();
  //DEBUGLOG("IOThread %d start loop ", thread->m_thread_id);
  m_event_loop->loop();
  //DEBUGLOG("IOThread %d end loop ", thread->m_thread_id);
}

void IOThread::start() {
  //DEBUGLOG("Now invoke IOThread %d", m_thread_id);
  std::call_once(m_start_once, [this] {
    m_start_semaphore.release();
  });
}

void IOThread::stop() {
  m_event_loop->stop();
  start();
  if (m_thread.joinable())
    m_thread.join();
}

}