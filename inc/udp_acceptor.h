/********************************************************
 * Description : udp acceptor
 * Data        : 2019-07-22 20:00:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_UDP_ACCEPTOR_H
#define BOOST_NET_UDP_ACCEPTOR_H


#include <string>
#include <vector>
#include <deque>
#include <map>
#include <boost/asio.hpp>
#include "boost_net.h"

namespace BoostNet { // namespace BoostNet begin

class UdpPassiveConnection;

class UdpAcceptor : public std::enable_shared_from_this<UdpAcceptor>
{
public:
    typedef boost::asio::ip::udp::endpoint                      endpoint_type;
    typedef boost::asio::ip::udp::socket                        socket_type;
    typedef boost::asio::io_context                             io_context_type;
    typedef std::pair<endpoint_type, std::vector<char>>         endpoint_buffer_type;
    typedef std::deque<endpoint_buffer_type>                    udp_send_buffer_type;
    typedef UdpPassiveConnection                                connection_type;
    typedef std::shared_ptr<connection_type>                    udp_connection_ptr;
    typedef std::map<endpoint_type, udp_connection_ptr>         udp_connection_map;

public:
    UdpAcceptor(io_context_type & io_context, UdpServiceBase * udp_service, const char * host, unsigned short port);
    ~UdpAcceptor();

public:
    UdpAcceptor(const UdpAcceptor &) = delete;
    UdpAcceptor(UdpAcceptor &&) = delete;
    UdpAcceptor & operator = (const UdpAcceptor &) = delete;
    UdpAcceptor & operator = (UdpAcceptor &&) = delete;

public:
    void get_host_address(std::string & ip, unsigned short & port);
    bool start();
    void stop();
    void send(const endpoint_type & endpoint, const void * data, std::size_t len);
    void close(const endpoint_type & endpoint);

private:
    void send();
    void recv();

private:
    void push_send_data(endpoint_buffer_type endpoint_data);

private:
    void handle_send(const boost::system::error_code & error, std::size_t bytes_transferred);
    void handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred);
    void handle_close(endpoint_type endpoint);
    void handle_stop();

private:
    enum { max_recv_payload = 1500 };

private:
    io_context_type                               & m_io_context;
    UdpServiceBase                                * m_udp_service;
    bool                                            m_running;
    endpoint_type                                   m_host_endpoint;
    endpoint_type                                   m_peer_endpoint;
    socket_type                                     m_socket;
    std::string                                     m_host_ip;
    unsigned short                                  m_host_port;
    udp_connection_map                              m_connection_map;
    udp_send_buffer_type                            m_send_buffer;
    char                                            m_recv_data[max_recv_payload];
    bool                                            m_good;
};

} // namespace BoostNet end


#endif // BOOST_NET_UDP_ACCEPTOR_H
