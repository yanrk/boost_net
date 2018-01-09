/********************************************************
 * Description : tcp manager implement
 * Data        : 2018-01-02 11:38:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#ifndef BOOST_NET_TCP_MANAGER_IMPLEMENT_H
#define BOOST_NET_TCP_MANAGER_IMPLEMENT_H


#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "boost_net.h"
#include "tcp_connection.h"
#include "io_service_pool.h"

namespace BoostNet { // namespace BoostNet begin

class TcpManagerImpl
{
public:
    typedef boost::asio::io_service                 io_service_type;
    typedef boost::asio::ip::tcp::endpoint          endpoint_type;
    typedef boost::asio::ip::tcp::acceptor          acceptor_type;
    typedef boost::ptr_vector<acceptor_type>        acceptors_type;
    typedef IOServicePool                           io_service_pool_type;
    typedef std::shared_ptr<TcpConnection>          tcp_connection_ptr;

public:
    TcpManagerImpl();
    ~TcpManagerImpl();

public:
    TcpManagerImpl(const TcpManagerImpl &) = delete;
    TcpManagerImpl & operator = (const TcpManagerImpl &) = delete;

public:
    bool init(TcpServiceBase * tcp_service, std::size_t thread_count, unsigned short port_array[], std::size_t port_count);
    void exit();

public:
    void run(bool blocking = false);

public:
    bool create_connection(const std::string & host, const std::string & service, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);
    bool create_connection(const std::string & host, unsigned short port, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);

private:
    void start_accept(acceptor_type & acceptor, unsigned short port);
    void handle_accept(acceptor_type & acceptor, unsigned short port, tcp_connection_ptr tcp_connection, const boost::system::error_code & error);

private:
    io_service_pool_type                            m_io_service_pool;
    acceptors_type                                  m_acceptors;
    TcpServiceBase                                * m_tcp_service;
};

} // namespace BoostNet end


#endif // BOOST_NET_TCP_MANAGER_IMPLEMENT_H
