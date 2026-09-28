#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include "../../common/log.h"
#include "net_addr.h"
#include "tcp_acceptor.h"
#include "../eventloop.h"

namespace rocket {

TcpAcceptor::TcpAcceptor(EventLoop*event_loop,const std::shared_ptr<NetAddr>&local_addr,std::function<void()>callback) : 
FdEvent(socket(local_addr->getSockAddr()->sa_family, SOCK_STREAM, 0),event_loop),
m_local_addr(local_addr),
m_family(m_local_addr->getSockAddr()->sa_family)
{
  // if (!local_addr->checkValid()) {
  //   ERRORLOG("invalid local addr %s", local_addr->toString().c_str());
  //   exit(0);
  // }
  // if (m_listenfd < 0) {
  //   ERRORLOG("invalid listenfd %d", m_listenfd);
  //   exit(0);
  // }  
  int val = 1;
  if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
//    ERRORLOG("setsockopt REUSEADDR error, errno=%d, error=%s", errno, strerror(errno));
  }

  if(bind(m_fd, m_local_addr->getSockAddr(), m_local_addr->getSockLen() ) != 0) {
    // ERRORLOG("bind error, errno=%d, error=%s", errno, strerror(errno));
    // exit(0);
  }

  if(listen(m_fd, 1000) != 0) {
    // ERRORLOG("listen error, errno=%d, error=%s", errno, strerror(errno));
    // exit(0);
  }

  setCallback(EPOLLIN,std::move(callback));
}

std::pair<int, std::shared_ptr<NetAddr> > TcpAcceptor::accept() {
  if (m_family == AF_INET) {
    sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    socklen_t clien_addr_len = sizeof(client_addr);

    int client_fd = ::accept(m_fd, reinterpret_cast<sockaddr*>(&client_addr), &clien_addr_len);
    // if (client_fd < 0) {
    //   ERRORLOG("accept error, errno=%d, error=%s", errno, strerror(errno));
    // }
    //INFOLOG("A client have accpeted succ, peer addr [%s]", peer_addr->toString().c_str());
    return {client_fd, std::make_shared<IPNetAddr>(client_addr)};
  } else {
    // ...
    return std::make_pair(-1, nullptr);
  }
}

}