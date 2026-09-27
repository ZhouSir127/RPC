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
m_listen_fd_event(local_addr,[this](){onAccept();} )
{ 
  m_main_event_loop->addEpollEvent(&m_listen_fd_event);
	m_main_event_loop->addTimerEvent( TimerEvent(5000, true, [this](){ClearClientTimerFunc();}) ); 
  //INFOLOG("rocket TcpServer listen sucess on [%s]", m_local_addr->toString().c_str());
}

void TcpServer::onAccept() {
  auto [client_fd,peer_addr] = m_listen_fd_event.accept();

  // 把 cleintfd 添加到任意 IO 线程里面
  m_client.emplace(m_io_thread_group.getIOThread()->getEventLoop(),client_fd, peer_addr, m_local_addr).first->second.setState(Connected);
  //INFOLOG("TcpServer succ get client, fd=%d", client_fd);
}

void TcpServer::start() {
  m_io_thread_group.start();
  m_main_event_loop->loop();
}


void TcpServer::ClearClientTimerFunc() {
  auto it = m_client.begin();
  for (it = m_client.begin(); it != m_client.end(); ) {
    // TcpConnection::ptr s_conn = i.second;
		// DebugLog << "state = " << s_conn->getState();
    if ((*it) != nullptr && (*it).use_count() > 0 && (*it)->getState() == Closed) {
      // need to delete TcpConnection
      DEBUGLOG("TcpConection [fd:%d] will delete, state=%d", (*it)->getFd(), (*it)->getState());
      it = m_client.erase(it);
    } else {
      it++;
    }
	
  }

}

}