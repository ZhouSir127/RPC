#ifndef ROCKET_NET_TCP_TCP_ACCEPTOR_H
#define ROCKET_NET_TCP_TCP_ACCEPTOR_H

#include <memory>
#include "net_addr.h"
#include "../fd_event.h"

namespace rocket {

class TcpAcceptor:public FdEvent {
public:
  typedef std::shared_ptr<TcpAcceptor> s_ptr;

  TcpAcceptor(const std::shared_ptr<NetAddr>&local_addr,std::function<void()>f);
  ~TcpAcceptor();

  std::pair<int, std::shared_ptr<NetAddr>> accept();

  int getListenFd()const;

private:

  std::shared_ptr<NetAddr> m_local_addr; // 服务端监听的地址，addr -> ip:port 
  int m_family {-1};

};

}

#endif