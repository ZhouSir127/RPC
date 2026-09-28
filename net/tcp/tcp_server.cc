#include "tcp_server.h"
#include "../eventloop.h"
#include "tcp_connection.h"
#include "../../common/log.h"
#include "../../common/config.h"

namespace rocket {

TcpServer::TcpServer(const std::shared_ptr<NetAddr>&local_addr) 
:m_local_addr(local_addr),
m_main_event_loop(EventLoop::GetCurrentEventLoop() ),
m_io_thread_group (Config::GetGlobalConfig()->m_io_threads), 
m_listen_fd_event(m_main_event_loop,local_addr,[this](){onAccept();})
{ 
  m_main_event_loop->addTimerEvent( TimerEvent(5000, true, [this](){ClearClientTimerFunc();}) ); 
  //INFOLOG("rocket TcpServer listen sucess on [%s]", m_local_addr->toString().c_str());
}

void TcpServer::onAccept() {
  auto [client_fd,peer_addr] = m_listen_fd_event.accept();
  // 把 cleintfd 添加到任意 IO 线程里面
  if (client_fd < 0) 
    return;

  EventLoop* io_loop = m_io_thread_group.getIOThread()->getEventLoop();
  auto [it, inserted] = m_client.try_emplace(client_fd, client_fd , io_loop, 128, peer_addr, m_local_addr);
  if (inserted){
      it->second.setState(Connected);
      io_loop->addEpollEvent(&(it->second) );
  }
}

void TcpServer::start() {
  m_io_thread_group.start();
  m_main_event_loop->loop();
}

void TcpServer::ClearClientTimerFunc() {
    auto it = m_client.begin(); 
    while (it != m_client.end() )
        if(it->second.getState() == Closed)
            it = m_client.erase(it);  // 删除对象，返回下一个迭代器
        else
          ++it;
}

}