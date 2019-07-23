/********************************************************
 * Description : udp manager implement
 * Data        : 2019-07-23 09:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_UDP_MANAGER_IMPLEMENT_H
#define BOOST_NET_UDP_MANAGER_IMPLEMENT_H


#include <string>
#include <memory>
#include <boost/asio.hpp>
#include "boost_net.h"
#include "udp_acceptor.h"
#include "udp_active_connection.h"
#include "io_context_pool.h"

namespace BoostNet { // namespace BoostNet begin

class UdpManagerImpl
{
public:
    typedef boost::asio::io_context                             io_context_type;
    typedef boost::asio::ip::udp::endpoint                      endpoint_type;
    typedef IOServicePool                                       io_context_pool_type;
    typedef UdpActiveConnection                                 udp_connection_type;
    typedef std::shared_ptr<udp_connection_type>                udp_connection_ptr;
    typedef std::shared_ptr<UdpAcceptor>                        udp_acceptor_ptr;
    typedef std::shared_ptr<boost::asio::ip::udp::resolver>     resolver_ptr;

public:
    UdpManagerImpl();
    ~UdpManagerImpl();

public:
    UdpManagerImpl(const UdpManagerImpl &) = delete;
    UdpManagerImpl(UdpManagerImpl &&) = delete;
    UdpManagerImpl & operator = (const UdpManagerImpl &) = delete;
    UdpManagerImpl & operator = (UdpManagerImpl &&) = delete;

public:
    bool init(UdpServiceBase * udp_service, std::size_t thread_count, unsigned short port_array[], std::size_t port_count);
    void exit();

public:
    void run(bool blocking = false);

public:
    bool create_connection(const std::string & host, const std::string & service, bool sync_connect = true, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);
    bool create_connection(const std::string & host, unsigned short port, bool sync_connect = true, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);

private:
    bool sync_create_connection(const std::string & host, const std::string & service, std::size_t identity, const char * bind_ip, unsigned short bind_port);
    bool async_create_connection(const std::string & host, const std::string & service, std::size_t identity, const char * bind_ip, unsigned short bind_port);

private:
    io_context_pool_type                            m_io_context_pool;
    UdpServiceBase                                * m_udp_service;
};

} // namespace BoostNet end


#endif // BOOST_NET_UDP_MANAGER_IMPLEMENT_H
