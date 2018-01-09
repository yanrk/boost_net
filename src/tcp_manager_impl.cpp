/********************************************************
 * Description : tcp manager implement
 * Data        : 2018-01-02 11:38:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#include <cstring>
#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include <boost/lexical_cast.hpp>
#include "tcp_manager_impl.h"

namespace BoostNet { // namespace BoostNet begin

TcpManagerImpl::TcpManagerImpl()
    : m_io_service_pool()
    , m_acceptors()
    , m_tcp_service(nullptr)
{

}

TcpManagerImpl::~TcpManagerImpl()
{

}

bool TcpManagerImpl::init(TcpServiceBase * tcp_service, std::size_t thread_count, unsigned short port_array[], std::size_t port_count)
{
    if (nullptr == tcp_service)
    {
        return(false);
    }

    if (0 == thread_count)
    {
        return(false);
    }

    if (nullptr == port_array && 0 != port_count)
    {
        return(false);
    }

    if (m_io_service_pool.size() > 0)
    {
        return(false);
    }

    if (!m_io_service_pool.init(thread_count))
    {
        return(false);
    }

    m_tcp_service = tcp_service;

    try
    {
        for (std::size_t index = 0; index < port_count; ++index)
        {
            unsigned short port = port_array[index];
            endpoint_type endpoint(boost::asio::ip::tcp::v4(), port);
            bool reuse_address = true;
            m_acceptors.push_back(boost::factory<acceptor_type *>()(m_io_service_pool.get(), endpoint, reuse_address));
            start_accept(m_acceptors.back(), port);
        }
    }
    catch (boost::system::error_code &)
    {
        return(false);
    }
    catch (...)
    {
        return(false);
    }

    return(true);
}

void TcpManagerImpl::exit()
{
    m_io_service_pool.exit();
    m_acceptors.clear();
    m_tcp_service = nullptr;
}

void TcpManagerImpl::run(bool blocking)
{
    m_io_service_pool.run(blocking);
}

void TcpManagerImpl::start_accept(acceptor_type & acceptor, unsigned short port)
{
    bool passtive = true;
    std::size_t identity = port;
    tcp_connection_ptr tcp_connection = boost::factory<tcp_connection_ptr>()(m_io_service_pool.get(), m_tcp_service, passtive, identity);
    acceptor.async_accept(tcp_connection->socket(), boost::bind(&TcpManagerImpl::handle_accept, this, boost::ref(acceptor), port, tcp_connection, boost::asio::placeholders::error));
}

void TcpManagerImpl::handle_accept(acceptor_type & acceptor, unsigned short port, tcp_connection_ptr tcp_connection, const boost::system::error_code & error)
{
    start_accept(acceptor, port);

    if (error)
    {
        tcp_connection->close();
        return;
    }

    tcp_connection->io_service().post(boost::bind(&TcpConnection::start, tcp_connection));
}

bool TcpManagerImpl::create_connection(const std::string & host, const std::string & service, std::size_t identity, const char * bind_ip, unsigned short bind_port)
{
    typedef boost::asio::ip::tcp::resolver  resolver_type;
    typedef resolver_type::query            query_type;
    typedef resolver_type::iterator         iterator_type;
    typedef boost::system::error_code       error_code_type;
    typedef boost::asio::ip::tcp::endpoint  endpoint_type;

    bool passtive = false;
    tcp_connection_ptr tcp_connection = boost::factory<tcp_connection_ptr>()(m_io_service_pool.get(), m_tcp_service, passtive, identity);
    TcpConnection::socket_type & socket = tcp_connection->socket();

    endpoint_type endpoint;
    if (0 != bind_port)
    {
        if (nullptr == bind_ip || 0 == strcmp(bind_ip, "0.0.0.0") || 0 == strcmp(bind_ip, "127.0.0.1"))
        {
            endpoint = endpoint_type(boost::asio::ip::tcp::v4(), bind_port);
        }
        else
        {
            endpoint = endpoint_type(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
        }
    }

    resolver_type resolver(tcp_connection->io_service());
    query_type query(host, service);
    iterator_type iter_end;
    error_code_type error = boost::asio::error::host_not_found;
    for (iterator_type iter = resolver.resolve(query); error && iter_end != iter; ++iter)
    {
        socket.close(error);
        if (0 != bind_port)
        {
            socket.open(endpoint.protocol(), error);
            if (error)
            {
                continue;
            }
            socket.set_option(boost::asio::ip::tcp::socket::reuse_address(true), error);
            if (error)
            {
                continue;
            }
            socket.bind(endpoint, error);
            if (error)
            {
                continue;
            }
        }
        socket.connect(*iter, error);
    }

    if (error)
    {
        return(false);
    }

    tcp_connection->io_service().post(boost::bind(&TcpConnection::start, tcp_connection));

    return(true);
}

bool TcpManagerImpl::create_connection(const std::string & host, unsigned short port, std::size_t identity, const char * bind_ip, unsigned short bind_port)
{
    return(create_connection(host, boost::lexical_cast<std::string>(port), identity, bind_ip, bind_port));
}

} // namespace BoostNet end
