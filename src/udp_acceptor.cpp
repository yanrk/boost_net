/********************************************************
 * Description : udp acceptor
 * Data        : 2019-07-22 20:00:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <cstring>
#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include "udp_passive_connection.h"
#include "udp_acceptor.h"

namespace BoostNet { // namespace BoostNet begin

UdpAcceptor::UdpAcceptor(io_context_type & io_context, UdpServiceBase * udp_service, const char * host, unsigned short port)
    : m_io_context(io_context)
    , m_udp_service(udp_service)
    , m_running(false)
    , m_host_endpoint(boost::asio::ip::address_v4::from_string(nullptr == host ? "0.0.0.0" : host), port)
    , m_peer_endpoint()
    , m_socket(io_context)
    , m_host_ip(nullptr == host ? "0.0.0.0" : host)
    , m_host_port(port)
    , m_connection_map()
    , m_send_buffer()
    , m_recv_data()
    , m_good(false)
{
    boost::system::error_code ec;

    m_socket.open(m_host_endpoint.protocol(), ec);
    if (ec)
    {
        m_udp_service->on_error(UdpConnectionSharedPtr(), "listener", "open", ec.value(), ec.message().c_str());
        return;
    }

    m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true), ec);
    if (ec)
    {
        m_udp_service->on_error(UdpConnectionSharedPtr(), "listener", "reuse", ec.value(), ec.message().c_str());
        return;
    }

    m_socket.bind(m_host_endpoint, ec);
    if (ec)
    {
        m_udp_service->on_error(UdpConnectionSharedPtr(), "listener", "bind", ec.value(), ec.message().c_str());
        return;
    }

    memset(m_recv_data, 0x0, sizeof(m_recv_data));

    m_good = true;
}

UdpAcceptor::~UdpAcceptor()
{

}

bool UdpAcceptor::start()
{
    if (m_good)
    {
        boost::system::error_code ignore_error_code;
        m_host_ip = m_host_endpoint.address().to_string(ignore_error_code);
        m_host_port = m_host_endpoint.port();

        m_running = true;

        recv();
    }

    return (m_good);
}

void UdpAcceptor::stop()
{
    if (m_running)
    {
        boost::system::error_code ignore_error_code;
        m_socket.shutdown(socket_type::shutdown_both, ignore_error_code);
        m_socket.close(ignore_error_code);
        m_io_context.post(boost::bind(&UdpAcceptor::handle_stop, shared_from_this()));
        m_running = false;
    }
}

void UdpAcceptor::handle_stop()
{
    for (udp_connection_map::iterator iter = m_connection_map.begin(); m_connection_map.end() != iter; ++iter)
    {
        udp_connection_ptr udp_connection = iter->second;
        udp_connection->stop();
    }
    m_connection_map.clear();
}

void UdpAcceptor::send(const endpoint_type & endpoint, const void * data, std::size_t len)
{
    m_io_context.post(boost::bind(&UdpAcceptor::push_send_data, shared_from_this(), std::make_pair(endpoint, std::vector<char>(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + len))));
}

void UdpAcceptor::push_send_data(endpoint_buffer_type endpoint_data)
{
    bool need_send = m_send_buffer.empty();
    m_send_buffer.emplace_back(std::move(endpoint_data));
    if (need_send)
    {
        send();
    }
}

void UdpAcceptor::send()
{
    m_socket.async_send_to(boost::asio::buffer(m_send_buffer.front().second), m_send_buffer.front().first, boost::bind(&UdpAcceptor::handle_send, shared_from_this(), boost::asio::placeholders::error));
}

void UdpAcceptor::handle_send(const boost::system::error_code & error)
{
    BOOST_ASSERT(!m_send_buffer.empty());

    m_send_buffer.pop_front();

    if (!m_send_buffer.empty())
    {
        send();
    }
}

void UdpAcceptor::recv()
{
    m_socket.async_receive_from(boost::asio::buffer(m_recv_data, sizeof(m_recv_data)), m_peer_endpoint, boost::bind(&UdpAcceptor::handle_recv, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void UdpAcceptor::handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred)
{
    udp_connection_ptr udp_connection;

    udp_connection_map::iterator iter = m_connection_map.find(m_peer_endpoint);
    if (m_connection_map.end() == iter)
    {
        udp_connection = boost::factory<udp_connection_ptr>()(*this, m_udp_service, m_host_port, m_peer_endpoint);
        m_connection_map.insert(std::make_pair(m_peer_endpoint, udp_connection));
        udp_connection->start();
    }
    else
    {
        udp_connection = iter->second;
    }

    udp_connection->recv(m_recv_data, bytes_transferred);

    recv();
}

void UdpAcceptor::close(const endpoint_type & endpoint)
{
    m_io_context.post(boost::bind(&UdpAcceptor::handle_close, shared_from_this(), endpoint));
}

void UdpAcceptor::handle_close(endpoint_type endpoint)
{
    udp_connection_map::iterator iter = m_connection_map.find(endpoint);
    if (m_connection_map.end() != iter)
    {
        udp_connection_ptr udp_connection = iter->second;
        udp_connection->stop();
        m_connection_map.erase(iter);
    }
}

void UdpAcceptor::get_host_address(std::string & ip, unsigned short & port)
{
    ip = m_host_ip;
    port = m_host_port;
}

} // namespace BoostNet end
