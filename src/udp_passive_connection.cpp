/********************************************************
 * Description : udp passive connection
 * Data        : 2019-07-22 19:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <boost/bind.hpp>
#include "udp_acceptor.h"
#include "udp_passive_connection.h"

namespace BoostNet { // namespace BoostNet begin

UdpPassiveConnection::UdpPassiveConnection(UdpAcceptor & acceptor, UdpServiceBase * udp_service, unsigned short host_port, endpoint_type endpoint)
    : m_acceptor(acceptor)
    , m_udp_service(udp_service)
    , m_running(false)
    , m_host_port(host_port)
    , m_endpoint(endpoint)
    , m_peer_ip()
    , m_peer_port(0)
    , m_recv_buffer()
{

}

UdpPassiveConnection::~UdpPassiveConnection()
{

}

void UdpPassiveConnection::start()
{
    boost::system::error_code ignore_error_code;
    m_peer_ip = m_endpoint.address().to_string(ignore_error_code);
    m_peer_port = m_endpoint.port();

    m_running = true;

    if (nullptr != m_udp_service)
    {
        if (!m_udp_service->on_accept(shared_from_this(), m_host_port))
        {
            close();
            return;
        }
    }
}

void UdpPassiveConnection::stop()
{
    if (m_running)
    {
        if (nullptr != m_udp_service)
        {
            m_udp_service->on_close(shared_from_this());
        }
        m_running = false;
    }
}

void UdpPassiveConnection::send(const void * data, std::size_t len)
{
    m_acceptor.send(m_endpoint, data, len);
}

void UdpPassiveConnection::recv(const void * data, std::size_t len)
{
    if (nullptr != m_udp_service)
    {
        m_recv_buffer.emplace_back(std::vector<char>(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + len));
        if (!m_udp_service->on_recv(shared_from_this()))
        {
            close();
            return;
        }
    }
}

void UdpPassiveConnection::close()
{
    m_acceptor.close(m_endpoint);
}

void UdpPassiveConnection::get_host_address(std::string & ip, unsigned short & port)
{
    m_acceptor.get_host_address(ip, port);
}

void UdpPassiveConnection::get_peer_address(std::string & ip, unsigned short & port)
{
    ip = m_peer_ip;
    port = m_peer_port;
}

bool UdpPassiveConnection::recv_buffer_has_data()
{
    return (!m_recv_buffer.empty());
}

const void * UdpPassiveConnection::recv_buffer_data()
{
    if (m_recv_buffer.empty())
    {
        return (nullptr);
    }
    if (m_recv_buffer.front().empty())
    {
        return (nullptr);
    }
    return (reinterpret_cast<const void *>(&m_recv_buffer.front()[0]));
}

std::size_t UdpPassiveConnection::recv_buffer_size()
{
    if (m_recv_buffer.empty())
    {
        return (0);
    }
    return (m_recv_buffer.front().size());
}

bool UdpPassiveConnection::recv_buffer_drop()
{
    if (m_recv_buffer.empty())
    {
        return (false);
    }
    m_recv_buffer.pop_front();
    return (true);
}

bool UdpPassiveConnection::send_buffer_fill(const void * data, std::size_t len)
{
    if (nullptr == data && 0 != len)
    {
        return (false);
    }
    send(data, len);
    return (true);
}

} // namespace BoostNet end
