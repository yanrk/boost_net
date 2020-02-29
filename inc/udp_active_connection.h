/********************************************************
 * Description : udp active connection
 * Data        : 2019-07-22 14:00:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_UDP_ACTIVE_CONNECTION_H
#define BOOST_NET_UDP_ACTIVE_CONNECTION_H


#include <string>
#include <vector>
#include <deque>
#include <boost/asio.hpp>
#include "boost_net.h"

namespace BoostNet { // namespace BoostNet begin

class UdpActiveConnection : public UdpConnectionBase, public std::enable_shared_from_this<UdpActiveConnection>
{
public:
    typedef boost::asio::ip::udp::endpoint                      endpoint_type;
    typedef boost::asio::ip::udp::socket                        socket_type;
    typedef boost::asio::io_context                             io_context_type;
    typedef std::deque<std::vector<char>>                       udp_recv_buffer_type;
    typedef std::deque<std::vector<char>>                       udp_send_buffer_type;
    typedef std::shared_ptr<boost::asio::ip::udp::resolver>     resolver_ptr;

public:
    UdpActiveConnection(io_context_type & io_context, UdpServiceBase * udp_service, std::size_t identity);
    virtual ~UdpActiveConnection() override;

public:
    UdpActiveConnection(const UdpActiveConnection &) = delete;
    UdpActiveConnection(UdpActiveConnection &&) = delete;
    UdpActiveConnection & operator = (const UdpActiveConnection &) = delete;
    UdpActiveConnection & operator = (UdpActiveConnection &&) = delete;

public:
    virtual void get_host_address(std::string & ip, unsigned short & port) override;
    virtual void get_peer_address(std::string & ip, unsigned short & port) override;

public:
    virtual bool recv_buffer_has_data() override;
    virtual const void * recv_buffer_data() override;
    virtual std::size_t recv_buffer_size() override;
    virtual bool recv_buffer_drop() override;
    virtual bool send_buffer_fill(const void * data, std::size_t len) override;

public:
    virtual void close() override;

public:
    socket_type & socket();
    io_context_type & io_context();

public:
    void start();

public:
    void handle_resolve(const boost::system::error_code & error, boost::asio::ip::udp::resolver::iterator iterator, boost::asio::ip::udp::endpoint host_endpoint, resolver_ptr resolver);
    void handle_connect(const boost::system::error_code & error, boost::asio::ip::udp::resolver::iterator iterator, boost::asio::ip::udp::endpoint host_endpoint, resolver_ptr resolver);

private:
    void send();
    void recv();
    void stop();
    void post_send_data(const void * data, std::size_t len);
    void push_send_data(std::vector<char> data);

private:
    void handle_send(const boost::system::error_code & error);
    void handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred);

private:
    enum { max_recv_payload = 1500 };

private:
    io_context_type                               & m_io_context;
    UdpServiceBase                                * m_udp_service;
    bool                                            m_running;
    std::size_t                                     m_identity;
    socket_type                                     m_socket;
    std::string                                     m_host_ip;
    unsigned short                                  m_host_port;
    std::string                                     m_peer_ip;
    unsigned short                                  m_peer_port;
    udp_recv_buffer_type                            m_recv_buffer;
    udp_send_buffer_type                            m_send_buffer;
    char                                            m_recv_data[max_recv_payload];
};

} // namespace BoostNet end


#endif // BOOST_NET_UDP_ACTIVE_CONNECTION_H
