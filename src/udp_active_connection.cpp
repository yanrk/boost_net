/********************************************************
 * Description : udp active connection
 * Data        : 2019-07-22 14:00:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <cstring>
#include <boost/bind.hpp>
#include "udp_active_connection.h"

namespace BoostNet { // namespace BoostNet begin

UdpActiveConnection::UdpActiveConnection(io_context_type & io_context, UdpServiceBase * udp_service, std::size_t identity)
    : m_io_context(io_context)
    , m_udp_service(udp_service)
    , m_running(false)
    , m_identity(identity)
    , m_socket(io_context)
    , m_host_ip()
    , m_host_port(0)
    , m_peer_ip()
    , m_peer_port(0)
    , m_recv_buffer()
    , m_send_buffer()
    , m_recv_data()
{
    memset(m_recv_data, 0x0, sizeof(m_recv_data));
}

UdpActiveConnection::~UdpActiveConnection()
{

}

UdpActiveConnection::socket_type & UdpActiveConnection::socket()
{
    return (m_socket);
}

UdpActiveConnection::io_context_type & UdpActiveConnection::io_context()
{
    return (m_io_context);
}

void UdpActiveConnection::start()
{
    boost::system::error_code ignore_error_code;
    m_host_ip = m_socket.local_endpoint(ignore_error_code).address().to_string(ignore_error_code);
    m_host_port = m_socket.local_endpoint(ignore_error_code).port();
    m_peer_ip = m_socket.remote_endpoint(ignore_error_code).address().to_string(ignore_error_code);
    m_peer_port = m_socket.remote_endpoint(ignore_error_code).port();

    m_running = true;

    if (nullptr != m_udp_service)
    {
        if (!m_udp_service->on_connect(shared_from_this(), m_identity))
        {
            close();
            return;
        }
    }

    recv();
}

void UdpActiveConnection::stop()
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

void UdpActiveConnection::handle_resolve(const boost::system::error_code & error, boost::asio::ip::udp::resolver::iterator iterator, boost::asio::ip::udp::endpoint host_endpoint, resolver_ptr resolver)
{
    boost::asio::ip::udp::resolver::iterator iter_end;
    if (iter_end != iterator)
    {
        boost::system::error_code connect_error_code = boost::asio::error::host_not_found;
        m_socket.close(connect_error_code);
        if (0 != host_endpoint.port() || !host_endpoint.address().is_unspecified())
        {
            m_socket.open(host_endpoint.protocol(), connect_error_code);
            if (connect_error_code)
            {
                return;
            }
            m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true), connect_error_code);
            if (connect_error_code)
            {
                return;
            }
            m_socket.bind(host_endpoint, connect_error_code);
            if (connect_error_code)
            {
                return;
            }
        }
        boost::asio::ip::udp::endpoint peer_endpoint(*iterator++);
        m_socket.async_connect(peer_endpoint, boost::bind(&UdpActiveConnection::handle_connect, shared_from_this(), boost::asio::placeholders::error, iterator, host_endpoint, resolver));
    }
}

void UdpActiveConnection::handle_connect(const boost::system::error_code & error, boost::asio::ip::udp::resolver::iterator iterator, boost::asio::ip::udp::endpoint host_endpoint, resolver_ptr resolver)
{
    if (error)
    {
        handle_resolve(error, iterator, host_endpoint, resolver);
    }
    else
    {
        m_io_context.post(boost::bind(&UdpActiveConnection::start, shared_from_this()));
    }
}

void UdpActiveConnection::close()
{
    boost::system::error_code ignore_error_code;
    m_socket.shutdown(socket_type::shutdown_both, ignore_error_code);
    m_socket.close(ignore_error_code);
    m_io_context.post(boost::bind(&UdpActiveConnection::stop, shared_from_this()));
}

void UdpActiveConnection::recv()
{
    m_socket.async_receive(boost::asio::buffer(m_recv_data, sizeof(m_recv_data)), boost::bind(&UdpActiveConnection::handle_recv, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UdpActiveConnection::send()
{
    m_socket.async_send(boost::asio::buffer(m_send_buffer.front()), boost::bind(&UdpActiveConnection::handle_send, shared_from_this(), boost::asio::placeholders::error));
}

void UdpActiveConnection::post_send_data(const void * data, std::size_t len)
{
    m_io_context.post(boost::bind(&UdpActiveConnection::push_send_data, shared_from_this(), std::vector<char>(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + len)));
}

void UdpActiveConnection::push_send_data(std::vector<char> data)
{
    bool need_send = m_send_buffer.empty();
    m_send_buffer.emplace_back(std::move(data));
    if (need_send)
    {
        send();
    }
}

void UdpActiveConnection::handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred)
{
    if (error)
    {
        close();
        return;
    }

    if (nullptr != m_udp_service)
    {
        m_recv_buffer.emplace_back(std::vector<char>(m_recv_data, m_recv_data + bytes_transferred));
        if (!m_udp_service->on_recv(shared_from_this()))
        {
            close();
            return;
        }
    }

    recv();
}

void UdpActiveConnection::handle_send(const boost::system::error_code & error)
{
    if (error)
    {
        close();
        return;
    }

    BOOST_ASSERT(!m_send_buffer.empty());

    m_send_buffer.pop_front();

    if (m_send_buffer.empty())
    {
        if (nullptr != m_udp_service)
        {
            if (!m_udp_service->on_send(shared_from_this()))
            {
                close();
                return;
            }
        }
    }
    else
    {
        send();
    }
}

void UdpActiveConnection::get_host_address(std::string & ip, unsigned short & port)
{
    ip = m_host_ip;
    port = m_host_port;
}

void UdpActiveConnection::get_peer_address(std::string & ip, unsigned short & port)
{
    ip = m_peer_ip;
    port = m_peer_port;
}

bool UdpActiveConnection::recv_buffer_has_data()
{
    return (!m_recv_buffer.empty());
}

const void * UdpActiveConnection::recv_buffer_data()
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

std::size_t UdpActiveConnection::recv_buffer_size()
{
    if (m_recv_buffer.empty())
    {
        return (0);
    }
    return (m_recv_buffer.front().size());
}

bool UdpActiveConnection::recv_buffer_drop_len(std::size_t len)
{
    if (m_recv_buffer.empty())
    {
        return (false);
    }
    if (m_recv_buffer.front().size() != len)
    {
        return (false);
    }
    m_recv_buffer.pop_front();
    return (true);
}

bool UdpActiveConnection::send_buffer_fill_len(const void * data, std::size_t len)
{
    if (nullptr == data && 0 != len)
    {
        return (false);
    }
    post_send_data(data, len);
    return (true);
}

} // namespace BoostNet end
