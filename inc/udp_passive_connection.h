/********************************************************
 * Description : udp passive connection
 * Data        : 2019-07-22 19:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_UDP_PASSIVE_CONNECTION_H
#define BOOST_NET_UDP_PASSIVE_CONNECTION_H


#include <string>
#include <vector>
#include <deque>
#include <boost/asio.hpp>
#include "boost_net.h"

namespace BoostNet { // namespace BoostNet begin

class UdpAcceptor;

class UdpPassiveConnection : public UdpConnectionBase, public std::enable_shared_from_this<UdpPassiveConnection>
{
public:
    typedef boost::asio::ip::udp::endpoint                      endpoint_type;
    typedef std::deque<std::vector<char>>                       udp_recv_buffer_type;

public:
    UdpPassiveConnection(UdpAcceptor & acceptor, UdpServiceBase * udp_service, unsigned short host_port, endpoint_type endpoint);
    virtual ~UdpPassiveConnection() override;

public:
    UdpPassiveConnection(const UdpPassiveConnection &) = delete;
    UdpPassiveConnection(UdpPassiveConnection &&) = delete;
    UdpPassiveConnection & operator = (const UdpPassiveConnection &) = delete;
    UdpPassiveConnection & operator = (UdpPassiveConnection &&) = delete;

public:
    virtual void get_host_address(std::string & ip, unsigned short & port) override;
    virtual void get_peer_address(std::string & ip, unsigned short & port) override;

public:
    virtual bool recv_buffer_has_data() override;
    virtual const void * recv_buffer_data() override;
    virtual std::size_t recv_buffer_size() override;
    virtual bool recv_buffer_drop_len(std::size_t len) override;
    virtual bool send_buffer_fill_len(const void * data, std::size_t len) override;

public:
    virtual void close() override;

public:
    void start();
    void stop();
    void send(const void * data, std::size_t len);
    void recv(const void * data, std::size_t len);

private:
    UdpAcceptor                                   & m_acceptor;
    UdpServiceBase                                * m_udp_service;
    bool                                            m_running;
    unsigned short                                  m_host_port;
    endpoint_type                                   m_endpoint;
    std::string                                     m_peer_ip;
    unsigned short                                  m_peer_port;
    udp_recv_buffer_type                            m_recv_buffer;
};

} // namespace BoostNet end


#endif // BOOST_NET_UDP_PASSIVE_CONNECTION_H
