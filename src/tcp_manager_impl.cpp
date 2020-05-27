/********************************************************
 * Description : tcp manager implement
 * Data        : 2018-01-02 11:38:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include <boost/lexical_cast.hpp>
#include "tcp_manager_impl.h"

namespace BoostNet { // namespace BoostNet begin

TcpManagerImpl::TcpManagerImpl()
    : m_io_context_pool()
    , m_acceptors()
    , m_tcp_service(nullptr)
{

}

TcpManagerImpl::~TcpManagerImpl()
{

}

bool TcpManagerImpl::init(TcpServiceBase * tcp_service, std::size_t thread_count, const char * host, unsigned short port_array[], std::size_t port_count)
{
    if (nullptr == tcp_service)
    {
        return (false);
    }

    if (0 == thread_count)
    {
        return (false);
    }

    if (nullptr == port_array && 0 != port_count)
    {
        return (false);
    }

    if (m_io_context_pool.size() > 0)
    {
        return (false);
    }

    if (!m_io_context_pool.init(thread_count))
    {
        return (false);
    }

    m_tcp_service = tcp_service;

    try
    {
        for (std::size_t index = 0; index < port_count; ++index)
        {
            unsigned short port = port_array[index];
            endpoint_type endpoint(boost::asio::ip::address_v4::from_string(nullptr == host ? "0.0.0.0" : host), port);
            bool reuse_address = true;
            m_acceptors.push_back(boost::factory<acceptor_type *>()(m_io_context_pool.get(), endpoint, reuse_address));
            start_accept(m_acceptors.back(), port);
        }
    }
    catch (boost::system::error_code &)
    {
        return (false);
    }
    catch (...)
    {
        return (false);
    }

    return (true);
}

void TcpManagerImpl::exit()
{
    m_io_context_pool.exit();
    m_acceptors.clear();
    m_tcp_service = nullptr;
}

void TcpManagerImpl::run(bool blocking)
{
    m_io_context_pool.run(blocking);
}

void TcpManagerImpl::start_accept(acceptor_type & acceptor, unsigned short port)
{
    bool passive = true;
    const void * identity = reinterpret_cast<const void *>(port);
    tcp_connection_ptr tcp_connection = boost::factory<tcp_connection_ptr>()(m_io_context_pool.get(), m_tcp_service, passive, identity);
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

    tcp_connection->io_context().post(boost::bind(&TcpConnection::start, tcp_connection));
}

bool TcpManagerImpl::create_connection(const std::string & host, const std::string & service, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    if (sync_connect)
    {
        return (sync_create_connection(host, service, identity, bind_ip, bind_port));
    }
    else
    {
        return (async_create_connection(host, service, identity, bind_ip, bind_port));
    }
}

bool TcpManagerImpl::create_connection(const std::string & host, unsigned short port, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    return (create_connection(host, boost::lexical_cast<std::string>(port), sync_connect, identity, bind_ip, bind_port));
}

bool TcpManagerImpl::sync_create_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    boost::asio::ip::tcp::endpoint endpoint;
    if (nullptr == bind_ip || '\0' == *bind_ip)
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), bind_port);
    }
    else
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
    }

    bool passive = false;
    tcp_connection_ptr tcp_connection = boost::factory<tcp_connection_ptr>()(m_io_context_pool.get(), m_tcp_service, passive, identity);
    tcp_connection_type::socket_type & socket = tcp_connection->socket();

    boost::asio::ip::tcp::resolver resolver(tcp_connection->io_context());
    boost::asio::ip::tcp::resolver::query query(host, service);
    boost::asio::ip::tcp::resolver::iterator iter_end;
    boost::system::error_code error = boost::asio::error::host_not_found;
    for (boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query); error && iter_end != iter; ++iter)
    {
        socket.close(error);
        if (0 != endpoint.port() || !endpoint.address().is_unspecified())
        {
            socket.open(endpoint.protocol(), error);
            if (error)
            {
                return (false);
            }
            socket.set_option(boost::asio::ip::tcp::socket::reuse_address(true), error);
            if (error)
            {
                return (false);
            }
            socket.bind(endpoint, error);
            if (error)
            {
                return (false);
            }
        }
        socket.connect(*iter, error);
    }

    if (error)
    {
        return (false);
    }

    tcp_connection->io_context().post(boost::bind(&TcpConnection::start, tcp_connection));

    return (true);
}

bool TcpManagerImpl::async_create_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    boost::asio::ip::tcp::endpoint endpoint;
    if (nullptr == bind_ip || '\0' == *bind_ip)
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), bind_port);
    }
    else
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
    }

    bool passive = false;
    tcp_connection_ptr tcp_connection = boost::factory<tcp_connection_ptr>()(m_io_context_pool.get(), m_tcp_service, passive, identity);

    boost::asio::ip::tcp::resolver::query query(host, service);
    resolver_ptr resolver = boost::factory<resolver_ptr>()(tcp_connection->io_context());
    resolver->async_resolve(query, boost::bind(&TcpConnection::handle_resolve, tcp_connection, boost::asio::placeholders::error, boost::asio::placeholders::iterator, endpoint, resolver));

    return (true);
}

} // namespace BoostNet end
