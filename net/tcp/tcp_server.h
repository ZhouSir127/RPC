#ifndef ROCKET_NET_TCP_SERVER_H
#define ROCKET_NET_TCP_SERVER_H

#include <unordered_map>
#include "tcp_acceptor.h"
#include "tcp_connection.h"
#include "net_addr.h"
#include "../eventloop.h"
#include "../io_thread_group.h"

namespace rocket {

class TcpServer {
public:
  TcpServer(const std::shared_ptr<NetAddr>& local_addr);
  void onAccept();
  void start();
private:
  // 当有新客户端连接之后需要执行
  // 清除 closed 的连接
  void ClearClientTimerFunc();
  std::shared_ptr<NetAddr> m_local_addr;    // 本地监听地址
 
  EventLoop* m_main_event_loop {NULL};    // mainReactor
  
  IOThreadGroup m_io_thread_group;   // subReactor 组

  TcpAcceptor m_listen_fd_event;

  std::unordered_map<int,TcpConnection> m_client;
  //TimerEvent m_clear_client_timer_event;
};

}


#endif