/********************************************************
 * Description : tcp connection
 * Data        : 2018-01-07 18:18:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#ifndef BOOST_NET_TCP_CONNECTION_H
#define BOOST_NET_TCP_CONNECTION_H


#include <string>
#include <vector>
#include <boost/asio.hpp>
#include "boost_net.h"
#include "tcp_recv_buffer.h"
#include "tcp_send_buffer.h"

namespace BoostNet { // namespace BoostNet begin

class TcpConnection : public TcpConnectionBase, public std::enable_shared_from_this<TcpConnection>
{
public:
    typedef boost::asio::ip::tcp::socket            socket_type;
    typedef boost::asio::io_service                 io_service_type;
    typedef TcpRecvBuffer                           tcp_recv_buffer_type;
    typedef TcpSendBuffer                           tcp_send_buffer_type;

public:
    TcpConnection(io_service_type & io_service, TcpServiceBase * tcp_service, bool passtive, std::size_t identity);
    virtual ~TcpConnection() override;

public:
    TcpConnection(const TcpConnection &) = delete;
    TcpConnection & operator = (const TcpConnection &) = delete;

public:
    virtual void get_host_address(std::string & ip, unsigned short & port) override;
    virtual void get_peer_address(std::string & ip, unsigned short & port) override;

public:
    virtual const void * recv_buffer_data() override;
    virtual std::size_t recv_buffer_size() override;
    virtual bool recv_buffer_copy_len(void * buf, std::size_t len) override;
    virtual bool recv_buffer_move_len(void * buf, std::size_t len) override;
    virtual bool recv_buffer_drop_len(std::size_t len) override;
    virtual void recv_buffer_water_mark(std::size_t len) override;
    virtual bool send_buffer_fill_len(const void * data, std::size_t len) override;

public:
    virtual void close() override;

public:
    socket_type & socket();
    io_service_type & io_service();
    tcp_recv_buffer_type & recv_buffer();
    tcp_send_buffer_type & send_buffer();
    void start();
    bool send(const void * data, std::size_t len);

private:
    void send();
    void recv();
    void stop();
    void post_send_data(const void * data, std::size_t len);
    void push_send_data(std::vector<char> data);
    void handle_send(const boost::system::error_code & error);
    void handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred);

private:
    TcpServiceBase                                * m_tcp_service;
    bool                                            m_running;
    bool                                            m_passtive;
    std::size_t                                     m_identity;
    socket_type                                     m_socket;
    std::string                                     m_host_ip;
    unsigned short                                  m_host_port;
    std::string                                     m_peer_ip;
    unsigned short                                  m_peer_port;
    tcp_recv_buffer_type                            m_recv_buffer;
    tcp_send_buffer_type                            m_send_buffer;
    std::size_t                                     m_recv_water_mark;
};

} // namespace BoostNet end


#endif // BOOST_NET_TCP_CONNECTION_H
