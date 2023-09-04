/********************************************************
 * Description : udp manager implement
 * Data        : 2019-07-23 09:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include <boost/lexical_cast.hpp>
#include "udp_manager_impl.h"

namespace BoostNet { // namespace BoostNet begin

UdpManagerImpl::UdpManagerImpl()
    : m_io_context_pool()
    , m_udp_service(nullptr)
    , m_udp_ports()
{

}

UdpManagerImpl::~UdpManagerImpl()
{

}

bool UdpManagerImpl::init(UdpServiceBase * udp_service, std::size_t thread_count, const char * host, unsigned short port_array[], std::size_t port_count, bool port_any_valid)
{
    if (nullptr == udp_service)
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

    m_udp_service = udp_service;

    m_udp_ports.clear();

    if (0 == port_count)
    {
        return (true);
    }

    if (port_any_valid)
    {
        std::size_t index = 0;
        while (index < port_count)
        {
            unsigned short port = port_array[index];
            if (0 == port)
            {
                m_udp_service->on_error(UdpConnectionSharedPtr(), "listener", "init", 1, "port is invalid");
            }
            else
            {
                udp_acceptor_ptr udp_acceptor = boost::factory<udp_acceptor_ptr>()(m_io_context_pool.get(), m_udp_service, host, port);
                if (udp_acceptor->start())
                {
                    m_udp_ports.push_back(port);
                    break;
                }
            }
            ++index;
        }
        if (index == port_count)
        {
            return (false);
        }
    }
    else
    {
        for (std::size_t index = 0; index < port_count; ++index)
        {
            unsigned short port = port_array[index];
            if (0 == port)
            {
                m_udp_service->on_error(UdpConnectionSharedPtr(), "listener", "init", 1, "port is invalid");
                return (false);
            }
            else
            {
                udp_acceptor_ptr udp_acceptor = boost::factory<udp_acceptor_ptr>()(m_io_context_pool.get(), m_udp_service, host, port);
                if (!udp_acceptor->start())
                {
                    return (false);
                }
            }
        }
        m_udp_ports.assign(port_array, port_array + port_count);
    }

    return (true);
}

void UdpManagerImpl::exit()
{
    m_io_context_pool.exit();
    m_udp_service = nullptr;
    m_udp_ports.clear();
}

void UdpManagerImpl::get_ports(std::vector<uint16_t> & ports)
{
    ports = m_udp_ports;
}

void UdpManagerImpl::run(bool blocking)
{
    m_io_context_pool.run(blocking);
}

bool UdpManagerImpl::create_connection(const std::string & host, const std::string & service, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
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

bool UdpManagerImpl::create_connection(const std::string & host, unsigned short port, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    return (create_connection(host, boost::lexical_cast<std::string>(port), sync_connect, identity, bind_ip, bind_port));
}

bool UdpManagerImpl::sync_create_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    boost::asio::ip::udp::endpoint endpoint;
    if (nullptr == bind_ip || '\0' == *bind_ip)
    {
        endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), bind_port);
    }
    else
    {
        endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
    }

    udp_connection_ptr udp_connection = boost::factory<udp_connection_ptr>()(m_io_context_pool.get(), m_udp_service, identity);
    udp_connection_type::socket_type & socket = udp_connection->socket();

    boost::asio::ip::udp::resolver resolver(udp_connection->io_context());
    boost::asio::ip::udp::resolver::query query(host, service);
    boost::asio::ip::udp::resolver::iterator iter_end;
    boost::system::error_code error = boost::asio::error::host_not_found;
    for (boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query); error && iter_end != iter; ++iter)
    {
        socket.close(error);
        if (0 != endpoint.port() || !endpoint.address().is_unspecified())
        {
            socket.open(endpoint.protocol(), error);
            if (error)
            {
                return (false);
            }
            socket.set_option(boost::asio::ip::udp::socket::reuse_address(true), error);
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

    udp_connection->io_context().post(boost::bind(&UdpActiveConnection::start, udp_connection));

    return (true);
}

bool UdpManagerImpl::async_create_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    boost::asio::ip::udp::endpoint endpoint;
    if (nullptr == bind_ip || '\0' == *bind_ip)
    {
        endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), bind_port);
    }
    else
    {
        endpoint = boost::asio::ip::udp::endpoint(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
    }

    udp_connection_ptr udp_connection = boost::factory<udp_connection_ptr>()(m_io_context_pool.get(), m_udp_service, identity);

    boost::asio::ip::udp::resolver::query query(host, service);
    resolver_ptr resolver = boost::factory<resolver_ptr>()(udp_connection->io_context());
    resolver->async_resolve(query, boost::bind(&UdpActiveConnection::handle_resolve, udp_connection, boost::asio::placeholders::error, boost::asio::placeholders::iterator, endpoint, resolver));

    return (true);
}

} // namespace BoostNet end
