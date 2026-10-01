// tcp_connection.h
#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "../fd_event.h"
#include "../eventloop.h"
#include "../coder/abstract_coder.h"
#include "net_addr.h"
#include "tcp_buffer.h"

namespace rocket {

enum TcpState {
    NotConnected = 0,
    Connected,
    HalfClosing,
    Closed
};

enum TcpConnectionType {
    TcpConnectionByServer = 0,
    TcpConnectionByClient
};

class TcpConnection : public FdEvent {
public:
    //using s_ptr = std::shared_ptr<TcpConnection>;

    TcpConnection(int fd, EventLoop* event_loop,int buffer_size,
                  const std::shared_ptr<NetAddr>& peer_addr,
                  const std::shared_ptr<NetAddr>& local_addr,
                  TcpConnectionType type= TcpConnectionByServer);
    ~TcpConnection();

    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;
    TcpConnection(TcpConnection&&) = delete;
    TcpConnection& operator=(TcpConnection&&) = delete;

    void onRead();
    void excute();
    void onWrite();
    void reply(std::vector<AbstractProtocol::s_ptr>& messages);

    void listenRead();
    void listenWrite();
    void clear();
    void shutdown();

    void setState(TcpState state);
    TcpState getState() const;

    void setConnectionType(TcpConnectionType type);
    void pushSendMessage(AbstractProtocol::s_ptr message,
                         std::function<void(AbstractProtocol::s_ptr)> done);
    void pushReadMessage(const std::string& msg_id,
                         std::function<void(AbstractProtocol::s_ptr)> done);

    std::shared_ptr<NetAddr> getLocalAddr() const;
    std::shared_ptr<NetAddr> getPeerAddr() const;
    // getFd() 直接使用继承自 FdEvent 的版本。

private:
    std::shared_ptr<NetAddr> m_peer_addr;
    std::shared_ptr<NetAddr> m_local_addr;

    TcpBuffer m_in_buffer;
    TcpBuffer m_out_buffer;
    std::unique_ptr<AbstractCoder> m_coder;

    std::atomic<TcpState> m_state{NotConnected};
    TcpConnectionType m_connection_type;

    std::vector<std::pair<AbstractProtocol::s_ptr,
                          std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones;
    std::map<std::string,std::function<void(AbstractProtocol::s_ptr)>> m_read_dones;
};

}  // namespace rocket
#endif